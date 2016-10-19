//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//
//	Pete Brownlow 26/6/11	Add transmit error trapping and retry at higher priority after lost arbitration
//							Set flags for 2 beeps after transmission failure

#include "project.h"

//
// Low priority interrupt. Used for CAN receive.
//
void isr_low(void) {
    // If FIFO watermark interrupt is signalled then we send a high
    // priority OPC_HLT to halt the CBUS. The packet has been preloaded
    // in TXB0

    /*
    if (PIR3bits.FIFOWMIF == 1) {
        TXB0CONbits.TXREQ = 1;
        op_flags.bus_off = 1;
        PIE3bits.FIFOWMIE = 0;
    }
    if (PIR3bits.ERRIF == 1) { 
		if (TXB1CONbits.TXLARB) {  // lost arbitration
			if (Latcount == 0) {	// already tried higher priority
				op_flags.can_transmit_failed = 1;
				if (BeepCount == 0) {
					BeepCount = 2;		// 2 beeps for CAN transmit failure

				}
				TXB1CONbits.TXREQ = 0;
			}				
			else if ( --Latcount == 0) {	// Allow tries at lower level priority first
				TXB1CONbits.TXREQ = 0;
				Tx1[sidh] &= 0b00111111;		// change priority	
				TXB1CONbits.TXREQ = 1;			// try again
			}
		}
		if (TXB1CONbits.TXERR) {	// bus error
			op_flags.can_transmit_failed = 1;
			if (BeepCount == 0) {
				BeepCount = 2;		// 2 beeps for CAN transmit failure			
			}
			TXB1CONbits.TXREQ = 0;
		}
 
    }
    */
	//PIR3 = 0;   // clear interrupts
}
