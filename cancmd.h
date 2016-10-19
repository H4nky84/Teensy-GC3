#ifndef __CANCMD_H
#define __CANCMD_H

//
// CANCMD Programmer/Command Station (C) 2009 SPROG DCC
//	web:	http://www.sprog-dcc.co.uk
//	e-mail:	sprog@sprog-dcc.co.uk
//

// define SWAP_OUTPUTS to put main track output through on board
// booster, purely for testing, not suitable for normal use since
// programming track, through external booster, will have no current
// sensing feedback
//#define SWAP_OUTPUTS

//
// Current sensing for Teensy
// 5V reference => 3.3/4096 = 0.81mV resolution
// Sense resistor is 0R50
// so 60mA is 30mV Vsense => 37 steps
// 250mA overload is 125mV Vsense => 155 steps

// 06/04/11 Roger Healey - Add external reference to params
//
#define I_ACK_DIFF 37	// No. steps for additional 60ma ACK pulse
#define I_OVERLOAD 155
#define I_DEFAULT 500
#define I_LIMIT 3500

// EEPROM addresses
#define EE_MAGIC 0
#define EE_MW 2
#define EE_IMAX 3

// values
#define MAGIC 93

/*
//
// Flags register used for DCC packet transmission
//
extern volatile union {
    struct {
        unsigned dcc_rdy_s:1;		    // set if Tx ready for a new packet
        unsigned dcc_long_pre:1;	// set forces long preamble
        unsigned dcc_retry:1;
        unsigned dcc_ack:1;
        unsigned dcc_overload:1;	// set if overload detected
        unsigned dcc_check_ack:1;
        unsigned dcc_check_ovld:1;
        unsigned :1;
        unsigned dcc_rdy_m:1;
        unsigned dcc_reading:1;
        unsigned dcc_writing:1;
        unsigned dcc_cv_no_ack:1;
        unsigned dcc_rec_time:1;
        unsigned :1;
        unsigned dcc_em_stop:1;
        unsigned :1;
    } ;
    unsigned int word;
} dcc_flags;

//
// MODE_WORD flags
//
extern volatile union {
    struct {
        unsigned boot_en:1;
        unsigned :1;
        unsigned s_full:1;
        unsigned :1;
        unsigned :1;
        unsigned ztc_mode:1;	// ZTC compatibility mode
        unsigned direct_byte:1;
        unsigned :1;
    } ;
    unsigned char byte;
}mode_word;

//
// OP_FLAGS for DCC output
//
extern volatile union {
    struct {
        unsigned op_pwr_s:1;
        unsigned op_bit_s:1;
        unsigned op_pwr_m:1;
        unsigned op_bit_m:1;
        unsigned bus_off:1;
        unsigned slot_timer:1;
        unsigned can_transmit_failed:1;
        unsigned beeping:1;
    } ;
    unsigned char byte;
}op_flags;

//
// FLAGS for STAT output
//
extern volatile union {
    struct {
        unsigned hw_err:1;
        unsigned track_err:1;
        unsigned track_on_off:1;
        unsigned op_bit_m:1;
        unsigned bus_on:1;
        unsigned em_stop:1;
        unsigned sm_on_off:1;
        unsigned res:1;
    } ;
    unsigned char byte;
}stat_flags;

*/
volatile extern unsigned char ovld_delay;
extern volatile unsigned char dcc_buff_s[7];
extern volatile unsigned char dcc_buff_m[7];
volatile extern uint16_t imax;
extern unsigned char ad_state;
extern uint16_t iccq;
extern unsigned char BeepCount;
extern unsigned char PowerButtonDelay;
extern unsigned char can_transmit_timeout;
extern volatile unsigned char tmr0_reload;
extern const unsigned char params[7];
extern unsigned short LEDCanActTimer;
extern unsigned short PowerButtonTimer;
extern unsigned char PowerON;


#endif	// __SPROG_H
