//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

#include "project.h"

// local function prototypes
void cv_sm(void);
void program_packet(unsigned char cmd, unsigned char addr, unsigned char data);
void print_ack(void);

//#pragma udata access VARS
unsigned char sm_handle;
unsigned int cv;
unsigned char cv_register;
unsigned char cv_page;
unsigned char mode;
unsigned char cv_data;

unsigned char prog_state;
unsigned char state_count;
unsigned char cv_cmd, val;

//#pragma code APP

//
// cmd_cv()
//
// Read/Program CV in service mode or program in ops mode
//
void cmd_cv(void) {
    // Remove offset from CV address
    cv = (((unsigned int)rx_ptr.buf[2])<<8 | rx_ptr.buf[3]) - 1;
    cv_data = rx_ptr.buf[5];
    if ((rx_ptr.buf[0] == OPC_WCVO) || (rx_ptr.buf[0] == OPC_WCVB)) {
        // Add ops mode write packet to send queue
        cv_data = rx_ptr.buf[4];
        ops_write();
    } else {
        // Service mode programming is split into a state machine which
        // is called regularly from the main loop. We must save all the
        // data from the Rx packet before returning.
        sm_handle = rx_ptr.buf[1];                     // save session handle

		// *** what does this do >>>
        //power_off_cycle();
  		recovery_time();
  		op_flags.op_pwr_s = 0;


        mode = rx_ptr.buf[4];
        if (cv < 1024) {
            if ((dcc_flags.dcc_reading == 0) && (dcc_flags.dcc_writing == 0)) {
                if (rx_ptr.buf[0] == OPC_QCVS) {
                    // Start CV read
                    dcc_flags.dcc_reading = 1;
                } else if (rx_ptr.buf[0] == OPC_WCVS) {
                    // Start CV write
                    dcc_flags.dcc_writing = 1;;
                }
                // Start the service mode state machine
                cv_sm();
                return;
            } else {
                // Already programming
                Tx1.buf[2] = SSTAT_BUSY;
            }
        } else {
            // CV Error
            Tx1.buf[2] = SSTAT_CV_ERROR;
        }
        // Send status
        Tx1.buf[1] = sm_handle;
        Tx1.buf[0] = OPC_SSTAT;
        can_tx(3);
    }
}

//
// get_page_reg()
//
// Calculate the Page and Register ofsets for paged mode
//
// Using RP-923 algorithm
//	Subtract 1 from CV address
//	Divide result by 4
//	Remainder is the Register address 0 - 3
//	Add 1 to the quotient to give the Page number
// e.g. CV #65 - 1 = 64
// 	64 divided by 4 = 16, 0 remainder.
// 	Register 0, Page 17
//
void get_page_reg(void) {
    // cv already has 1 subtracted
    cv_register = cv & 0x3;
    cv_page = (cv>>2) & 0xFF;
    cv_page++;
}

//
// cv_read_sm()
//
// Service mode operation is controlled by a state machine that sends
// packets to the programming track. Each call of this function sends one
// distinct packet (maybe with a repeat value and then returns so that
// main() loop can continue handling CAN traffic.
//
void cv_sm(void) {
    switch (prog_state) {
        case CV_IDLE:
            // Idle state wait for something to do
            if ((dcc_flags.dcc_reading) || (dcc_flags.dcc_writing)){
                iccq = 0;
				sum = 0;
				ave = 0;
				retry_delay = 0;
				ovld_delay = 0;
                op_flags.op_pwr_s = 1;
                dcc_flags.dcc_overload = 0;
                dcc_flags.dcc_check_ovld = 0;
                dcc_flags.dcc_check_ack = 0;
                dcc_flags.dcc_ack = 0;
                dcc_flags.dcc_cv_no_ack = 0;
                dcc_flags.dcc_rec_time = 0;
				digitalWriteFast(OVERLOAD_PIN, 0);
				// ACK_PIN = 0;
				//DCC_PKT_PIN = 0;
                // send reset packets for 250ms delay after power up
                packet_reset(14);
                prog_state = CV_PON1;
                break;
            }

        case CV_PON1:
			// Check reset packets have been sent
            	// 250ms power up elapsed, check for overload
	            iccq = ave;
	            if (iccq > I_OVERLOAD) {
	                op_flags.op_pwr_s = 0;
	                dcc_flags.dcc_overload = 1;
					digitalWriteFast(OVERLOAD_PIN, 1);
	                print_ack();
	                dcc_flags.dcc_reading = 0;
	                dcc_flags.dcc_writing = 0;
	                prog_state = CV_IDLE;
	                break;
	            }
	            // continue power up time
	            dcc_flags.dcc_check_ovld = 1;
	            dcc_flags.dcc_check_ack = 1;
                // reset spare pin from ack detected (for scope monitoring)
				// ACK_PIN = 0;
	            // Longer delay required for QSI locos
	            packet_reset(21);
	            prog_state = CV_CHECK_OVLD;
            break;

        // Power up complete
        case CV_CHECK_OVLD:
        	// Check for overload every time
            if (dcc_flags.dcc_overload == 1) {
                print_ack();
                dcc_flags.dcc_reading = 0;
                dcc_flags.dcc_writing = 0;
                prog_state = CV_IDLE;
			// Else check reset packets have completed
            } else if ((mode == PAGED) || (mode == REGISTER)) {
	                // Load the page reg
	                if (mode == REGISTER) {
	                    cv_page = 1;
	                    cv_register = cv & 0x7;
	                } else {
	                    get_page_reg();
	                }
	                program_packet(0x7d, 0, cv_page);
	                prog_state = CV_PAGE_SENT;
	            } else if (mode == DIRECT_BIT) {
	                if (dcc_flags.dcc_reading) {
	                    // Direct bit read
	                    // 1st byte 011110AA - bit manipulation
	                    cv_cmd = 0x78 | (cv>>8);
	                    // First verify against 1
	                    // 3rd byte 111KDBBB - verify bit 7 == 1
	                    cv_data = 0xef;
	                    val = 0;
	                    state_count = 8;
	                    program_packet(cv_cmd, cv&0xff, cv_data);
	                    prog_state = CV_READ_BIT_V_1;
	                } else {
	                    // Direct bit write
	// For now force "direct bit mode" to be direct bit read
	// and direct byte write as per SPROG
	// due to problems with direct bit write in QSI
	//                    val = cv_data;                          // Save value to be written
	//                    cv_cmd = 0x78 | (cv>>8);		        // 1st byte 011110AA - bit manipulation
	//                    cv_data = 0xf7;					        // 3rd byte 111KDBBB - write bit 7 == 0
	//                    state_count = 8;
	//                    if (val & 0x80) {                       // Test MSB
	//                        cv_data = cv_data | 0x08;           // write a 1
	//                    } else {
	//                        cv_data = cv_data & 0xf7;           // write a 0
	//                    }
	//                    program_packet(cv_cmd, cv&0xff, cv_data);	// 8 Program Packets
	//                    prog_state = CV_WRITE_BIT_REC;
	                    cv_cmd = 0x7c | (cv>>8);		            // 1st byte 011111AA - write
	                    program_packet(cv_cmd, cv, cv_data);	    // 8 Program Packets
	                    prog_state = CV_REC;
	                }
	            } else if (mode == DIRECT_BYTE) {
	                if (dcc_flags.dcc_reading) {
	                    cv_cmd = 0x74 | (cv>>8);		                // 1st byte 011101AA - bit manipulation
	                    cv_data = 0;
	                    program_packet(cv_cmd, cv&0xff, cv_data++);
	                    prog_state = CV_READ;
	                } else {
	                    cv_cmd = 0x7c | (cv>>8);		            // 1st byte 011111AA - write
	                    program_packet(cv_cmd, cv, cv_data);	    // 8 Program Packets
	                    prog_state = CV_REC;
	            }
			}
            break;

        // Start recovery time after write bit
        case CV_WRITE_BIT_REC:
            recovery_time();
            prog_state = CV_WRITE_BIT;
            break;

        // Write next bit
        case CV_WRITE_BIT:
            dcc_flags.dcc_rec_time = 0;
            state_count--;
            if (state_count == 0) {
                recovery_time();
                prog_state = CV_POFF;
                break;
            }
            // Adjust for next bit
            val = val<<1;
            cv_data--;
            if (val & 0x80) {
                cv_data = cv_data | 0x08;           // write a 1
            } else {
                cv_data = cv_data & 0xf7;           // write a 0
            }
            program_packet(cv_cmd, cv&0xff, cv_data);
            prog_state = CV_WRITE_BIT_REC;
            break;

        // Verifying against 1
        case CV_READ_BIT_V_1:
            val = val<<1;
            if (dcc_flags.dcc_ack == 1) {
                // ACK so bit == 1, set bit 0 by incrementing
                val++;
            }
            // Start recovery time
            recovery_time();
            prog_state = CV_READ_BIT_1_REC;
            break;

        // Recovery time complete, verify next bit
        case CV_READ_BIT_1_REC:
            dcc_flags.dcc_rec_time = 0;
            cv_data--;
            state_count--;
            if ((state_count == 0) && (val == 0)) {
                // No ACK at all could be a problem so verify against 0
                // 3rd byte 111KDBBB - verify bit 7 == 0
                cv_data = 0xe7;
                state_count = 8;
                program_packet(cv_cmd, cv&0xff, cv_data);
                prog_state = CV_READ_BIT_V_0;
            } else if (state_count == 0) {
                cv_data = val;
                recovery_time();
                prog_state = CV_POFF;
            } else {
                program_packet(cv_cmd, cv&0xff, cv_data);
                prog_state = CV_READ_BIT_V_1;
            }
            break;

        // Verifying against 0
        case CV_READ_BIT_V_0:
            val = val<<1;
            if (dcc_flags.dcc_ack == 0) {
                // No ACK so bit == 1, set bit 0 by incrementing
                val++;
            }
            // Start recovery time
            recovery_time();
            prog_state = CV_READ_BIT_0_REC;
            break;

        // Recovery time complete, verify next bit
        case CV_READ_BIT_0_REC:
            dcc_flags.dcc_rec_time = 0;
            cv_data--;
            state_count--;
            if ((state_count == 0) && (val == 0)) {
                // Genuine 0 this time
                cv_data = 0;
                recovery_time();
                prog_state = CV_POFF;
            } else if (state_count == 0) {
                dcc_flags.dcc_cv_no_ack = 1;
                recovery_time();
                prog_state = CV_POFF;
            } else {
                program_packet(cv_cmd, cv&0xff, cv_data);
                prog_state = CV_READ_BIT_V_0;
            }
            break;

        // Page mode start recovery time after writing page reg
        case CV_PAGE_SENT:
            recovery_time();
            prog_state = CV_PAGE_REC;
            break;

        // page mode recovery time complete after page reg write
        case CV_PAGE_REC:
            dcc_flags.dcc_rec_time = 0;
            dcc_flags.dcc_ack = 0;
            if (dcc_flags.dcc_reading) {
                // 1st byte 0111CRRR - verify
                cv_cmd = 0x70 | cv_register;
                cv_data = 0;
                program_packet(cv_cmd, 0, cv_data++);
                prog_state = CV_READ;
            } else {
                // Paged or register write
                // 1st byte 0111CRRR - write
                cv_cmd = 0x78 | cv_register;
                // addr parameter is ignored for paged mode
                program_packet(cv_cmd, 0, cv_data);
                prog_state = CV_REC;
            }
            break;

        case CV_READ:
            // Verify next value
            if ((cv_data != 0) && (dcc_flags.dcc_ack == 0)) {
                // cv address will be ignored by program_packet
                // for paged mode
                program_packet(cv_cmd, cv&0xff, cv_data++);
                prog_state = CV_READ;
            } else {
                if (cv_data == 0) {
                    // No value matched
                    dcc_flags.dcc_cv_no_ack = 1;
                }
                cv_data--;
                recovery_time();
                prog_state = CV_POFF;
            }
            break;

        // Recovery time
        case CV_REC:
            recovery_time();
            prog_state = CV_POFF;
            break;

        // Power off and send ack message
        case CV_POFF:
            dcc_flags.dcc_rec_time = 0;
            op_flags.op_pwr_s = 0;
            print_ack();
            dcc_flags.dcc_reading = 0;
            dcc_flags.dcc_writing = 0;
            prog_state = CV_IDLE;
            break;

        default:
            break;
    }
}

//
// program_packet(cmd, addr, data)
//
// Generate service mode programming packets
//
// Direct Mode Program/Read/Verify packets are 4 bytes of the form
// 	long-preamble 0 0111CCAA 0 AAAAAAAA 0 DDDDDDDD 0 EEEEEEEE 1
//
// Page Mode Program/Read packets are 3 bytes of the form
//	long-preamble 0 0111CRRR 0 DDDDDDDD 0 EEEEEEEE 1
//
// Register Mode Program/Read packets are 3 bytes of the form
//	long-preamble 0 0111CRRR 0 DDDDDDDD 0 EEEEEEEE 1
//
// addr parameter is ignored for paged mode packets
//
// Packet is sent up to 8 times
//
// PNB 16/4/11 - use constant for LONG_PRE instead of hard coded value of 20

void program_packet(unsigned char cmd, unsigned char addr, unsigned char data) {
    dcc_flags.dcc_ack = 0;
    dcc_pre_s = LONG_PRE;					// long preamble
    dcc_buff_s[0] = cmd;					// 1st byte
    dcc_buff_s[3] = cmd ^ data;			    // partial error byte
    dcc_bytes_s = 3;
    if ((mode == DIRECT_BYTE) || (mode == DIRECT_BIT)) {
      dcc_buff_s[1] = addr;
      dcc_buff_s[2] = data;
      dcc_buff_s[3] = dcc_buff_s[3] ^ addr;
      dcc_bytes_s++;
    } else {
      dcc_buff_s[1] = data;
      dcc_buff_s[2] = dcc_buff_s[3];		// move error byte
    }
    dcc_buff_s[6] = 8;                      // Repeat
    dcc_flags.dcc_rdy_s = 0;                // hand over buffer
}

// *** Use a different Tx buffer for this in case main code 
// is trying to use Tx1 
void print_ack() {
    cv++;                               // restore offset
    Tx1.buf[0] = OPC_SSTAT;                // speculative
    Tx1.buf[1] = sm_handle;                // handle
    if (dcc_flags.dcc_cv_no_ack == 1) {
        // No acknowledge received during reads
        Tx1.buf[2] = SSTAT_NO_ACK;
    } else if (dcc_flags.dcc_overload == 1) {
        // Overload
        Tx1.buf[2] = SSTAT_OVLD;
    } else {
        if (dcc_flags.dcc_reading) {
            // Read ack
            Tx1.buf[0] = OPC_PCVS;         // Report CV
            Tx1.buf[2] = cv>>8;            // cvmsb
            Tx1.buf[3] = cv & 0xff;        // cvlsb
            Tx1.buf[4] = cv_data;          // data
            can_tx(5);
            return;
        } else {
            if (dcc_flags.dcc_ack == 1) {
                // Write ack
                Tx1.buf[2] = SSTAT_WR_ACK;
            } else {
                Tx1.buf[2] = SSTAT_NO_ACK;
            }
        }
    }
    can_tx(3);
}

