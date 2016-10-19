#ifndef __PROGRAM_CMDS_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

extern unsigned char sm_handle;
extern unsigned int cv;
extern unsigned char cv_register;
extern unsigned char cv_page;
extern unsigned char mode;
extern unsigned char cv_data;

#define CV_IDLE             0
#define CV_PON1             1
#define CV_CHECK_OVLD       2
#define CV_WRITE_BIT_REC    3
#define CV_WRITE_BIT        4
#define CV_READ_BIT_V_1     5
#define CV_READ_BIT_1_REC   6
#define CV_READ_BIT_V_0     7
#define CV_READ_BIT_0_REC   8
#define CV_READ_BYTE_START  9
#define CV_READ_PAGE_SENT   10
#define CV_PAGE_SENT        11
#define CV_PAGE_REC         12
#define CV_READ             13
#define CV_REC              14
#define CV_POFF             15
extern unsigned char prog_state;

void cv_sm(void);
void cmd_cv(void);

#define __PROGRAM_CMDS_H
#endif
