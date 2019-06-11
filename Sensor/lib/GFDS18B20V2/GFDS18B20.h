#ifndef DS18B20_h
#define DS18B20_h

// tw
// Port and pins definition:

// [Ground] ----x         x              x----------------- [+5v]
//			      |					
//                                  |
//                                  |	  
//                                Digital pin

#include <inttypes.h>

#define FALSE 0
#define TRUE  1

class DS18B20
{
  private:

	volatile uint8_t * _OWPORTDIR;	// was uint16_t but got error with new IDE-0009
	volatile uint8_t * _OWPORTREN;	// was uint16_t but got error with new IDE-0009
	volatile uint8_t * _OWPORTIN;	// was uint16_t but got error with new IDE-0009
	volatile uint8_t * _OWPORTOUT;	// was uint16_t but got error with new IDE-0009
	uint8_t _OWPORTPIN;
	//uint16_t ReadDS1820(void); // moved to public
	//int reset();	// moved to Public
	void write_bit(int bit);
	int read_bit();
	//void write_byte(uint8_t byte); moved to Public
	//uint8_t read_byte();//
	
	unsigned char ROM_NO[8];
    uint8_t LastDiscrepancy;
    uint8_t LastFamilyDiscrepancy;
    uint8_t LastDeviceFlag;
	//unit8_t ii;


  public:
    DS18B20( uint8_t pin); // 1-wire pin
	int32_t GetData(void); // temp (integer)	
	int32_t GetData10(void); // temp*10 (integer)
    void reset_search(void);
	uint8_t search(uint8_t *newAddr);
    int reset();
    // Issue a 1-Wire rom select command, you do the reset first.
    void select( uint8_t rom[8]);

   // Write a byte. If 'power' is one then the wire is held high at
    // the end for parasitically powered devices. You are responsible
    // for eventually depowering it by calling depower() or doing
    // another read or write.
    //void write(uint8_t v, uint8_t power = 0);

    void write_byte(uint8_t byte);
	uint16_t ReadDS1820(void);
	uint8_t read_byte();//
    // Compute a Dallas Semiconductor 8 bit CRC, these are used in the
    // ROM and scratchpad registers.
    
    //static uint8_t crc8( uint8_t *addr, uint8_t len);
};

#endif
