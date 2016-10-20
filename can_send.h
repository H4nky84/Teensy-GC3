#ifndef __CAN_SEND_H
#define __CAN_SEND_H

extern void can_opc(unsigned char opc);
extern void can_tx_nn(unsigned char dlc_val);
extern void can_tx(unsigned char dlc_val);
void can_bus_on(void);
void can_debug1(unsigned char status);
void can_debug_dcc(void);
void sendTX1(void);


#endif
