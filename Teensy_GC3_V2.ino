#include <FlexCAN.h>
#include <EEPROM.h>
#include "project.h"

FlexCAN CANbus(125000, 0);  //Declare canbus object at 125kbit

//Interval Timers to control precise timing of railcom cutout
IntervalTimer dccBit;
IntervalTimer railcomDelay;
IntervalTimer railcomCh1Delay;
IntervalTimer railcomCh1Occ;
IntervalTimer railcomCh2Delay;
IntervalTimer railcomCh2Occ;

#define RAILCOM_SERIAL Serial1

//CAN message buffers
CAN_message_t Tx1, rx_ptr, TXB0;

//
// Current sensing for Teensy
// 5V reference => 3.3/4096 = 0.81mV resolution
// Sense resistor is 0R50
// so 60mA is 30mV Vsense => 37 steps
// 250mA overload is 125mV Vsense => 155 steps

//
#define I_ACK_DIFF 37  // No. steps for additional 60ma ACK pulse
#define I_OVERLOAD 155  //This is the value for 250mA for the service mode
#define I_DEFAULT 1500
#define I_LIMIT 2900    //This is for 4 amps (capability of L6203

// EEPROM addresses
#define EE_MAGIC 0
#define EE_MW 2
#define EE_IMAX 3

// values
#define MAGIC 93

//
// Flags register used for DCC packet transmission
//
volatile union {
    struct {
        unsigned dcc_rdy_s:1;        // set if Tx ready for a new packet
        unsigned dcc_long_pre:1;  // set forces long preamble
        unsigned dcc_retry:1;
        unsigned dcc_ack:1;
        unsigned dcc_overload:1;  // set if overload detected
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
volatile union {
    struct {
        unsigned boot_en:1;
        unsigned :1;
        unsigned s_full:1;
        unsigned :1;
        unsigned :1;
        unsigned ztc_mode:1;  // ZTC compatibility mode
        unsigned direct_byte:1;
        unsigned railcom:1;
    } ;
    unsigned char byte;
} mode_word;

//
// OP_FLAGS for DCC output
//
volatile union {
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
} op_flags;

//
// FLAGS for STAT output
//
volatile union {
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
} stat_flags;

//
//
//
volatile unsigned char ovld_delay;
//extern volatile unsigned char dcc_buff_s[7];
//extern volatile unsigned char dcc_buff_m[7];
volatile uint16_t imax;    // Booster mode current limit
unsigned char ad_state;             // A/D state machine
volatile uint16_t iccq;               // Quiescent decoder current
volatile unsigned char tmr0_reload;
unsigned char Latcount;
unsigned char BeepCount;
unsigned char PowerButtonDelay;
unsigned char can_transmit_timeout;
uint16_t NN_temp;
unsigned short LEDCanActTimer;
unsigned char PowerTrigger;
unsigned char PowerON;
unsigned short PowerButtonTimer;
//extern const unsigned char params[7];
volatile boolean railCom_active;
volatile byte RailCom_CH1_data[2];
volatile byte RailCom_CH2_data[6];


// dcc packet buffers for service mode programming track
// and main track
volatile unsigned char dcc_buff_s[7];
volatile unsigned char dcc_buff_m[7];

// Module parameters at fixed place in ROM, also used by bootloader
const unsigned char params[7] = {MANU_MERG, MINOR_VER, MODULE_ID, EVT_NUM, EVperEVT, NV_NUM, MAJOR_VER};

void setup() {
  pinMode(SWAP_OP, INPUT);
  pinMode(PWRBUTTON, INPUT_PULLUP);
  pinMode(LEDCANACT, OUTPUT);
  pinMode(DCC_EN, OUTPUT);
  pinMode(DCC_OUT_POS, OUTPUT);
  pinMode(DCC_OUT_NEG, OUTPUT);
  pinMode(DCC_POS, OUTPUT);
  pinMode(DCC_NEG, OUTPUT);
  pinMode(AWD, OUTPUT);
  pinMode(OVERLOAD_PIN, OUTPUT);
  pinMode(BOOSTER_OUT, OUTPUT);
  pinMode(START_PREAMBLE, OUTPUT);
  
  unsigned char i;

  noInterrupts();

  RAILCOM_SERIAL.begin(250000);
  
  PowerButtonDelay = 0;
  PowerTrigger = 0;
  PowerON = 0;
  PowerButtonTimer = 0;
  
  //
  // setup initial values before enabling ports
  // Port A are analogue
  //
  op_flags.op_pwr_s = 0;
  op_flags.op_pwr_m = 0;
  op_flags.bus_off = 0;
  op_flags.can_transmit_failed = 0;
  op_flags.beeping = 0;
  can_transmit_timeout = 0;
  BeepCount = 0;
  retry_delay = 0;
  stat_flags.byte = 0;

  // Setup ports
  //TRISBbits.TRISB4 = 0; /* CAN activity */
  //TRISBbits.TRISB1 = 0; /* RUN indicator */
  //TRISBbits.TRISB0 = 0; /* Internal booster/PT */
  //TRISBbits.TRISB7 = 1; /* Power button */

  digitalWriteFast(DCC_EN, 1);

  cbus_setup();
  //PIE3 = 0b00100001;      // CAN TX error and FIFOWM interrupts
  //RCONbits.IPEN = 1;      // enable interrupt priority levels

  ovld_delay = 0;
  bit_flag_s = 6;         // idle state
  bit_flag_m = 6;         // idle state
  dcc_flags.word = 0;
  dcc_flags.dcc_rdy_m = 1;
  dcc_flags.dcc_rdy_s = 1;

  // Clear the refresh queue
  for (i = 0; i < MAX_HANDLES; i++) {
      q_queue[i].status.byte = 0;
      q_queue[i].address.addr_int = 0;
      q_queue[i].speed = 0x80;
      q_queue[i].fn1 = 0;
      q_queue[i].fn2 = 0;
      q_queue[i].fn2a = 0;
      q_queue[i].timeout = 0;
  }
  q_idx = 0;
  q_state = 0;

  // Clear the send queue
  for (i = 0; i < 16; i++) {
      s_queue[i].status.byte = 0;
      s_queue[i].d[0] = 0;
      s_queue[i].d[1] = 0;
      s_queue[i].d[2] = 0;
      s_queue[i].d[3] = 0;
      s_queue[i].d[4] = 0;
      s_queue[i].d[5] = 0;
      s_queue[i].repeat = 0;
  }
  s_head = 0;
  s_tail = 0;

  // clear the fifo receive buffers
  while (ecan_fifo_empty() == 0) {
      //rx_ptr->con = 0;
      CANbus.read(rx_ptr);
  }

  mode_word.byte = 0;

  cmd_rmode();          // read mode & current limit
  // check for magic value and set defaults if not found
  if (EEPROM.read(EE_MAGIC) != 93)  {
      mode_word.byte = 0;
      imax = I_DEFAULT;
      cmd_wmode();            // Save default
  }

  ad_state = 0;
  iccq = 0;

  // Start slot timeout timer
  slot_timer = ((long)500000)/58;  // Half second count down for 58uS interrupts

  // Set up TMR0 for DCC bit timing with 58us period prescaler 4:1,


  dccBit.priority(16);  //Set interrupt priority for bit timing to 16 (second highest)
  SCB_SHPR3 = 0x20200000; //Change systick priority to 32 (3rd highest)
  dccBit.begin(isr_high, 58); //begin dcc timer with period of 58 us
  dccBit.priority(16);  //Set interrupt priority for bit timing to 16 (second highest)
  

  // Programmer state machine
  prog_state = CV_IDLE;

  // Clear current sense averager
  ave = 0;
  sum = 0;

  // Setup ID
  NN_temp = DEFAULT_NN;
  //Tx1[con] = 0;
  //Tx1[sidh] = 0b10110000 | (FIXED_CAN_ID & 0x78) >>3;
  //Tx1[sidl] = (FIXED_CAN_ID & 0x07) << 5;

  Tx1.id = (FIXED_CAN_ID & 0x7F) | 0b10110000000;
  Tx1.rtr = 0;
  Tx1.ext = 0;

  // Setup TXB0 with high priority OPC_HLT
  TXB0.id = (FIXED_CAN_ID & 0x7F) | 0b1110000000;
  TXB0.rtr = 0;
  TXB0.ext = 0;
  TXB0.len = 1;
  TXB0.buf[0] = OPC_HLT;
  

  //Turn on CAN transceiver 0
  digitalWrite(2, 0);
  
  // enable interrupts
  interrupts();

}

void loop() {
  // put your main code here, to run repeatedly:
    unsigned char i;
    LEDCanActTimer = 0;
    
    
    // Initial power off on main track
    op_flags.op_pwr_m = 0;

    for (i = 0; i < 5; i++) {
        Tx1.buf[0] = OPC_ARST;
        can_tx(1);
    }
    
    //Temporary code to enable power on automatically and enabling railCom cutout
    power_control(OPC_RTON);
    PowerON = 0;
    mode_word.railcom = 1;

    // Loop forever
    while (1) {

      unsigned char pwr = digitalRead(PWRBUTTON);
      pwr = !pwr; // Input is inverted.

      if( pwr && !PowerTrigger && (PowerButtonTimer == 0) ) {
        PowerTrigger = 1;
      }
      else if( !pwr && PowerTrigger && (PowerButtonTimer == 0)) {
        PowerTrigger = 0;
        PowerButtonTimer = 10000;
        // Toggle Power.
        if( PowerON == 1 ) {
          power_control(OPC_RTON);
          PowerON = 0;
          //stat_flags.track_on_off = 0;
        }
        else {
          power_control(OPC_RTOF);
          PowerON = 1;
          //stat_flags.track_on_off = 1;
        }
      }


        if (dcc_flags.dcc_overload) {
            // Programming overload
            dcc_flags.dcc_overload = 0;
      digitalWriteFast(OVERLOAD_PIN, 0);
        } else {
            if (digitalRead(SWAP_OP) == 0) {
                // Low power booster mode
          if (dcc_flags.dcc_retry) {
                    // Turn power back on after retry
            dcc_flags.dcc_retry = 0;
                    op_flags.op_pwr_m = 1;
          digitalWriteFast(OVERLOAD_PIN, 0);
          }
            }
    }

        if (((dcc_flags.dcc_reading) || (dcc_flags.dcc_writing))
            && (dcc_flags.dcc_rdy_s == 1)) {
            // iterate service mode state machine
            cv_sm();
        }

        if (dcc_flags.dcc_rdy_m) {
            // Main track output is ready for next packet
            packet_gen();
        }

        // Check for Rx packet and setup pointer to it
        if (ecan_fifo_empty() == 0) {
            // Decode the new command
            LEDCanActTimer = 2000;
            digitalWriteFast(LEDCANACT, 1);
            parse_cmd();
        }

        // Handle slot & service mode timeout and beeps every half second
        if (op_flags.slot_timer) {
            for (i = 0; i < MAX_HANDLES; i++) {
                if (q_queue[i].status.valid && q_queue[i].speed == 0) {
                    if (--q_queue[i].timeout == 0) {
                        q_queue[i].status.valid = 0;
                    }
                }
            }

      if (BeepCount > 0) {
        op_flags.beeping = !op_flags.beeping;
        digitalWriteFast(AWD, (op_flags.beeping || (retry_delay > 0)));
        if (op_flags.beeping) {
          BeepCount--;
        }
      }
      else {
        op_flags.beeping = 0;
          if (retry_delay == 0) {
          digitalWriteFast(AWD, 0);
        }
      }
            op_flags.slot_timer = 0;
        }  // slot timer flag set
    }


}

void railComInit(){
   railcomDelay.end(); //stop the interval timer
   digitalWriteFast(START_PREAMBLE, 1);
   railCom_active = 1;
   digitalWriteFast(DCC_OUT_POS, 1);
   digitalWriteFast(DCC_OUT_NEG, 1);
   railcomCh1Delay.begin(railComCh1Start, 47); //begin railcom channel 1 delay timer with period of 47 us
   railcomCh1Delay.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
   
}

void railComCh1Start(){
  digitalWriteFast(START_PREAMBLE, 0);
  railcomCh1Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh1Occ.begin(railComCh1End, 100); //begin railcom channel 1 occupied timer with period of 100 us
  railcomCh1Occ.priority(0);  //Set interrupt priority for bit timing to 16 (highest)
}

void railComCh1End(){
  digitalWriteFast(START_PREAMBLE, 1);
  railcomCh1Occ.end();
  railcomCh2Delay.begin(railComCh2Start, 7); //begin railcom channel 2 delay timer with period of 7 us
  railcomCh2Delay.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
  int i = 0;
  while(RAILCOM_SERIAL.available()){
    RailCom_CH1_data[i] = RAILCOM_SERIAL.read();
    i++;
  }
}

void railComCh2Start(){
  digitalWriteFast(START_PREAMBLE, 0);
  railcomCh2Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh2Occ.begin(railComCh2End, 263); //begin railcom channel 2 occupied timer with period of 263 us
  railcomCh2Occ.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
}

void railComCh2End(){
  digitalWriteFast(START_PREAMBLE, 1);
  railcomCh2Occ.end();
  railCom_active = 0;
  int i = 0;
  while(RAILCOM_SERIAL.available()){
    RailCom_CH2_data[i] = RAILCOM_SERIAL.read();
    i++;
  }
  if (op_flags.op_pwr_m) {
        digitalWriteFast(DCC_OUT_POS, op_flags.op_bit_m);
        digitalWriteFast(DCC_OUT_NEG, !op_flags.op_bit_m);
        digitalWriteFast(BOOSTER_OUT, 1);
      }
}

