#ifndef __OPCODES_H
#define __OPCODES_H

/*
 * Copyright (C) 2009 Andrew Crosland
 */

#include "project.h"

typedef unsigned long event;

void enclear(void);
unsigned char learnin(void);
unsigned char enmatch(void);
void en_ram(void);

#define OPC_HLT     0x02    // Bus Halt
#define OPC_BON     0x03    // Bus on
#define OPC_TOF     0x04    // Track off
#define OPC_TON     0x05    // Track on
#define OPC_ESTOP   0x06    // Track stopped
#define OPC_ARST    0x07    // System reset
#define OPC_RTOF    0x08    // Request track off
#define OPC_RTON    0x09    // Request track on
#define OPC_RESTP   0x0a    // Request emergency stop all

#define OPC_RQNP    0x10    // Read node parameters
#define OPC_RCOF     0x12    // Railcom off
#define OPC_RCON     0x13    // Railcom on

#define OPC_KLOC    0x21    // Release engine by handle
#define OPC_QLOC    0x22    // Query engine by handle
#define OPC_KEEP    0x23    // Keep alive for cab

#define OPC_DBG1    0x30    // Debug message with 1 status byte

#define OPC_RLOC    0x40    // Request session for loco
#define OPC_QCON    0x41    // Query consist
#define OPC_SNN		  0x42    // Set node number
#define OPC_STMOD   0x44    // Set Throttlle mode
#define OPC_PCON    0x45    // Consist loco
#define OPC_KCON    0x46    // De-consist loco
#define OPC_DSPD    0x47    // Loco speed/dir
#define OPC_SSTAT   0x4C    // Service mode status

#define OPC_NNACK	0x50
#define OPC_NNREL	0x51
#define OPC_NNREF	0x52
#define OPC_NNLRN	0x53
#define OPC_NNULN	0x54
#define OPC_NNCLR	0x55
#define OPC_NNEVN	0x56
#define OPC_BOOT	0x5C

#define OPC_DFUN    0x60    // Set engine functions
#define OPC_ERR     0x63    // Command station error

#define OPC_EVNLF	0x70
#define OPC_NVRD	0x71

#define OPC_RDCC3   0x80    // 3 byte DCC packet
#define OPC_WCVO    0x82    // Write CV byte Ops mode by handle
#define OPC_WCVB    0x83    // Write CV bit Ops mode by handle
#define OPC_QCVS    0x84    // Read CV
#define OPC_PCVS    0x85    // Report CV

#define	OPC_ACON	0x90	// on event
#define	OPC_ACOF	0x91	// off event
#define OPC_AREQ	0x92
#define	OPC_EVULN	0x95
#define OPC_NVSET	0x96
#define OPC_NVANS	0x97
#define	OPC_ASON	0x98
#define	OPC_ASOF	0x99
#define OPC_ASRQ	0x9A

#define OPC_RDCC4   0xA0    // 4 byte DCC packet
#define OPC_WCVS    0xA2    // Write CV service mode

#define OPC_AREQ1	0xB2

#define OPC_RDCC5   0xC0    // 5 byte DCC packet
#define OPC_WCVOA   0xC1    // Write CV ops mode by address

#define OPC_EVLRN	0xd2
#define OPC_EVANS	0xd3

#define OPC_RDCC6   0xE0    // 6 byte DCC packets
#define OPC_PLOC	0xE1    // Loco session report
#define OPC_PARAMS	0xEF

// Modes for STMOD
#define TMOD_SPD_MASK   3
#define TMOD_SPD_128    0
#define TMOD_SPD_14     1
#define TMOD_SPD_28I    2
#define TMOD_SPD_28     3

// Error codes for OPC_ERR
#define ERR_LOCO_STACK_FULL     1
#define ERR_LOCO_ADDR_TAKEN     2
#define ERR_SESSION_NOT_PRESENT 3
#define ERR_NO_MORE_ENGINES     4
#define ERR_ENGINE_NOT_FOUND    5
#define ERR_CAN_BUS_ERROR       6
#define ERR_INVALID_REQUEST     7
#define ERR_SESSION_CANCELLED   8

// Status codes for OPC_SSTAT
#define SSTAT_NO_ACK    1
#define SSTAT_OVLD      2
#define SSTAT_WR_ACK    3
#define SSTAT_BUSY      4
#define SSTAT_CV_ERROR  5

#ifdef HAS_EVENTS
// Array for RAM copy of ENs & servo settings\par
	extern event EN1[EN_NUM];
	extern unsigned char ENcount;
	extern unsigned char EVtemp;		// holds current EV
	extern unsigned char EVtemp2;		// holds current EV qualifier
#endif

#endif	// __OPCODES_H
