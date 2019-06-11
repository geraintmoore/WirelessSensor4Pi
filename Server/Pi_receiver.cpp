#include "cc1100_raspi.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <time.h>

#include <sys/select.h>

#include <getopt.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define PACKAGE    "CC1100 SW"
#define VERSION_SW "0.9.1"

//--------------------------[Global CC1100 variables]--------------------------
uint8_t Tx_fifo[FIFOBUFFER], Rx_fifo[FIFOBUFFER];
uint8_t My_addr, Tx_addr, Rx_addr, Pktlen, pktlen, Lqi, Rssi;
uint8_t rx_addr,sender,lqi;
 int8_t rssi_dbm;

int cc1100_freq_select, cc1100_mode_select, cc1100_channel_select;

CC1100 cc1100;

//-------------------------- [End] --------------------------


int insert2db(char *insertrecord, char *dbname){

	sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open(dbname, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    
    rc = sqlite3_exec(db, insertrecord, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return 1;
    } 
    
    sqlite3_close(db);
}


//|============================ Main ============================|
int main(int argc, char *argv[]) {

	//set debug level
	cc1100.set_debug_level(0);

	//------------- hardware setup ------------------------
	wiringPiSetup();			//setup wiringPi library
	cc1100.begin(My_addr);			//setup cc1000 RF IC
	cc1100.sidle();
	cc1100.set_mode(0x03);                //set modulation mode 1 = GFSK_1_2_kb; 2 = GFSK_38_4_kb; 3 = GFSK_100_kb; 4 = MSK_250_kb; 5 = MSK_500_kb; 6 = OOK_4_8_kb
	cc1100.set_ISM(0x02);                 //set ISM Band 1=315MHz; 2=433MHz; 3=868MHz; 4=915MHz
	cc1100.set_channel(0x01);             //set channel  
	cc1100.set_output_power_level(0);       //set PA level
	cc1100.set_myaddr(0x00);
	
	cc1100.receive();
 


	//------------------------- Main Loop ------------------------
	for (;;) {
		char oneMSG[900]="";
		delay(1);                            //delay to reduce system load

		if (cc1100.packet_available())		 //checks if a packed is available
		{
		cc1100.get_payload(oneMSG, Rx_fifo, pktlen, rx_addr, sender, rssi_dbm, lqi); //stores the payload data to Rx_fifo

  		time_t timer;
    	char time_buffer[26];
	    struct tm* tm_info;

        //------------- class to read datetime ------------------------
	    time(&timer);
	    tm_info = localtime(&timer);
	    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	    //puts(time_buffer);

	    puts(oneMSG);

    	char record[2000] = "INSERT INTO sensors_v1 VALUES(NULL,'";
    	char mid[4]="','";
		char ending[4] = "');";

		strcat(record,oneMSG);
		strcat(record,mid);
		strcat(record,time_buffer);
		strcat(record,ending);
		
		insert2db(record, "/home/pi/sensorRX.db");
		}
	}
	return 0;
}
//-------------------- END OF MAIN --------------------------------
