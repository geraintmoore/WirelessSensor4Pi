# WirelessSensor4Pi
CC1101 DS18B20 Raspberry Pi Zero Sqlite3 LowPower IOT

### A very new beginner's project.


## Thank you - This is a mixture of others' projects and example codes

For the CC1101 driver 
- SpaceTeddy https://github.com/SpaceTeddy/CC1101.git

For DS18B20 driver
- GFDS18B20(StellarisOW)  Grant Forest 29 Jan 2013. From https://www.43oh.com


## Sensors

MCU: Texas Intruments' MSP430G2553

Temperature Sensor: DS18B20

Light sensor: a solar battery with 2 10k ohm resisters connected to msp430g2553's ADC pin.

Wireless : TI's CC1101

## Server

Raspberry Pi Zero W, Wireless is better for access to collect the data.

Wireless : TI's CC1101


## How it works
MCU will get the tempature and light data, and send from CC1101 to Pi Zero.

there is a Daemon Process running on the Pi side, listening to the radio. Once received data, put the RxData to Sqlite3 database file.

Sensor will upload the data about each 31 seconds. 

## How to compile and upload to the board

### Sensor
just download the Energia IDE (similar to Arduino), and copy the lib(CC1100 & DS18B20) to proper path.

Compile and upload the code to MCU with MSP430G2 Launchpad.

### Server

You need to install the libs before compiling.

[ ] wiringPi for CC1101 Driver.

[ ] sqlite3-dev for the storage of Data.

If you are using raspberry just as I did. You may install them with Apt-get. Don't forget to update apt cache.

then compile with g++
> g++ -lwiringPi -lsqlite3 cc1100_rasp.cpp Pi_receiver.cpp -o Daemon.out

Create the sqlite3 DB file, 3 columns
> create table sensors_v1(ID integer Primary Key Autoincrement, RxBuff TEXT, Datetime TEXT);

Then put both Daemon.out and the database file to /home/pi/.

Add the start-and-run command to /etc/rc.local file, Just before "exit 0".

> /home/pi/Daemon.out

Reboot and leave it there.


## Please change any code you like for your own version.



### PS: how to get data from sqlite3 database
> sqlite3 your_database_file_name.db

> sqlite> .headers on

> sqlite> .mode csv

> sqlite> .output data.csv

> sqlite> SELECT FROM table;

> sqlite> .quit
