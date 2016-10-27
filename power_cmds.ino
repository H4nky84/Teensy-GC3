//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

#include "project.h"

//
// power_control()
//
// Turn track on or off.
//
void power_control(unsigned char cmd) {
    if (cmd == OPC_RTOF) {
        // Turn off main track power
        op_flags.op_pwr_m = 0;

        // Acknowledge it
        Tx1.buf[0] = OPC_TOF;
        trackOffMessage();
        railcom_control(OPC_RTOF);
    } else  {
        // Turn on main track power
        op_flags.op_pwr_m = 1;

        // Acknowledge it
        Tx1.buf[0] = OPC_TON;
        trackOnMessage();
        railcom_control(OPC_RTON);
    }
    can_tx(1);
}

void railcom_control(unsigned char cmd) {
    if (cmd == OPC_RTOF) {
        // Turn off main track power
        mode_word.railcom = 0;

        // Acknowledge it
        Tx1.buf[0] = OPC_TOF;
    } else  {
        // Turn on main track power
        mode_word.railcom = 1;

        // Acknowledge it
        Tx1.buf[0] = OPC_TON;
        
    }
    can_tx(1);
    railComIcon();
}

//
// Power On Cycle
//
// RP923 calls for at least 20 valid packets with a check for overload of 250mA
// sustained after 100ms. 14 reset packets gives us approx 105ms, after which the
// decoder quiescent current is logged and checked for overload. We then enable
// gross overload checking in the A/D routines and a further 6 reset packets
// complete the power on cycle.
//
// [1.5] Add another 15 reset packets for QSI V6 steam decoders
//
//void power_on_cycle() {
//  iccq = 0;
//  // [rev r] initialize current measurement
//  sum = 0;
//  ave = 0;
//  op_flags.op_pwr_s = 1;
//  dcc_flags.dcc_overload = 0;
//  dcc_flags.dcc_check_ovld = 0;
//  dcc_flags.dcc_check_ack = 0;
//  dcc_flags.dcc_ack = 0;
//  packet_reset(14);
//  iccq = ave;
//  if (iccq > I_OVERLOAD) {
//    op_flags.op_pwr_s = 0;
//    dcc_flags.dcc_overload = 1;
//    return;
//  }
//  dcc_flags.dcc_check_ovld = 1;
//  dcc_flags.dcc_check_ack = 1;
////  packet_reset(6);
//  packet_reset(21);
//  // [rev r] wait until all reset packets complete before final sampling of ICCQ
//  while (dcc_flags.dcc_rdy_s == 0)
//    ;
//  iccq = ave;
//}

//
// power_off_cycle()
//
//void power_off_cycle(void) {
//  recovery_time();
//  op_flags.op_pwr_s = 0;
//}

//
// packet_reset(unsigned char i)
//
// Place i reset packets in the DCC transmit buffer
//
void packet_reset(unsigned char i) {
    while (dcc_flags.dcc_rdy_s == 0)
      ;
    dcc_buff_s[0] = dcc_buff_s[1] = dcc_buff_s[2] = 0;
    dcc_buff_s[6] = i;            // repeat
    dcc_bytes_s = 3;
    dcc_pre_s = LONG_PRE;
    dcc_flags.dcc_rdy_s = 0;
}

//
// Place an idle packet in the DCC transmit buffer
//
void packet_idle(unsigned char i) {
      while (dcc_flags.dcc_rdy_s == 0)
        ;
      dcc_buff_s[0] = dcc_buff_s[2] = 0xff;
      dcc_buff_s[1] = 0;
      dcc_buff_s[6] = i;        // repeat
      dcc_bytes_s = 3;
      dcc_pre_s = LONG_PRE;
      dcc_flags.dcc_rdy_s = 0;
}

