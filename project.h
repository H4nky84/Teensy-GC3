#ifndef __PROJECT_H
#define __PROJECT_H


// headers for this project
#include "isr_high.h"
#include "mode_cmds.h"
#include "packet_gen.h"
#include "commands.h"
#include "power_cmds.h"
#include "program_cmds.h"
#include "typedefs.h"

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

//#define SW          	25	// Flim switch
//#define LED1    		  33	// Internal booster/service track
//#define LED2    		  34	// RUN indicator
//#define PWRBUTTON    	6	// Power ON/OFF
//#define SWAP_OP_HW 		5	// Jumper for main on ext booster or on board


#define DCC_EN            2
#define CIRCBUFFERSIZE    5
#define DCC_OUT_EN        5
#define LEDCANACT         6   // CBUS activity
#define DCC_OUT_POS       20
#define DCC_OUT_NEG       21
#define DCC_NEG           22      // other side o/p 
#define DCC_POS           23      // one side of booster H-bridge o/p 
#define OVERLOAD_PIN      26    // (pin 18) For scope debug - set bit when overload detected
#define BOOSTER_OUT       27    // (pin 17) For scope debug - set bit when in high priority ISR
#define START_PREAMBLE    35   //(pin 15) used to show when the first bit of a preamble is active
#define AWD               36  // Sounder


#endif	// __PROJECT_H
