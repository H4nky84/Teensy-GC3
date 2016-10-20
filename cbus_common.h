#ifndef __CBUS_COMMON_H
#define __CBUS_COMMON_H

/*
 * Copyright (C) 2008 Andrew Crosland
 */

#include "project.h"

extern unsigned char defaultID;
extern unsigned char status;
extern unsigned short nodeID;

#ifdef HAS_EVENTS
	extern unsigned short ENindex;
	extern unsigned long ENstart[EN_NUM];
	extern unsigned short EVstart[EN_NUM];
#endif

extern unsigned char bootflag;

void cbus_setup(void);
uint16_t ee_read_short(unsigned char addr);
void ee_write_short(unsigned char addr, uint16_t data);


unsigned char ecan_fifo_empty(void);


#endif	// __CBUS_COMMON_H
