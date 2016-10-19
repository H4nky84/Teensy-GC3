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


#ifdef ECAN_MODE_2
unsigned char ecan_fifo_empty(void);

typedef struct {
	unsigned char con;
	unsigned char sidh;
	unsigned char sidl;
	unsigned char eidh;
	unsigned char eidl;
	unsigned char dlc;
	unsigned char d0;
	unsigned char d1;
	unsigned char d2;
	unsigned char d3;
	unsigned char d4;
	unsigned char d5;
	unsigned char d6;
	unsigned char d7;
} ecan_rx_buffer;


#endif

#endif	// __CBUS_COMMON_H
