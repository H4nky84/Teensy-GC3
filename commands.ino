//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

// 06/04/11 Roger Healey - Modify code for OPC_BOOT
//						 - add thisNN routine
//						 - add code for OPC_RQNPN
//						 - add doError for command error response
#include "project.h"

void cmd_cv(void);

//
// parse_cmd()
//
// Decode the OPC and call the function to handle it.
//
void parse_cmd(void) {
    mode_word.s_full = 0;
    switch(rx_ptr.buf[0]) {
        case OPC_DSPD:
        case OPC_DFUN:
            // Loco sped/dir or functions
            queue_update();
            break;

        case OPC_QNN:
          Tx1.buf[0]  = OPC_PNN;
          Tx1.buf[1]  = (NN_temp / 256) & 0xFF;
          Tx1.buf[2]  = (NN_temp % 256) & 0xFF;
          Tx1.buf[3]  = params[0];
          Tx1.buf[4]  = params[2];
          Tx1.buf[5]  = 0;
          can_tx(6);
          break;

        case OPC_KEEP:
            // Session keep alive message
            keep_alive();
            break;

        case OPC_QCVS:
        case OPC_WCVS:
        case OPC_WCVO:
        case OPC_WCVB:
            // CV programming
            cmd_cv();
            break;

        case OPC_RLOC:
            // Request session for loco
            queue_add();
            break;

        case OPC_STMOD:
            throttle_mode();
            break;

        case OPC_KLOC:
            // Release engine by session number
            if ((mode_word.dispatch_active) && ((q_queue[rx_ptr.buf[1]].speed & 0x7F) != 0)) {  //if the special dispatch mode is activated, dont purge the session, put it into the special array of internal controls for the timeout period
              d_queue_add(rx_ptr.buf[1]);
              break;
            }
            else purge_session();
            break;

        case OPC_QLOC:
            // Query engine by session number
            query_session();
            break;

        case OPC_RDCC3:
        case OPC_RDCC4:
        case OPC_RDCC5:
        case OPC_RDCC6:
            // Send DCC packet
            dcc_packet();
            break;

        case OPC_RTOF:
        case OPC_RTON:
            // Track power control
            PowerON = ((rx_ptr.buf[0] == OPC_RTOF) ? 1:0);
            power_control(rx_ptr.buf[0]);
            break;

        case OPC_RESTP:
            // Emergency stop all
            // Set all sessions to emergency stop
            em_stop();
            // Send track stopped in response
            Tx1.buf[0] = OPC_ESTOP;
            can_tx(1);
            break;

        case OPC_QCON:
            // We only support advanced consisting so query consist
            // is not supported
            // Can't give an address so send 0
            Tx1.buf[0] = OPC_ERR;
            Tx1.buf[1] = 0;
            Tx1.buf[2] = 0;
            Tx1.buf[3] = ERR_NO_MORE_ENGINES;
            can_tx(4);
            break;

        case OPC_PCON:
            // Add to consist
            consist_add();
            break;
        
        case OPC_RSTAT:
            //Collect flags
            stat_flags.byte = 0;
            stat_flags.sm_on_off = !SWAP_OP;
            stat_flags.track_on_off = op_flags.op_pwr_m;
            stat_flags.em_stop = dcc_flags.dcc_em_stop;

            // Send status
            Tx1.buf[0] = OPC_STAT;
            Tx1.buf[1]  = NN_temp>>8;
            Tx1.buf[2]  = NN_temp&0xFF;
            Tx1.buf[3] = 0;
            Tx1.buf[4] = stat_flags.byte;
            Tx1.buf[5] = MAJOR_VER;
            Tx1.buf[6] = MINOR_VER;
            Tx1.buf[7] = 0;
            can_tx(8);
            break;
            
        case OPC_RQNPN:
        	// Request to read a parameter
            if (thisNN() == 1)
	        {
		        doRqnpn((unsigned int)rx_ptr.buf[3]);
		    }
		    break;

        default: break;
    }
    if (mode_word.s_full == 0) {
        // No problem so retire RX buffer
        //Nop();
        //rx_ptr->con = 0;
        if (op_flags.bus_off) {
            // At least one buffer is now free
            op_flags.bus_off = 0;
            //PIE3bits.FIFOWMIE = 1;
            can_bus_on();
        }
    } else {
        // Couldn't allocate in S queue so keep buffer valid and try
        // again next time round
        ;
    }
}

void doRqnpn(unsigned int idx)
{
	if (idx < 8)
	{
		Tx1.buf[0] = OPC_PARAN;
		Tx1.buf[3] = idx;
		Tx1.buf[4] = params[idx-1];
		can_tx_nn(5);
	}
	else
	{
	doError(CMDERR_INV_PARAM_IDX);
	}
}

void doError(unsigned int code)
{
		Tx1.buf[0] = OPC_CMDERR;
		Tx1.buf[3] = code;
		can_tx_nn(4);
}

int thisNN()
{
 	if ((((unsigned short)(rx_ptr.buf[1])<<8) + rx_ptr.buf[2]) == NN_temp)
	 	return 1;
	 else
	 	return 0;

}	
