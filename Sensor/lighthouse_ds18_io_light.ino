//Addr of long wire VCC is 0x02
//Addr of short wire VCC is 0x01

#include <GFDS18B20.h>
#include <CC1101_MSP430.h>
#define OWPIN  11  //11
#define MAXOW 2  //Max number of OW's used

byte ROMarray[MAXOW][8];
byte ROMtype[MAXOW];     // 28 for temp', 12 for switch etc.
byte ROMtemp[MAXOW];
byte result[MAXOW + 5];
uint8_t TempFloat;

byte data[12];
byte i;
byte addr[8];
uint8_t ROMmax = 0;
uint8_t ROMcount = 0;
boolean foundOW = false;

DS18B20 ds(OWPIN);  // currently on PIN 11

int sensorPin = A0;




//--------------------------[Global CC1100 variables]------------------------
uint8_t Tx_fifo[FIFOBUFFER], Rx_fifo[FIFOBUFFER];
uint8_t My_addr, Tx_addr, Rx_addr, Pktlen, pktlen, Lqi, Rssi;
uint8_t rx_addr, sender, lqi;
int8_t rssi_dbm;


//--------------------------[class constructors]-----------------------------
//init CC1100 constructor
CC1101 RF;

//---------------------------------[SETUP]-----------------------------------
void setup()
{
  // init serial Port for debugging
  Serial.begin(9600);
  Serial.println();
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  // -------- DS18B20 setup ---------------
  findOW();
  displayOW();

  // init CC1101 RF-module and get My_address from EEPROM
  RF.begin(My_addr);                   //inits RF module with main default settings
  Serial.println(F("CC1101 TX Demo for MSP430"));   //welcome message
}
//---------------------------------[LOOP]-----------------------------------
void loop()
{
  digitalWrite(12, HIGH);   // DS18B20 Power Pin high
  tempCMD();                // Start the Convert
  sleep(1000);              // wait until convention is done

  uint8_t LightHi = analogRead(sensorPin) >> 8;    // read solar battart votage
  uint8_t LightLow = analogRead(sensorPin);        // read solar battart votage

  RF.reset();                          // Re-setup CC1101 after reset.
  RF.sidle();                          //set to ILDE first
  RF.set_mode(0x03);                   //set modulation mode 1 = GFSK_1_2_kb; 2 = GFSK_38_4_kb; 3 = GFSK_100_kb; 4 = MSK_250_kb; 5 = MSK_500_kb; 6 = OOK_4_8_kb
  RF.set_ISM(0x02);                    //set frequency 1=315MHz; 2=433MHz; 3=868MHz; 4=915MHz
  RF.set_channel(0x01);                //set channel
  RF.set_output_power_level(0);        //set PA level in dbm
  RF.set_myaddr(0x02);                 //set my own address
  RF.receive();                        //set to RECEIVE mode
  uint8_t ID = 0xA1;

  for (i = 1; i < ROMmax + 1; i++) {
    if (ROMtype[i] == 0x28) {
      readOW(i);
      saveTemperature(i);
    }
  }
  digitalWrite(12, LOW);              // DS18B20 Power pin LOW to save power

  Rx_addr = 0x00;                     //receiver address

  uint8_t TempInteger = result[1];    //read the temperature from the first of the serial of DS18B20. pnly one DA18B20

  // the first 3 Bytes are not usable.
  Tx_fifo[3] = ID;                  //Seneor ID
  Tx_fifo[4] = LightHi;            // 10bit ADC High 2 bits
  Tx_fifo[5] = LightLow;           // 10bit ADC Low 8 bits
  Tx_fifo[6] = TempInteger;        // DS18B20 sign + integer 7bits
  Tx_fifo[7] = TempFloat;          // DS18B20 float part, x 0.625 already
  //  Tx_fifo[8] = ID;             // Change to a longer packet length (Pktlen), and send more data
  //  Tx_fifo[9] = ID;
  
  Pktlen = 0x0A;                   //Control the package Length.
  
  RF.send_packet(My_addr, Rx_addr, Tx_fifo, Pktlen, 40);   //sents package over air. ACK is received via GPIO polling
  RF.powerdown();

  sleep(30000);    // about 30 + 1 Sec


}
//--------------------------[end loop]----------------------------

void tempCMD(void) {     //Send a global temperature convert command
  ds.reset();
  ds.write_byte(0xcc);  // was ds.select(work); so request all OW's to do next command
  ds.write_byte(0x44);  // start conversion, with parasite power on at the end
}
void saveTemperature(uint8_t ROMno) {
  int32_t newtemp32;
  uint8_t i;
  //带符号位的整数部分  最高位为1时证明为负值，需要加上-号，数值上减去128 Bin(1000 0000) HEx 0x80
  //sign + integer part. ht highest bit is for sign, 0x1000 0000 means temperature below Zero.
  newtemp32 = data[1] << 8;
  newtemp32 = newtemp32 + data[0] >> 4;        

// Float part, 4 bits need to plus 0.0625 in the Post data processing. 
  TempFloat = data[0] & 0x0F;  //小数部分
  result[ROMno] = byte(newtemp32);

  //小数部分四舍五入处理
  // i = (data[0] & 0x0F) * 625 / 1000;
  // if (i >= 5)  result[ROMno]++;
}
void readOW(uint8_t ROMno)
{
  uint8_t i;
  ds.reset();
  ds.select(ROMarray[ROMno]);
  ds.write_byte(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {   // need 9 bytes
    data[i] = ds.read_byte();
#if TEST
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
#endif
  }
}
void findOW(void)
{
  byte addr[8];
  uint8_t i;
  ROMmax = 0; ///////////////////////////////////////////////////////
  while (true) { //get all the OW addresses on the buss
    i = ds.search(addr);
    if ( i < 10) {
      Serial.print("ret=(");
      Serial.print(i);
      Serial.print(") No more addresses.\n");
      ds.reset_search();
      sleep(500);
      return;
    }
    Serial.print("R=");
    for ( i = 0; i < 8; i++) {
      if (i == 0)  ROMtype[ROMmax + 1] = addr[i]; // store the device type

      ROMarray[ROMmax + 1][i] = addr[i];

      if (addr[i] < 16) Serial.print("0");
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    ROMmax++;
    Serial.print ("\t(OW");
    Serial.print (ROMmax, HEX);
    Serial.print (") Type=");
    Serial.println (ROMtype[ROMmax], HEX);


  }
}
void displayOW(void)
{
  uint8_t i;
  Serial.println ("From array");
  for (ROMcount = 1; ROMcount < ROMmax + 1; ROMcount++) {
    ds.reset();
    for ( i = 0; i < 8; i++) {
      if (ROMarray[ROMcount][i] < 16) Serial.print("0");
      Serial.print( ROMarray[ROMcount][i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
  }
}
