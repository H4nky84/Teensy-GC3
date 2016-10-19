#ifndef __COMMANDS_H
#define __COMMANDS_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

// 06/04/11 Roger Healey - Add fn prototypes for doRqnpn, doError and thisNN

// mode values
#define DIRECT_BYTE 0
#define DIRECT_BIT 1
#define PAGED 2
#define REGISTER 3
#define ADDRESS 4
#define PWR_OFF 10
#define PWR_ON 11

void parse_cmd(void);
void doRqnpn(unsigned int);
void doError(unsigned int);
int thisNN(void);

#endif
