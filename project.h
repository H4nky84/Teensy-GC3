#ifndef __PROJECT_H
#define __PROJECT_H


// headers for this project
#include "isr_high.h"
#include "mode_cmds.h"
#include "packet_gen.h"
#include "commands.h"
#include "power_cmds.h"
#include "program_cmds.h"

// for DCC debug packets
#define HAS_DCC

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

#define SW          	25	// Flim switch
#define AWD         	24	// Sounder
#define LEDCANACT               13   // CBUS activity
#define LED1    		32	// Internal booster/service track
#define LED2    		33	// RUN indicator

#define PWRBUTTON    		6	// Power ON/OFF
#define SWAP_OP 		5	// Jumper for main on ext booster or on board

#define OVERLOAD_PIN 	26		// (pin 18) For scope debug - set bit when overload detected
#define BOOSTER_OUT		27		// (pin 17) For scope debug - set bit when in high priority ISR
//#define DCC_PKT_PIN	PORTCbits.RC5		// (pin 16) For scope debug - Set during packet send (can sync scope on packet start)
//#define SHUTDOWN_N    PORTCbits.RC4
#define START_PREAMBLE    7   //(pin 15) used to show when the first bit of a preamble is active
#define DCC_OUT_POS       21
#define DCC_OUT_NEG       20
#define DCC_NEG		23	    // other side o/p 
#define DCC_POS		22	    // one side of booster H-bridge o/p 
#define DCC_EN        19



#endif	// __PROJECT_H
