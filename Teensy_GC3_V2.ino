#include "project.h"

#include <font_Arial.h> // from ILI9341_t3
#include <font_ArialBold.h> // from ILI9341_t3
#include <font_ArialBlack.h> // from ILI9341_t3
#include <font_AwesomeF000.h>
#include <font_AwesomeF080.h>

#include <FlexCAN.h>
#include <EEPROM.h>

#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

#include "merg_logo.c"
#include "pjrc_logo.c"

// Use hardware SPI with SCK set to pin 14

// For optimized ILI9341_t3 library
#define TFT_DC      9
#define TFT_CS      10
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     11
#define TFT_SCLK    14
#define TFT_MISO    12
#define CS_PIN  8

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
XPT2046_Touchscreen ts(CS_PIN, 6);

//touchscreen x values go from 3700 (left hand side) to 350 (right hand side)
// touchscreen y values go from  3700 (top) to 400 (bottom)

#ifdef __MK66FX1M0__
  FlexCAN CANbus(125000, 2);
#else
  FlexCAN CANbus(125000, 0);
#endif


IntervalTimer dccBit;
IntervalTimer railcomDelay;
IntervalTimer railcomCh1Delay;
IntervalTimer railcomCh1Occ;
IntervalTimer railcomCh2Delay;
IntervalTimer railcomCh2Occ;

#define RAILCOM_SERIAL Serial1
//#define USB_SERIAL Serial

CAN_message_t Tx1, rx_ptr, TXB0;

//
// Current sensing for Teensy
// 5V reference => 3.3/4096 = 0.81mV resolution
// Sense resistor is 0R50
// so 60mA is 30mV Vsense => 37 steps
// 250mA overload is 125mV Vsense => 155 steps

//
#define I_ACK_DIFF 30  // No. steps for additional 60ma ACK pulse
#define I_OVERLOAD 155  //This is the value for 250mA for the service mode
#define I_DEFAULT 1500
#define I_LIMIT 2400    //This is for 4 amps (capability of L6203

// EEPROM addresses
#define EE_MAGIC 0
#define EE_MW 2
#define EE_IMAX 4
#define EE_ACTIVE_TIMEOUT 6
#define EE_INACTIVE_TIMEOUT 8
#define EE_DISPATCH_TIMEOUT 10

// values
#define MAGIC 93

#define BACKGROUND ILI9341_WHITE

//
// Flags register used for DCC packet transmission
//
volatile union dccflags_t{
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
volatile union modeword_t{
    struct {
        unsigned analog_en:1; //Option to run analog (PWM) output with address 0
        unsigned dispatch_active:1; //Option for special queue to handle dispatch of locos
        unsigned s_full:1;
        unsigned inactive_timeout:1;  //wether the inactive timeout feature is enabled
        unsigned active_timeout:1;  //wether the inactive timeout feature is enabled
        unsigned active_timeout_mode:1;  // Option for estop on active timeout or controlled stop
        unsigned direct_byte:1;
        unsigned railcom:1;
    } ;
    unsigned char byte;
} mode_word;

//
// OP_FLAGS for DCC output
//
volatile union opflags_t{
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
volatile union statflags_t{
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
unsigned int TouchTapTimer;
volatile boolean railCom_active;
volatile byte RailCom_CH1_data[2];
volatile byte RailCom_CH2_data[6];
volatile float ch1Current;
volatile float last_ch1Current;
boolean lastOverload;
boolean railcomDisplay_active;
boolean trackOffDisplay_active;
boolean trackOnDisplay_active;
unsigned int noOfSessions;
unsigned int last_noOfSessions;
uint16_t inactiveTimeout;
uint16_t activeTimeout;
uint16_t dispatchTimeout;
boolean analogOperationActive = 0;
boolean railcomEnabled = 0;
boolean SWAP_OP;  //status of the swap output function, basically turns the 1st booster into service mode
uint16_t ch1Current_readings[CIRCBUFFERSIZE];    //array for ring buffer of current readings
uint16_t ch2Current_readings[CIRCBUFFERSIZE];
unsigned char ch1Current_idx = 0;     //indexes for ring buffers
unsigned char ch2Current_idx = 0;


screenDisplay currentScreen = Splash;


boolean wastouched = true;

rectangle TRACK_STAT = {90, 160, 140, 60, 1};
rectangle RAIL_COM = {280, 120, 30, 30, 0};
rectangle CURRENT_FRAME = {7, 50, 306, 60, 1};
rectangle CURRENT_BOX = {188, 60, 85, 40, 1};
rectangle SESSIONS_BOX = {90, 10, 85, 40, 1};
rectangle SWAP_BOX = {10, 160, 60, 60, 1};
rectangle SETTINGS_BOX = {250, 160, 60, 60, 1};
rectangle RETURN_BOX = {250, 160, 60, 60, 0};
rectangle INACTIVE_TIMEOUT = {30, 70, 180, 40, 0};
rectangle ACTIVE_TIMEOUT = {30, 120, 180, 40, 0};
rectangle DISPATCH_TIMEOUT = {30, 170, 180, 40, 0};


rectangle OPTION_1_RADIO = {30, 100, 30, 60, 0};
rectangle OPTION_2_RADIO = {108, 100, 40, 40, 0};
rectangle OPTION_3_RADIO = {172, 100, 40, 40, 0};
rectangle OPTION_4_RADIO = {236, 100, 40, 40, 0};


// dcc packet buffers for service mode programming track
// and main track
volatile unsigned char dcc_buff_s[7];
volatile unsigned char dcc_buff_m[7];

// Module parameters at fixed place in ROM, also used by bootloader
const unsigned char params[7] = {MANU_MERG, MINOR_VER, MODULE_ID, EVT_NUM, EVperEVT, NV_NUM, MAJOR_VER};

unsigned char i;



void setup() {
  Serial.begin(9600);
  RAILCOM_SERIAL.begin(250000);
  analogReadResolution(12);
  
  //SPI.setSCK(14);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  //tft.setTextColor(ILI9341_YELLOW);
  tft.setRotation(3);
  //tft.setFont(Arial_16);
  //tft.setTextSize(1);
  //tft.println("Welcome to the Teensy GC3");
  ts.begin();

  
  //delay(1000);

  //tft.fillScreen(ILI9341_BLACK);
  
  //pinMode(SWAP_OP_HW, INPUT);
  //pinMode(PWRBUTTON, INPUT_PULLUP);
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
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(36, OUTPUT);
  digitalWrite(35, LOW);
  digitalWrite(36, LOW);

  //tft.setCursor(0, 0);
  //tft.println("IO Initialized");

  noInterrupts();

  PowerButtonDelay = 0;
  PowerTrigger = 0;
  PowerON = 0;
  TouchTapTimer = 0;
  
  //
  // setup initial values before enabling ports
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

  //tft.setCursor(0, 30);
  //tft.println("Operation Flags Initialized");

  digitalWriteFast(DCC_EN, 1);

  cbus_setup();

  //tft.setCursor(0, 60);
  //tft.println("CBUS Stack Initialized");

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
  //maybe use memset for faster clearing
  //memset(s_queue, 0, sizeof(s_queue));
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

  //tft.setCursor(0, 90);
  //tft.println("Queues Cleared");

  // clear the fifo receive buffers
  while (ecan_fifo_empty() == 0) {
      //rx_ptr->con = 0;
      CANbus.read(rx_ptr);
  }

  cmd_rmode();          // read mode & current limit
  // check for magic value and set defaults if not found
  if (EEPROM.read(EE_MAGIC) != 93)  {
      mode_word.byte = 0;
      mode_word.inactive_timeout = 1;
      mode_word.railcom = 1;
      imax = I_DEFAULT;
      inactiveTimeout = 240;  //Set the default inactive timeout for 120 seconds (2 minutes)
      activeTimeout = 600;  //set the active timeout to be 240 seconds (4 minutes)
      dispatchTimeout = 600;
      cmd_wmode();            // Save default
  }

  //set temporary staus bits after reading them from the EEPROM mode word
  railcomEnabled = mode_word.railcom;

  ad_state = 0;
  iccq = 0;

  // Start slot timeout timer
  slot_timer = 8620;  // Half second count down for 58uS interrupts = 500000/58

  dccBit.priority(16);  //Set interrupt priority for bit timing to 16 (second highest)
  SCB_SHPR3 = 0x20200000; //Change systick priority to 32 (3rd highest)
  dccBit.begin(isr_high, 58); //begin dcc timer with period of 58 us
  dccBit.priority(16);  //Set interrupt priority for bit timing to 16 (second highest)

  //tft.setCursor(0, 120);
  //tft.println("Bit Timer Initialized");
  

  // Programmer state machine
  prog_state = CV_IDLE;

  // Clear current sense averager
  ave = 0;
  sum = 0;

  // Setup ID
  NN_temp = DEFAULT_NN;

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

  //tft.setCursor(0, 150);
  //tft.println("Commencing Operation");
  splashScreen();
  //currentScreen = Main;
  delay(3000);

  //unsigned char i;
  LEDCanActTimer = 0;
  
  
  // Initial power off on main track
  op_flags.op_pwr_m = 0;

  for (i = 0; i < 5; i++) {
      Tx1.buf[0] = OPC_ARST;
      can_tx(1);
  }

  mode_word.analog_en = 1;
  mode_word.dispatch_active = 1;
  initScreenCurrent();
  currentScreen = Main;
  SWAP_OP = 0;
  swapButton();
  settingsButton();

}

void loop() {
  // put your main code here, to run repeatedly:

  boolean istouched = ts.touched();
  if (istouched) {
    TS_Point p = ts.getPoint();
    if ((!wastouched) && (TouchTapTimer == 0)) {
      switch (currentScreen) {
        case Splash:

        break;

        case Main:

          if ((buttonPressed(TRACK_STAT, p)) && TRACK_STAT.active) {
            if(stat_flags.track_on_off == 0 ) {
              power_control(OPC_RTON);
              //PowerON = 1;
              stat_flags.track_on_off = 1;
            }
            else {
              power_control(OPC_RTOF);
              //PowerON = 0;
              stat_flags.track_on_off = 0;
            }
          }

          if ((buttonPressed(SWAP_BOX, p)) && SWAP_BOX.active) {
            if(SWAP_OP == 0 ) {
              SWAP_OP = 1;
              swapButton();
            }
            else {
              SWAP_OP = 0;
              swapButton();
            }
          }
  
          if ((buttonPressed(SETTINGS_BOX, p)) && SETTINGS_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }

        break;

        case Overload:

        break;

        case Settings:

          if ((buttonPressed(INACTIVE_TIMEOUT, p)) && INACTIVE_TIMEOUT.active) {
            inactivePopup();
            currentScreen = InactiveTimeoutSplash;
            OPTION_1_RADIO.active = 1;
            OPTION_2_RADIO.active = 1;
            OPTION_3_RADIO.active = 1;
            OPTION_4_RADIO.active = 1;
          }

          if ((buttonPressed(ACTIVE_TIMEOUT, p)) && ACTIVE_TIMEOUT.active) {
            activePopup();
            currentScreen = ActiveTimeoutSplash;
            OPTION_1_RADIO.active = 1;
            OPTION_2_RADIO.active = 1;
            OPTION_3_RADIO.active = 1;
            OPTION_4_RADIO.active = 1;
          }

          if ((buttonPressed(DISPATCH_TIMEOUT, p)) && DISPATCH_TIMEOUT.active) {
            dispatchPopup();
            currentScreen = DispatchSplash;
            OPTION_1_RADIO.active = 1;
            OPTION_2_RADIO.active = 1;
            OPTION_3_RADIO.active = 1;
            OPTION_4_RADIO.active = 1;
          }

          else if ((buttonPressed(RAIL_COM, p)) && RAIL_COM.active) {
            if(railcomEnabled == 0) {
              railcom_control(OPC_RTON);
              //PowerON = 1;
              railcomEnabled = 1;
            }
            else {
              railcom_control(OPC_RTOF);
              //PowerON = 0;
              railcomEnabled = 0;
              
            }
          }

          if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            mainPage();
            currentScreen = Main;
          }

        break;

        case ActiveTimeoutSplash:

          if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
            OPTION_1_RADIO.active = 0;
            OPTION_2_RADIO.active = 0;
            OPTION_3_RADIO.active = 0;
            OPTION_4_RADIO.active = 0;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            activeTimeout = 0;
            mode_word.active_timeout = 0;
            ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_2_RADIO, p)) && OPTION_2_RADIO.active) {
            activeTimeout = 240;
            mode_word.active_timeout = 1;
            ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_3_RADIO, p)) && OPTION_3_RADIO.active) {
            activeTimeout = 600;
            mode_word.active_timeout = 1;
            ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_4_RADIO, p)) && OPTION_4_RADIO.active) {
            activeTimeout = 1200;
            mode_word.active_timeout = 1;
            ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
          }

        break;

        case InactiveTimeoutSplash:

          if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
            OPTION_1_RADIO.active = 0;
            OPTION_2_RADIO.active = 0;
            OPTION_3_RADIO.active = 0;
            OPTION_4_RADIO.active = 0;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            inactiveTimeout = 0;
            mode_word.inactive_timeout = 0;
            ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_2_RADIO, p)) && OPTION_2_RADIO.active) {
            inactiveTimeout = 240;
            mode_word.inactive_timeout = 1;
            ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_3_RADIO, p)) && OPTION_3_RADIO.active) {
            inactiveTimeout = 600;
            mode_word.inactive_timeout = 1;
            ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_4_RADIO, p)) && OPTION_4_RADIO.active) {
            inactiveTimeout = 1200;
            mode_word.inactive_timeout = 1;
            ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
          }

        break;

        case DispatchSplash:

          if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
            OPTION_1_RADIO.active = 0;
            OPTION_2_RADIO.active = 0;
            OPTION_3_RADIO.active = 0;
            OPTION_4_RADIO.active = 0;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            dispatchTimeout = 0;
            mode_word.dispatch_active = 0;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_2_RADIO, p)) && OPTION_2_RADIO.active) {
            dispatchTimeout = 240;
            mode_word.dispatch_active = 1;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_3_RADIO, p)) && OPTION_3_RADIO.active) {
            dispatchTimeout = 600;
            mode_word.dispatch_active = 1;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_4_RADIO, p)) && OPTION_4_RADIO.active) {
            dispatchTimeout = 1200;
            mode_word.dispatch_active = 1;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLUE);
          }

        break;

      }
      
      TouchTapTimer = 5000;
    }
  }
  wastouched = istouched;

    /*if( pwr && !PowerTrigger && (TouchTapTimer == 0) ) {
      PowerTrigger = 1;
    }
    else if( !pwr && PowerTrigger && (TouchTapTimer == 0)) {
      PowerTrigger = 0;
      TouchTapTimer = 10000;
      // Toggle Power.
      if( PowerON == 0 ) {
        power_control(OPC_RTON);
        PowerON = 1;
        //stat_flags.track_on_off = 0;
      }
      else {
        power_control(OPC_RTOF);
        PowerON = 0;
        //stat_flags.track_on_off = 1;
      }
    }
    */


      if (dcc_flags.dcc_overload) {
          // Programming overload
          dcc_flags.dcc_overload = 0;
    digitalWriteFast(OVERLOAD_PIN, 0);
      } else {
          if (SWAP_OP == 0) {
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

      //digitalWriteFast(LEDCANACT, q_queue[1].status.valid);

      // Handle slot & service mode timeout and beeps every half second
      if (op_flags.slot_timer) {
        //shouldnt need to deactivate interrupts as this will only run when it detects the slot timer bit which is triggered by the interrupt routine anyway
        //noInterrupts();
        ch1Current = (ave*0.0008);
        //interrupts();

        
        if(mode_word.inactive_timeout || mode_word.active_timeout || mode_word.dispatch_active) {
          for (i = 0; i < MAX_HANDLES; i++) {
            //decrement the inactive timeout if required
            if (((q_queue[i].speed & 0x7F) == 0) && (q_queue[i].timeout > 0) && (mode_word.inactive_timeout) && (inactiveTimeout > 0)) {
              q_queue[i].timeout = constrain(q_queue[i].timeout, 0, inactiveTimeout); //If using the inactive timeout then if the loco is stopped start the timer
              --q_queue[i].timeout;
              if ((q_queue[i].status.valid) && ((q_queue[i].timeout) < 40)) {
                q_queue[i].status.valid = 0;
                //rx_ptr.buf[1] = i;
                //purge_session();
              }
              if (q_queue[i].timeout == 0) {
                //q_queue[i].status.valid = 0;
                //rx_ptr.buf[1] = i;
                purge_session(i);
              }
            }
            
            //decrement the active timeout if required
            if (((q_queue[i].speed & 0x7F) != 0) && (q_queue[i].timeout > 0) && (mode_word.active_timeout) && (activeTimeout > 0)) {
              --q_queue[i].timeout;
              //If this is in the dispatch queue just put its timeout back to the active one
              if (d_queue[i].status.valid) {
                q_queue[i].timeout = activeTimeout;
              }            
              if (q_queue[i].timeout == 0) {
                //set loco speed to 0 then purge session
                rx_ptr.buf[0] = OPC_DSPD;
                rx_ptr.buf[1] = i;
                if (mode_word.active_timeout_mode) rx_ptr.buf[2] = 1; //If the active timeout mode bit is set to 1, do an estop
                else rx_ptr.buf[2] = 0;  //if not, do a controlled stop, not an estop
                queue_update();
                purge_session(i);
              }
            }
            
            //check the dispatch queue for timeouts
            if ((d_queue[i].status.valid) && (d_queue[i].timeout > 0) && (mode_word.dispatch_active)) {
              --d_queue[i].timeout;
                             
              if (d_queue[i].timeout == 0) {
                //set loco speed to 0 then purge session
                rx_ptr.buf[0] = OPC_DSPD;
                rx_ptr.buf[1] = i;
                rx_ptr.buf[2] = 0;  //if not, do a controlled stop, not an estop
                queue_update();
                rx_ptr.buf[1] = i;
                purge_session(i);
                purge_dispatch(i);  //remove session from dispatch
              }
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
  
  
        /*
        tft.fillRect(188, 60+50, 85, 40, ILI9341_WHITE);
        tft.setTextColor(ILI9341_BLACK);
        tft.setCursor(188 + 3, 60 + 50);
        tft.print(q_queue[1].timeout);
        */
        
        if((noOfSessions != last_noOfSessions) && (SESSIONS_BOX.active))
        {
          updateSessions();
        }
        if(digitalRead(OVERLOAD_PIN) && (!lastOverload)){
          overloadDisplay();
          lastOverload = 1;
        }
        if((!digitalRead(OVERLOAD_PIN)) && (lastOverload))
        {
          if (currentScreen == Main) {
            mainPage();
          }
          else if (currentScreen == Settings) {
            settingsPage();
          }
            
          lastOverload = 0;
        }
        if((ch1Current != last_ch1Current) && (!lastOverload) && (CURRENT_BOX.active))
        {
          updateScreenCurrent();
        }
        
        last_ch1Current = ch1Current;
    
        op_flags.slot_timer = 0;
      }  // slot timer flag set
  
  
}

FASTRUN void railComInit(){
   railcomDelay.end(); //stop the interval timer
   digitalWriteFast(START_PREAMBLE, 1);
   //digitalWriteFast(LEDCANACT, 1);
   railCom_active = 1;
   digitalWriteFast(DCC_OUT_POS, 1);
   digitalWriteFast(DCC_OUT_NEG, 1);
   if (SWAP_OP == 0) {
    digitalWriteFast(DCC_POS, 1);
    digitalWriteFast(DCC_NEG, 1);
   }
   railcomCh1Delay.begin(railComCh1Start, 47); //begin railcom channel 1 delay timer with period of 47 us
   railcomCh1Delay.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
   
}

FASTRUN void railComCh1Start(){
  digitalWriteFast(START_PREAMBLE, 0);
  railcomCh1Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh1Occ.begin(railComCh1End, 100); //begin railcom channel 1 occupied timer with period of 100 us
  railcomCh1Occ.priority(0);  //Set interrupt priority for bit timing to 16 (highest)
}

FASTRUN void railComCh1End(){
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

FASTRUN void railComCh2Start(){
  digitalWriteFast(START_PREAMBLE, 0);
  railcomCh2Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh2Occ.begin(railComCh2End, 263); //begin railcom channel 2 occupied timer with period of 263 us
  railcomCh2Occ.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
}

FASTRUN void railComCh2End(){
  digitalWriteFast(START_PREAMBLE, 0);
  railcomCh2Occ.end();
  railCom_active = 0;
  int i = 0;
  
  while(RAILCOM_SERIAL.available()){
    RailCom_CH2_data[i] = RAILCOM_SERIAL.read();
    i++;
  }
  /*
  if (op_flags.op_pwr_m) {
        digitalWriteFast(DCC_OUT_POS, op_flags.op_bit_m);
        digitalWriteFast(DCC_OUT_NEG, !op_flags.op_bit_m);
        digitalWriteFast(BOOSTER_OUT, 1);
      }
      //digitalWriteFast(LEDCANACT, 0);
      */
}




