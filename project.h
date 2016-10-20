#ifndef __PROJECT_H
#define __PROJECT_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//
//	Modified (c) Pete Brownlow 31/3/11 to add status outputs on spare pins for scope debugging,
//                                 introduce conditional compiler directive for BC1a
//								   and add definitions used by node parameters
//				  Mike Bolton	19/4/11 Modified for 32 MHz clock
//				Pete Brownlow	27/6/11 Add CAN transmit error checking, beep twice if unable to transmit on CAN
//										Output bridge enable turned off during overload


// Uncomment to build for the appropriate hardware

#define CANCMD



// SPROG headers
//#include "cancmd.h"
#include "commands.h"
#include "power_cmds.h"
#include "program_cmds.h"

// headers for this project
#include "isr_high.h"
#include "mode_cmds.h"
#include "packet_gen.h"

// for DCC debug packets
#define HAS_DCC

// CAN mode for setup
#define ECAN_MODE_2

// CBUS headers
// One Rx buffer and one Tx buffer must be defined before
// including cbus_common.h
#define TX1

// This device has a fixed CAN ID

// 06/04/11 Roger Healey - Put CANCMD constants inside #ifdef
//  					 - Add MODULE_ID as MTYP_CANCMD		

#include "cbus_common.h"
#include "can_send.h"
#include "cbusdefs7e.h"


#define MODULE_ID 	MTYP_CANCMD

    
#define FIXED_CAN_ID 0x72
#define DEFAULT_NN 	0xFFFE
#define MAJOR_VER 	3
#define MINOR_VER 	'0'	// Minor version character

#define NV_NUM		0	// Number of node variables
#define EVT_NUM		0	// Number of event variables
#define EVperEVT	0	// Event variables per event



extern unsigned char Latcount;
extern unsigned short NN_temp;


// DEFINE INPUT AND OUTPUT PINS
// ??? NOTE - BC1a still wip, ddr and init not yet worked out 

//
// Port A analogue inputs and digital I/O
//


  // CANCMD
  // RA0 is AN0 current sense
  //
#define SW          	25	// Flim switch
#define AWD         	24	// Sounder



//
// Port B
// Note that RB2 and RB3 are canrx and cantx so not used as I/O pins
//           RB6 and RB7 are used as PGC and PGD but can also drive LEDs
//
#define LEDCANACT               13   // CBUS activity
#define LED1    		32	// Internal booster/service track
#define LED2    		33	// RUN indicator
#define PWRBUTTON    		6	// Power ON/OFF


#define SWAP_OP 		5	// Jumper for main on ext booster or on board


//
// Port C DCC and diagnostic outputs
//

  #define OVERLOAD_PIN 	26		// (pin 18) For scope debug - set bit when overload detected
  #define BOOSTER_OUT		27		// (pin 17) For scope debug - set bit when in high priority ISR
  //#define DCC_PKT_PIN	PORTCbits.RC5		// (pin 16) For scope debug - Set during packet send (can sync scope on packet start)
  //#define SHUTDOWN_N    PORTCbits.RC4
  #define START_PREAMBLE    28   //(pin 15) used to show when the first bit of a preamble is active
  #define DCC_OUT_POS       21
  #define DCC_OUT_NEG       20
  #define DCC_NEG		23	    // other side o/p 
  #define DCC_POS		22	    // one side of booster H-bridge o/p 
  #define DCC_EN        19



#endif	// __PROJECT_H
