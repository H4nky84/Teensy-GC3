#ifndef __POWER_CMDS_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

// Decoder Recovery Time
//
// RP923 calls for the same service mode write packet or reset packet to be sent
// until the specified packet time or acknowledge. Here we use reset packets. The
// worst case requirement is 10 packets.
//
#define recovery_time() dcc_flags.dcc_ack=0;dcc_flags.dcc_rec_time=1;packet_reset(10)

void power_control(unsigned char cmd);
//extern void power_on_cycle(void);
//extern void power_off_cycle(void);
extern void packet_reset(unsigned char i);
extern void packet_idle(unsigned char i);

#define __POWER_CMDS_H
#endif
