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
#include <SD.h>

//#include "merg_logo.c"
//#include "pjrc_logo.c"

// For optimized ILI9341_t3 library
#define TFT_DC      9
#define TFT_CS      10
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     11
#define TFT_SCLK    13
#define TFT_MISO    12
#define TS_CS_PIN  8  //Touchscreen chip select
#define TS_IRQ  7 //Touchscreen Interrupt request

// Use hardware SPI
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);  //Use standard SPI hardware with SCK on pin 13
XPT2046_Touchscreen ts(TS_CS_PIN, TS_IRQ);  //Use pin 7 and 8 for the touchscreen interface along with the normal SPI

//touchscreen x values go from 3700 (left hand side) to 350 (right hand side)
// touchscreen y values go from  3700 (top) to 400 (bottom)

#ifdef __MK66FX1M0__
  FlexCAN CANbus(125000, 2);  //Primary MERG canbus on CAN0 with alternate pin mapping
  FlexCAN CANbus2(250000, 1); //Secondary CANbus at 250k on CAN1
#else
  FlexCAN CANbus(125000, 0);  //If using Teensy 3.2, then just use the normal CAN0 on pins 3 and 4
#endif


IntervalTimer dccBit;
IntervalTimer railcomDelay;
IntervalTimer railcomCh1Delay;
IntervalTimer railcomCh1Occ;
IntervalTimer railcomCh2Delay;
IntervalTimer railcomCh2Occ;

#define RAILCOM_SERIAL Serial1  //Serial 1 is reserved for the railcom interface
//#define USB_SERIAL Serial

CAN_message_t Tx1, rx_ptr, TXB0, USBtxmsg, USBrxmsg;;

//
// Current sensing for Teensy
// 5V reference => 3.3/4096 = 0.81mV resolution
// Sense resistor is 0R50
// so 60mA is 30mV Vsense => 37 steps
// 250mA overload is 125mV Vsense => 155 steps

//
#define I_ACK_DIFF 40  // No. steps for additional 60ma ACK pulse
#define I_OVERLOAD 165  //This is the value for 250mA for the service mode
#define I_DEFAULT 2600
#define I_LIMIT 4000    //This is for 4 amps (capability of L6203

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
#define DOT_COLOUR ILI9341_BLACK

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
boolean sdCardPresent = 0;

String inputString, outputString;
unsigned int tempcanid1, tempcanid2;

const char ASCII_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};


screenDisplay currentScreen = Splash;


boolean wastouched = true;

rectangle TRACK_STAT = {90, 165, 140, 60, 1};
rectangle RAIL_COM = {280, 120, 30, 30, 0};
rectangle CURRENT_FRAME = {7, 50, 306, 60, 1};
rectangle CURRENT_BOX = {188, 60, 85, 40, 1};
rectangle SESSIONS_BOX = {90, 10, 85, 40, 1};
rectangle SWAP_BOX = {10, 165, 60, 60, 1};
rectangle SETTINGS_BOX = {250, 165, 60, 60, 1};
rectangle RETURN_BOX = {240, 170, 60, 60, 0};
rectangle INACTIVE_TIMEOUT = {10, 50, 180, 40, 0};
rectangle ACTIVE_TIMEOUT = {10, 100, 180, 40, 0};
rectangle DISPATCH_TIMEOUT = {10, 150, 180, 40, 0};
rectangle ANALOG_SETTING = {200, 50, 110, 40, 0};
rectangle CURRENT_SETTING = {200, 100, 110, 40, 0};
rectangle SPLASH_BOX = {10, 20, 300, 160, 0};
rectangle RAILCOM_SETTING = {10, 200, 180, 30, 0};

rectangle OPTION_1_RADIO = {30, 100, 30, 60, 0};
rectangle OPTION_2_RADIO = {118, 100, 40, 40, 0};
rectangle OPTION_3_RADIO = {182, 100, 40, 40, 0};
rectangle OPTION_4_RADIO = {246, 100, 40, 40, 0};


// dcc packet buffers for service mode programming track
// and main track
volatile unsigned char dcc_buff_s[7];
volatile unsigned char dcc_buff_m[7];

// Module parameters at fixed place in ROM, also used by bootloader
const unsigned char params[7] = {MANU_MERG, MINOR_VER, MODULE_ID, EVT_NUM, EVperEVT, NV_NUM, MAJOR_VER};

unsigned char i;



void setup() {
  Serial.begin(250000); //USB serial for use with JMRI
  RAILCOM_SERIAL.begin(250000); //Serial interface for RailCom
  analogReadResolution(12); //Set the analog inputs to 12 bit (4095) resolution
  analogWriteFrequency(20, 16000); // Set the PWM frequency to 20kHz when in analog mode.


  //Set up TFT
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  ts.begin();

  //Pin setups
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
  
  //Disable interrupts during setup
  noInterrupts();

  //Initialising
  PowerButtonDelay = 0;
  PowerTrigger = 0;
  PowerON = 0;
  TouchTapTimer = 0;
  
  //Ensure the strings for use with the CAN to Serial are big enough
  inputString.reserve(36);
  outputString.reserve(36);
  
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

  digitalWriteFast(DCC_EN, 1);

  cbus_setup();

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

  // clear the fifo receive buffers
  //while (ecan_fifo_empty() == 0) {
  while (CANbus.available()) {
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

  tft.setTextColor(ILI9341_BLACK);
  tft.fillScreen(BACKGROUND);

  //built in sd card initialisation
  /*
  while (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("failed to access SD card!"));
    tft.println(F("failed to access SD card!"));
    delay(2000);
  }
  */
  #ifdef __MK66FX1M0__

  if (SD.begin(BUILTIN_SDCARD)) {
    bmpDraw("splash.bmp", 0, 0);
    //bmpDraw("flowers.bmp", 0, 0);
    sdCardPresent = 1;
  } else {
    tft.println(F("failed to access SD card!"));
  }
  delay(2000);
  #endif
  delay(2000);

  

  //Turn on CAN transceiver 0
  //digitalWrite(2, 0);
  
  // enable interrupts
  interrupts();

  //unsigned char i;
  LEDCanActTimer = 0;
  
  
  // Initial power off on main track
  op_flags.op_pwr_m = 0;

  for (i = 0; i < 5; i++) {
      Tx1.buf[0] = OPC_ARST;
      can_tx(1);
  }

  if (railcomEnabled) {
    railcom_control(OPC_RCON);
  } else {
    railcom_control(OPC_RCOF);
  }

  //mode_word.analog_en = 1;
  //mode_word.dispatch_active = 1;
  initScreenCurrent();
  currentScreen = Main;
  SWAP_OP = 0;

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

          if ((buttonPressed(SESSIONS_BOX, p)) && (noOfSessions > 0)) {
            //Purge all active sessions
            purge_allSessions();
          }
  
          if ((buttonPressed(SETTINGS_BOX, p)) && SETTINGS_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }

        break;

        case Overload:

        break;

        case Settings:

          if (buttonPressed(RETURN_BOX, p)) {
            mainPage();
            currentScreen = Main;
          }

          if (buttonPressed(INACTIVE_TIMEOUT, p)) {
            inactivePopup();
            currentScreen = InactiveTimeoutSplash;
          }

          if (buttonPressed(ACTIVE_TIMEOUT, p)){
            activePopup();
            currentScreen = ActiveTimeoutSplash;
          }

          if (buttonPressed(DISPATCH_TIMEOUT, p)) {
            dispatchPopup();
            currentScreen = DispatchSplash;
          }

          if (buttonPressed(ANALOG_SETTING, p)) {
            analogPopup();
            currentScreen = AnalogSplash;
          }

          if (buttonPressed(CURRENT_SETTING, p)) {
            currentPopup();
            currentScreen = CurrentSplash;
          }

          if (buttonPressed(RAILCOM_SETTING, p)) {
            railcomPopup();
            currentScreen = RailcomSplash;
          }

          /*if ((buttonPressed(RAIL_COM, p)) && RAIL_COM.active) {
            if(railcomEnabled == 0) {
              railcom_control(OPC_RCON);
              railcomEnabled = 1;
            }
            else {
              railcom_control(OPC_RCOF);
              railcomEnabled = 0;
            }
          }*/

        break;

        case ActiveTimeoutSplash:

          /*
          if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }
          */

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            activeTimeout = 0;
            mode_word.active_timeout = 0;
            ee_write_short(EE_ACTIVE_TIMEOUT, activeTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
          }

        break;

        case InactiveTimeoutSplash:

          /*if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }*/

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            inactiveTimeout = 0;
            mode_word.inactive_timeout = 0;
            ee_write_short(EE_INACTIVE_TIMEOUT, inactiveTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
          }

        break;

        case DispatchSplash:

          /*if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }*/

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            dispatchTimeout = 0;
            mode_word.dispatch_active = 0;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
          }

        break;

        case AnalogSplash:

          /*if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }*/

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_2_RADIO, p)) && OPTION_2_RADIO.active) {
            mode_word.analog_en = 0;
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_4_RADIO, p)) && OPTION_4_RADIO.active) {
            mode_word.analog_en = 1;
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
          }

        break;

        case CurrentSplash:

          /*if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }*/

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_1_RADIO, p)) && OPTION_1_RADIO.active) {
            dispatchTimeout = 0;
            mode_word.dispatch_active = 0;
            ee_write_short(EE_DISPATCH_TIMEOUT, dispatchTimeout);
            EEPROM.update(EE_MW, mode_word.byte);
            tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLACK);
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
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLACK);
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
            tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLACK);
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
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_BLACK);
          }

        break;

        case RailcomSplash:

          /*if ((buttonPressed(RETURN_BOX, p)) && RETURN_BOX.active) {
            settingsPage();
            currentScreen = Settings;
          }*/

          if (!buttonPressed(SPLASH_BOX, p)) {
            settingsPage();
            currentScreen = Settings;
          }

          if ((buttonPressed(OPTION_2_RADIO, p)) && OPTION_2_RADIO.active) {
            railcom_control(OPC_RCOF);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
          }

          if ((buttonPressed(OPTION_4_RADIO, p)) && OPTION_4_RADIO.active) {
            railcom_control(OPC_RCON);
            tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, ILI9341_WHITE);
            tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
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
      //if (ecan_fifo_empty() == 0) {
      if (CANbus.available()) {
          CANbus.read(rx_ptr);
          CAN2Serial(rx_ptr);
          // Decode the new command
          LEDCanActTimer = 2000;
          digitalWriteFast(LEDCANACT, 1);
          parse_cmd();
      }

      //Chekc if there is serial data available off the USB port, if it is and it is a valid gridconnect packet, parse it like any other command and send a copy of it out on the network
      if (CheckSerial()) {
        rx_ptr.id = USBtxmsg.id;
        rx_ptr.len = USBtxmsg.len;
        rx_ptr.ext = USBtxmsg.ext;
        rx_ptr.rtr = USBtxmsg.rtr;
        rx_ptr.buf[0] = USBtxmsg.buf[0];
        rx_ptr.buf[1] = USBtxmsg.buf[1];
        rx_ptr.buf[2] = USBtxmsg.buf[2];
        rx_ptr.buf[3] = USBtxmsg.buf[3];
        rx_ptr.buf[4] = USBtxmsg.buf[4];
        rx_ptr.buf[5] = USBtxmsg.buf[5];
        rx_ptr.buf[6] = USBtxmsg.buf[6];
        rx_ptr.buf[7] = USBtxmsg.buf[7];
        parse_cmd();
        //CANbus.write(USBtxmsg);
      }


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
   //digitalWriteFast(START_PREAMBLE, 1);
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
  //digitalWriteFast(START_PREAMBLE, 0);
  railcomCh1Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh1Occ.begin(railComCh1End, 100); //begin railcom channel 1 occupied timer with period of 100 us
  railcomCh1Occ.priority(0);  //Set interrupt priority for bit timing to 16 (highest)
}

FASTRUN void railComCh1End(){
  //digitalWriteFast(START_PREAMBLE, 1);
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
  //digitalWriteFast(START_PREAMBLE, 0);
  railcomCh2Delay.end();
  RAILCOM_SERIAL.clear();
  railcomCh2Occ.begin(railComCh2End, 263); //begin railcom channel 2 occupied timer with period of 263 us
  railcomCh2Occ.priority(0);  //Set interrupt priority for bit timing to 0 (highest)
}

FASTRUN void railComCh2End(){
  //digitalWriteFast(START_PREAMBLE, 0);
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






