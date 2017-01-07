#include "project.h"
#include <FlexCAN.h>


extern CAN_message_t Tx1;

/*
 * Send simple 3-byte frame
 */
void can_opc(unsigned char opc) {
	Tx1.buf[0] = opc;
	can_tx_nn(3);
}

/*
 * Send a CAN frame, putting Node Number in first two data bytes
 */
void can_tx_nn(unsigned char dlc_val) {
	Tx1.buf[1] = NN_temp>>8;
	Tx1.buf[2] = NN_temp&0xFF;
	can_tx(dlc_val);
}

//
// Send a Bus On message
//
void can_bus_on(void) {
    Tx1.buf[0] = OPC_BON;
    can_tx(1);
}

/*
 * Send a CAN frame where data bytes have already been loaded
 */
void can_tx(unsigned char dlc_val) {
	Tx1.len = dlc_val;				// data length
	Tx1.id &= 0b00001111111;		// clear old priority
	Tx1.id |= 0b10110000000;		// low priority
	Latcount = 10;
	sendTX1();			
}

/*
 * Send a debug message with one status byte
 */
void can_debug1(unsigned char status) {
	Tx1.buf[0] = OPC_DBG1;
	Tx1.buf[1] = status;
    can_tx(2);
}

#ifdef HAS_DCC
/*
 * Send a debug message with dcc packet being sent to rail
 */
void can_debug_dcc(void) {
	Tx1.buf[0] = 0xB8;
	Tx1.buf[1] = dcc_buff_m[0];
	Tx1.buf[2] = dcc_buff_m[1];
	Tx1.buf[3] = dcc_buff_m[2];
	Tx1.buf[4] = dcc_buff_m[3];
	Tx1.buf[5] = dcc_buff_m[4];
    can_tx(6);
}
#endif

/*
 * Send contents of Tx1 buffer via CAN TXB1
 */
void sendTX1(void) {
  boolean send_complete = 0;
	int i;

	can_transmit_timeout = 2;	// half second intervals
	op_flags.can_transmit_failed = 0;

 //Attempt to send the packet out at low priority up to 5 times if it wont send

  for (i = 5; i >= 0; i--) {
    if(CANbus.write(Tx1)) {
      send_complete = 1;
      break;
    }
  }

  //If this still didnt work, attempt up to 3 times to send it high priority

  if(send_complete == 0) {
    Tx1.id &= 0b00001111111;    // clear old priority
    Tx1.id |= 0b01110000000;    // high priority
    for (i = 3; i >= 0; i--) {
      if(CANbus.write(Tx1)) {
        send_complete = 1;
        break;
      }
    }
  }

  if(send_complete == 0) op_flags.can_transmit_failed = 0;

	//while((TXB1CONbits.TXREQ) && (!op_flags.can_transmit_failed) && (can_transmit_timeout != 0));
  //CANbus.write(Tx1);

  LEDCanActTimer = 2000;
  digitalWriteFast(LEDCANACT, 1);

  //Send the can packet out on the serial port too
  CAN2Serial(Tx1);
}


