#include <FlexCAN.h>
#include <kinetis_flexcan.h>

#include "project.h"

// Ensure newly programmed part is in virgin state
unsigned char defaultID = 0x7F;
unsigned char status = 0;
unsigned short nodeID = 0;

#ifdef HAS_EVENTS
	unsigned short ENindex = 0;
	event ENstart[EN_NUM];
	unsigned short EVstart[EN_NUM];
#endif


/*
 * Common CBUS CAN setup
 */
void cbus_setup(void) {
  // Default mask is allow everything
  //defaultMask.rtr = 0;
  //defaultMask.ext = 0;
  //defaultMask.id = 0;
  //Can0.begin(125000, 1, 1);

  Can0.begin(125000, {0,0,0}, 1, 1);  //Setup the CAN port on the alternate pins of the unit with a default mask
  //Can0.begin(125000, defMask, 1, 1);  //Setup the CAN port on the alternate pins of the unit
  #ifdef __MK66FX1M0__
    Can1.begin(250000); //If using the Teensy 3.6, also enable the second CAN port
  #endif
  
}

/*
 * ee_read_short() - read a short (16 bit) word from EEPROM
 *
 * Data is stored in little endian format
 */
uint16_t ee_read_short(unsigned char addr) {
  unsigned char byte_addr = addr;
  uint16_t ret = EEPROM.read(byte_addr++);
  ret = ret | ((uint16_t)EEPROM.read(byte_addr) << 8);
  return ret;
}

/*
 * ee_write_short() - write a short (16 bit) data to EEPROM
 *
 * Data is stored in little endian format
 */
void ee_write_short(unsigned char addr, uint16_t data) {
  unsigned char byte_addr = addr;
  EEPROM.update(byte_addr++, (unsigned char)data);
  EEPROM.update(byte_addr, (unsigned char)(data>>8));
}

//
// ecan_fifo_empty()
//
// Check if ECAN FIFO has a valid receive buffer available and
// preload the pointer to it
// 
unsigned char ecan_fifo_empty(void) {
    
    if (Can0.available()) {
      Can0.read(rx_ptr);
      CAN2Serial(rx_ptr);
        // current buffer is not empty
        return 0;
    } else {
        return 1;
    }
}


