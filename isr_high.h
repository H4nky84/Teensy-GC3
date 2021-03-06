#ifndef __ISR_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

#define LONG_PRE	22	// Increased from 20 - PNB 16/4/11 to meet NMRA spec 
#define NORM_PRE	16  //Increased to 16 for railcom compatibility
#define CIRCBUFFERSIZE 5

//#include "project.h"

// ISR prototype 
extern void isr_high(void);
extern void isr_low(void);

extern unsigned int an0;
extern unsigned short retry_delay;
extern volatile unsigned long sum;
extern volatile unsigned long ave;

extern unsigned char bit_flag_s;
extern unsigned char dcc_bytes_s;
extern unsigned char dcc_pre_s;
extern unsigned char bit_flag_m;
extern unsigned char dcc_bytes_m;
extern unsigned char dcc_pre_m;
extern unsigned char bit_start_pre;

extern unsigned slot_timer;

extern volatile unsigned int ch1Current_readings[CIRCBUFFERSIZE];    //array for ring buffer of current readings
extern volatile unsigned int ch2Current_readings[CIRCBUFFERSIZE];
extern volatile unsigned char ch1Current_idx;     //indexes for ring buffers
extern volatile unsigned char ch2Current_idx;

#define __ISR_H
#endif
