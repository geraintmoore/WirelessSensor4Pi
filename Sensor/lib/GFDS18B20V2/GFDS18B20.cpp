
#include "Energia.h"
#include "GFDS18B20.h"

// list of commands DS18B20:

#define DS1820_WRITE_SCRATCHPAD 	0x4E
#define DS1820_READ_SCRATCHPAD      0xBE
#define DS1820_COPY_SCRATCHPAD 		0x48
#define DS1820_READ_EEPROM 			0xB8
#define DS1820_READ_PWRSUPPLY 		0xB4
#define DS1820_SEARCHROM 			0xF0
#define DS1820_SKIP_ROM             0xCC
#define DS1820_READROM 				0x33
#define DS1820_MATCHROM 			0x55
#define DS1820_ALARMSEARCH 			0xEC
#define DS1820_CONVERT_T            0x44

extern const uint16_t port_to_dir[]; // fix
extern const uint16_t port_to_ren[]; // fix
extern const uint16_t port_to_input[];
extern const uint16_t port_to_output[];

#define portDirRegister(P)     ( (volatile uint8_t *)( port_to_dir[P]) )
#define portRenRegister(P)     ( (volatile uint8_t *)( port_to_ren[P]) )
#define portOutputRegister(P)  ( (volatile uint8_t *)( port_to_output[P]) )
#define portInputRegister(P)   ( (volatile uint8_t *)( port_to_input[P]) )


#define OW_LO {	*_OWPORTDIR |= _OWPORTPIN;	*_OWPORTREN &= ~_OWPORTPIN; *_OWPORTOUT &= ~_OWPORTPIN; }
#define OW_HI {	*_OWPORTDIR |= _OWPORTPIN;	*_OWPORTREN &= ~_OWPORTPIN; *_OWPORTOUT |= _OWPORTPIN; }
#define OW_RLS { *_OWPORTDIR &= ~_OWPORTPIN; *_OWPORTREN |= _OWPORTPIN; *_OWPORTOUT |= _OWPORTPIN; }
#define OW_IN (*_OWPORTIN & _OWPORTPIN)


DS18B20::DS18B20(uint8_t OWPIN)
{


uint8_t OWPORT = digitalPinToPort(OWPIN);
	_OWPORTPIN = digitalPinToBitMask(OWPIN);
	_OWPORTDIR = portDirRegister(OWPORT);
	_OWPORTREN = portRenRegister(OWPORT);
	_OWPORTIN  = portInputRegister(OWPORT);
	_OWPORTOUT = portOutputRegister(OWPORT);
}

/***************************************************************/

int32_t DS18B20::GetData(void)
{
    uint16_t temp;
    reset();
    write_byte(0xcc); // skip ROM command
    write_byte(0x44); // convert T command
    OW_HI
    delay(750);		// had incorrectly used delayMicroseconds()
    reset();
    write_byte(0xcc); // skip ROM command
    write_byte(0xbe); // read scratchpad command
    temp = ReadDS1820();
	
	int16_t stemp = (int16_t)temp;
    return((int32_t)stemp*625/10000);

}

int32_t DS18B20::GetData10(void)
{
    uint16_t temp;
    reset();
    write_byte(0xcc); // skip ROM command
    write_byte(0x44); // convert T command
    OW_HI
    delay(750);		// had incorrectly used delayMicroseconds()
    reset();
    write_byte(0xcc); // skip ROM command
    write_byte(0xbe); // read scratchpad command
    temp = ReadDS1820();
    
	int16_t stemp = (int16_t)temp;

    return((int32_t)stemp*6250/10000);	
}

uint16_t DS18B20::ReadDS1820 ( void )
{
  unsigned int i;
  uint16_t byte = 0;
  for(i = 16; i > 0; i--){
    byte >>= 1;
    if (read_bit()) {
      byte |= 0x8000;
    }
  }
  return byte;
}
int DS18B20::reset(void)
{
	OW_LO
	delayMicroseconds(500); // 480us minimum  // try 500
	OW_RLS
	delayMicroseconds(80); // slave waits 15-60us  // try 80 or 40
	if (OW_IN) return 1; // line should be pulled down by slave
	delayMicroseconds(300); // slave TX presence pulse 60-240us
	if (!OW_IN) return 2; // line should be "released" by slave
	return 0;
}

void DS18B20::write_bit(int bit)
{
  delayMicroseconds(1); // recovery, min 1us
  OW_HI
  if (bit) {
    OW_LO
    delayMicroseconds(5); // max 15us
    OW_RLS	// input
    delayMicroseconds(56);
  }
  else {
    OW_LO
    delayMicroseconds(60); // min 60us
    OW_RLS	// input
    delayMicroseconds(1);
  }
 }

//#####################################################################

int DS18B20::read_bit()
{
  int bit=0;
  delayMicroseconds(1);
  OW_LO
  delayMicroseconds(5); // hold min 1us
  OW_RLS
  delayMicroseconds(10); // 15us window
  if (OW_IN) {
    bit = 1;
  }
  delayMicroseconds(46); // rest of the read slot
  return bit;
}

//#####################################################################

void DS18B20::write_byte(uint8_t byte)
{
  int i;
  for(i = 0; i < 8; i++)
  {
    write_bit(byte & 1);
    byte >>= 1;
  }
}

//#####################################################################

uint8_t DS18B20::read_byte()
{
  unsigned int i;
  uint8_t byte = 0;
  for(i = 0; i < 8; i++)
  {
    byte >>= 1;
    if (read_bit()) byte |= 0x80;
  }
  return byte;
}


void DS18B20::reset_search()
  {
  // reset the search state
  LastDiscrepancy = 0;
  LastDeviceFlag = FALSE;
  LastFamilyDiscrepancy = 0;
  for(int i = 7; ; i--)
    {
    ROM_NO[i] = 0;
    if ( i == 0) break;
    }
  }

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
uint8_t DS18B20::search(uint8_t *newAddr)
{
   uint8_t id_bit_number;
   uint8_t last_zero, rom_byte_number, search_result;
   uint8_t id_bit, cmp_id_bit;
   uint8_t ii=0;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
	
   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      ii=reset();
	  if (ii) // ii>0
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return ii;	// Pass back the reset error status  gf***
      }
      // issue the search command

      write_byte(0xF0);

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = read_bit();
         cmp_id_bit = read_bit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            write_bit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!(id_bit_number < 65))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = TRUE;

         //search_result = 0;	// All OK status GF***
		 search_result =30;	// All OK status GF***
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   //if (search_result || !ROM_NO[0])
   //if (!ROM_NO[0])
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = 3;	//FALSE;  //GF***
   }
   for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
   return search_result;
  }
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
//
  /*
uint8_t DS18B20::crc8( uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
*/
void DS18B20::select( uint8_t rom[8])
{
    int i;

    write_byte(0x55);           // Choose ROM

    for( i = 0; i < 8; i++) write_byte(rom[i]);
}
//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.


//void DS18B20::write(uint8_t v, uint8_t power /* = 0 */) {
/*    uint8_t bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	OneWire::write_bit( (bitMask & v)?1:0);
    }
    if ( !power) {
	noInterrupts();   // cli();
	DIRECT_MODE_INPUT(baseReg, bitmask);
	DIRECT_WRITE_LOW(baseReg, bitmask);
	interrupts();   // sei();
    }
}
*/
