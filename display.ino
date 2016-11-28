#include <ILI9341_t3.h>
//#include <Adafruit_STMPE610.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3700
#define TS_MINY 3700
#define TS_MAXX 300
#define TS_MAXY 400

//#define TS_MINX 3650
//#define TS_MINY 3790
//#define TS_MAXX 260
//#define TS_MAXY 400

#define BUFFPIXEL 80

boolean RecordOn = false;

void trackOffMessage()
{
  tft.fillRoundRect(TRACK_STAT.X+2, TRACK_STAT.Y+2, TRACK_STAT.W, TRACK_STAT.H, 8, BACKGROUND);
  tft.fillRoundRect(TRACK_STAT.X, TRACK_STAT.Y, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_RED);
  tft.drawRoundRect(TRACK_STAT.X, TRACK_STAT.Y, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_BLACK);
  tft.setCursor(TRACK_STAT.X+6, TRACK_STAT.Y+22);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_16);
  tft.println("TRACK OFF");
  trackOnDisplay_active = 0;
  trackOffDisplay_active = 1;
}

void trackOnMessage()
{
  tft.fillRoundRect(TRACK_STAT.X+2, TRACK_STAT.Y+2, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_BLACK);
  tft.fillRoundRect(TRACK_STAT.X, TRACK_STAT.Y, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_DARKGREEN);
  tft.drawRoundRect(TRACK_STAT.X, TRACK_STAT.Y, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_BLACK);
  tft.setCursor(TRACK_STAT.X+10, TRACK_STAT.Y+22);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_16);
  tft.println("TRACK ON");
  trackOnDisplay_active = 1;
  trackOffDisplay_active = 0;
}


void initScreenCurrent(){
  tft.fillScreen(BACKGROUND);
  tft.fillRoundRect(CURRENT_FRAME.X, CURRENT_FRAME.Y, CURRENT_FRAME.W, CURRENT_FRAME.H, 10, ILI9341_BLACK);
  tft.setCursor(CURRENT_FRAME.X + 8, CURRENT_FRAME.Y + 15);
  tft.setTextColor(ILI9341_CYAN);
  tft.setFont(Arial_28);
  tft.print("Current = ");
  tft.print(ch1Current);
  tft.print(" A");
  
  if(op_flags.op_pwr_m == 1)
  {
    trackOnMessage();
  } else trackOffMessage();
  
  railComIcon();
  swapButton();
  settingsButton();
  tft.setCursor(SESSIONS_BOX.X, SESSIONS_BOX.Y);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_16);
  tft.print("Sessions = ");
  tft.print(noOfSessions);
}

void splashScreen(){
  tft.fillScreen(BACKGROUND);
  tft.writeRect(38, 10, 244, 110, (uint16_t*)merg_logo);
  tft.setCursor(27, 150);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(ArialBlack_28);
  tft.print("Powered by:");
  tft.writeRect(22, 200, 276, 31, (uint16_t*)pjrc_logo);
}

void updateScreenCurrent(){
  tft.fillRect(CURRENT_BOX.X, CURRENT_BOX.Y, CURRENT_BOX.W, CURRENT_BOX.H, ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setFont(Arial_28);
  tft.setCursor(CURRENT_BOX.X + 3, CURRENT_BOX.Y + 5);
  //tft.print(an0);
  tft.print(ch1Current);
  tft.print(" A");
  railComIcon();
  //analogIcon();
}


void railComIcon()
{
  if(railcomEnabled){
    tft.fillRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_DARKGREEN);
    tft.drawRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_BLACK);
    tft.setCursor(RAIL_COM.X+7, RAIL_COM.Y+7);
    tft.setTextColor(ILI9341_WHITE);
    tft.setFont(Arial_16);
    tft.println("R");
    railcomDisplay_active = 1;
  }
  if(analogOperationActive){
    tft.fillRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_ORANGE);
    tft.drawRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_BLACK);
    tft.setCursor(RAIL_COM.X+7, RAIL_COM.Y+7);
    tft.setTextColor(ILI9341_WHITE);
    tft.setFont(Arial_16);
    tft.println("A");
  }
  if ((!railcomEnabled) && (!analogOperationActive)){
    tft.fillRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, BACKGROUND);
    railcomDisplay_active = 0;
  }
  
}

void swapButton()
{
  if(SWAP_OP){
  tft.fillRoundRect(SWAP_BOX.X+2, SWAP_BOX.Y+2, SWAP_BOX.W, SWAP_BOX.H, 4, ILI9341_BLACK);
  tft.fillRoundRect(SWAP_BOX.X, SWAP_BOX.Y, SWAP_BOX.W, SWAP_BOX.H, 4, ILI9341_LIGHTGREY);
  tft.drawRoundRect(SWAP_BOX.X, SWAP_BOX.Y, SWAP_BOX.W, SWAP_BOX.H, 4, ILI9341_BLACK);
  tft.setCursor(SWAP_BOX.X+7, SWAP_BOX.Y+7);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_16);
  tft.println("S");
  tft.setCursor(SWAP_BOX.X+5, SWAP_BOX.Y+2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(AwesomeF080_40);
  tft.print((char)45);
  tft.setCursor(SWAP_BOX.X+3, SWAP_BOX.Y);
  tft.setTextColor(ILI9341_DARKGREY);
  //tft.setFont(AwesomeF080_40);
  tft.print((char)45);
  } else{
    tft.fillRoundRect(SWAP_BOX.X+2, SWAP_BOX.Y+2, SWAP_BOX.W, SWAP_BOX.H, 4, BACKGROUND);
    tft.fillRoundRect(SWAP_BOX.X, SWAP_BOX.Y, SWAP_BOX.W, SWAP_BOX.H, 4, ILI9341_WHITE);
    tft.drawRoundRect(SWAP_BOX.X, SWAP_BOX.Y, SWAP_BOX.W, SWAP_BOX.H, 4, ILI9341_BLACK);
    tft.setCursor(SWAP_BOX.X+3, SWAP_BOX.Y);
    tft.setTextColor(ILI9341_LIGHTGREY);
    tft.setFont(AwesomeF080_40);
    tft.print((char)45);
    
  }
  
}

void settingsButton()
{
  //tft.fillRoundRect(SETTINGS_BOX.X+2, SETTINGS_BOX.Y+2, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_BLACK);
  tft.fillRoundRect(SETTINGS_BOX.X, SETTINGS_BOX.Y, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_WHITE);
  tft.drawRoundRect(SETTINGS_BOX.X, SETTINGS_BOX.Y, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_BLACK);
  tft.setCursor(SETTINGS_BOX.X+8, SETTINGS_BOX.Y+8);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(AwesomeF080_32);
  tft.print((char)5);
  tft.setCursor(SETTINGS_BOX.X+7, SETTINGS_BOX.Y+7);
  tft.setTextColor(ILI9341_DARKGREY);
  //tft.setFont(AwesomeF080_32);
  tft.print((char)5);
}

void returnButton()
{
  //tft.fillRoundRect(SETTINGS_BOX.X+2, SETTINGS_BOX.Y+2, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_BLACK);
  tft.fillRoundRect(RETURN_BOX.X, RETURN_BOX.Y, RETURN_BOX.W, RETURN_BOX.H, 4, ILI9341_WHITE);
  tft.drawRoundRect(RETURN_BOX.X, RETURN_BOX.Y, RETURN_BOX.W, RETURN_BOX.H, 4, ILI9341_BLACK);
  tft.setCursor(RETURN_BOX.X+3, RETURN_BOX.Y+3);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(AwesomeF000_40);
  tft.print((char)100);
  tft.setCursor(RETURN_BOX.X+2, RETURN_BOX.Y+2);
  tft.setTextColor(ILI9341_DARKGREY);
  //tft.setFont(AwesomeF000_40);
  tft.print((char)100);
}

void overloadDisplay(){
  tft.fillScreen(BACKGROUND);
  tft.fillRoundRect(7, 50, 306, 60, 10, ILI9341_BLACK);
  tft.drawRoundRect(10, 50, 300, 60, 10, ILI9341_BLACK);
  tft.setCursor(15, 65);
  tft.setTextColor(ILI9341_RED);
  tft.setFont(Arial_32);
  tft.print("OVERLOAD!");
  tft.setCursor(120, 140);
  tft.setTextColor(ILI9341_RED);
  tft.setFont(AwesomeF000_60);
  tft.print((char)113);
}

void updateSessions(){
  tft.fillRect(SESSIONS_BOX.X + 112, SESSIONS_BOX.Y, 40, 18, BACKGROUND);
  tft.setCursor(SESSIONS_BOX.X+114, SESSIONS_BOX.Y); //horizontal is original + 114
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_16);
  tft.print(noOfSessions);
  last_noOfSessions = noOfSessions;
}

void analogIcon()
{
  if(analogOperationActive){
  tft.fillRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_ORANGE);
  tft.drawRoundRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, 4, ILI9341_BLACK);
  tft.setCursor(RAIL_COM.X+7, RAIL_COM.Y+7);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_16);
  tft.println("A");
  } else{
    tft.fillRect(RAIL_COM.X, RAIL_COM.Y, RAIL_COM.W, RAIL_COM.H, BACKGROUND);
    railcomDisplay_active = 0;
  }
  
}

TS_Point scaleTouch(TS_Point point, unsigned minX, unsigned maxX, unsigned minY, unsigned maxY) {
  TS_Point temp = point;
  temp.x = map(point.x, minX, maxX, 0, 320);
  temp.y = map(point.y, minY, maxY, 0, 240);
  return temp;
}

boolean pressed(unsigned int numx, unsigned int numy, unsigned int lowerx, unsigned int upperx, unsigned int lowery, unsigned int uppery) {
  if ((lowerx < numx) && (numx < upperx) && (lowery < numy) && (numy < uppery)) {
    return 1;
  } else return 0; 
}

int buttonPressed(rectangle box, TS_Point touch) {
  TS_Point tempTouch;
  tempTouch = scaleTouch(touch, TS_MINX, TS_MAXX, TS_MINY, TS_MAXY);
  if ((box.X < tempTouch.x) && (tempTouch.x < (box.X + box.W)) && (box.Y < tempTouch.y) && (tempTouch.y < (box.Y + box.H))) {
    return 1;
  } else return 0; 
}

void settingsPage()
{
  tft.fillScreen(BACKGROUND);
  tft.setCursor(90, 10);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_28);
  tft.print("Settings");
  tft.drawRoundRect(INACTIVE_TIMEOUT.X, INACTIVE_TIMEOUT.Y, INACTIVE_TIMEOUT.W, INACTIVE_TIMEOUT.H, 8, ILI9341_BLACK);
  tft.setCursor(INACTIVE_TIMEOUT.X+10, INACTIVE_TIMEOUT.Y+10);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_16);
  tft.println("Inactive Timeout");
  tft.drawRoundRect(ACTIVE_TIMEOUT.X, ACTIVE_TIMEOUT.Y, ACTIVE_TIMEOUT.W, ACTIVE_TIMEOUT.H, 8, ILI9341_BLACK);
  tft.setCursor(ACTIVE_TIMEOUT.X+17, ACTIVE_TIMEOUT.Y+10);
  tft.println("Active Timeout");
  tft.drawRoundRect(DISPATCH_TIMEOUT.X, DISPATCH_TIMEOUT.Y, DISPATCH_TIMEOUT.W, DISPATCH_TIMEOUT.H, 8, ILI9341_BLACK);
  tft.setCursor(DISPATCH_TIMEOUT.X+5, DISPATCH_TIMEOUT.Y+10);
  tft.println("Dispatch Timeout");
  tft.drawRoundRect(RAILCOM_SETTING.X, RAILCOM_SETTING.Y, RAILCOM_SETTING.W, RAILCOM_SETTING.H, 8, ILI9341_BLACK);
  tft.setCursor(RAILCOM_SETTING.X+20, RAILCOM_SETTING.Y+7);
  tft.println("Railcom");
  tft.drawRoundRect(ANALOG_SETTING.X, ANALOG_SETTING.Y, ANALOG_SETTING.W, ANALOG_SETTING.H, 8, ILI9341_BLACK);
  tft.setCursor(ANALOG_SETTING.X+20, ANALOG_SETTING.Y+10);
  tft.println("Analog");
  tft.drawRoundRect(CURRENT_SETTING.X, CURRENT_SETTING.Y, CURRENT_SETTING.W, CURRENT_SETTING.H, 8, ILI9341_BLACK);
  tft.setCursor(CURRENT_SETTING.X+20, CURRENT_SETTING.Y+10);
  tft.println("Current");
  deactivateBoxes();
  RAIL_COM.active = 1;
  INACTIVE_TIMEOUT.active = 1;
  ACTIVE_TIMEOUT.active = 1;
  DISPATCH_TIMEOUT.active = 1;
  ANALOG_SETTING.active = 1;
  CURRENT_SETTING.active = 1;
  
  returnButton();
  //railComIcon();
  //RETURN_BOX.active = 1;
}

void backgroundSettingsPage()
{
  tft.fillScreen(ILI9341_LIGHTGREY);
  tft.setCursor(90, 10);
  tft.setTextColor(ILI9341_DARKGREY);
  tft.setFont(Arial_28);
  tft.print("Settings");
  tft.setFont(Arial_16);
  tft.drawRoundRect(DISPATCH_TIMEOUT.X, DISPATCH_TIMEOUT.Y, DISPATCH_TIMEOUT.W, DISPATCH_TIMEOUT.H, 8, ILI9341_DARKGREY);
  tft.setCursor(DISPATCH_TIMEOUT.X+5, DISPATCH_TIMEOUT.Y+10);
  tft.println("Dispatch Timeout");
  tft.drawRoundRect(RAILCOM_SETTING.X, RAILCOM_SETTING.Y, RAILCOM_SETTING.W, RAILCOM_SETTING.H, 8, ILI9341_DARKGREY);
  tft.setCursor(RAILCOM_SETTING.X+20, RAILCOM_SETTING.Y+7);
  tft.println("Railcom");
  
  tft.fillRoundRect(RETURN_BOX.X, RETURN_BOX.Y, RETURN_BOX.W, RETURN_BOX.H, 4, ILI9341_LIGHTGREY);
  tft.drawRoundRect(RETURN_BOX.X, RETURN_BOX.Y, RETURN_BOX.W, RETURN_BOX.H, 4, ILI9341_DARKGREY);
  tft.setCursor(RETURN_BOX.X+2, RETURN_BOX.Y+2);
  tft.setTextColor(ILI9341_DARKGREY);
  tft.setFont(AwesomeF000_40);
  tft.print((char)100);
}

void mainPage()
{
  tft.fillScreen(BACKGROUND);
  deactivateBoxes();
  RETURN_BOX.active = 0;
  CURRENT_FRAME.active = 1;
  CURRENT_BOX.active = 1;
  SESSIONS_BOX.active = 1;
  TRACK_STAT.active = 1;
  SETTINGS_BOX.active = 1;
  SWAP_BOX.active = 1;
  initScreenCurrent();
  swapButton();
  settingsButton();
  
}

void inactivePopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(40, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Inactive Timeout");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);
  tft.setCursor(OPTION_1_RADIO.X-10, OPTION_1_RADIO.Y-20);
  tft.println("Inactive");
  tft.setCursor(OPTION_2_RADIO.X-5, OPTION_2_RADIO.Y-20);
  tft.println("2 mins");
  tft.setCursor(OPTION_3_RADIO.X-5, OPTION_3_RADIO.Y-20);
  tft.println("5 mins");
  tft.setCursor(OPTION_4_RADIO.X-5, OPTION_4_RADIO.Y-20);
  tft.println("10 mins");
  
  
  tft.drawCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_3_RADIO.X + 20, OPTION_3_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 15, ILI9341_BLACK);
  switch (inactiveTimeout) {
     case 0:
       tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 240:
       tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 600:
       tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 1200:
       tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     default: 
       // if nothing else matches, do the default
     break;
   }

  
  OPTION_1_RADIO.active = 1;
  OPTION_2_RADIO.active = 1;
  OPTION_3_RADIO.active = 1;
  OPTION_4_RADIO.active = 1;
  
  //returnButton();
  //RETURN_BOX.active = 1;
}

void activePopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(50, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Active Timeout");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);
  tft.setCursor(OPTION_1_RADIO.X-10, OPTION_1_RADIO.Y-20);
  tft.println("Inactive");
  tft.setCursor(OPTION_2_RADIO.X-5, OPTION_2_RADIO.Y-20);
  tft.println("2 mins");
  tft.setCursor(OPTION_3_RADIO.X-5, OPTION_3_RADIO.Y-20);
  tft.println("5 mins");
  tft.setCursor(OPTION_4_RADIO.X-5, OPTION_4_RADIO.Y-20);
  tft.println("10 mins");
  
  tft.drawCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_3_RADIO.X + 20, OPTION_3_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 15, ILI9341_BLACK);
  switch (activeTimeout) {
     case 0:
       tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 240:
       tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 600:
       tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 1200:
       tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     default: 
       // if nothing else matches, do the default
     break;
   }

  
  OPTION_1_RADIO.active = 1;
  OPTION_2_RADIO.active = 1;
  OPTION_3_RADIO.active = 1;
  OPTION_4_RADIO.active = 1;
  
  //returnButton();
  //RETURN_BOX.active = 1;
}


void dispatchPopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(35, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Dispatch Timeout");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);
  tft.setCursor(OPTION_1_RADIO.X-10, OPTION_1_RADIO.Y-20);
  tft.println("Inactive");
  tft.setCursor(OPTION_2_RADIO.X-5, OPTION_2_RADIO.Y-20);
  tft.println("2 mins");
  tft.setCursor(OPTION_3_RADIO.X-5, OPTION_3_RADIO.Y-20);
  tft.println("5 mins");
  tft.setCursor(OPTION_4_RADIO.X-5, OPTION_4_RADIO.Y-20);
  tft.println("10 mins");
  
  tft.drawCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_3_RADIO.X + 20, OPTION_3_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 15, ILI9341_BLACK);
  switch (dispatchTimeout) {
     case 0:
       tft.fillCircle(OPTION_1_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 240:
       tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 600:
       tft.fillCircle(OPTION_3_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     case 1200:
       tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_1_RADIO.Y + 20, 10, DOT_COLOUR);
       break;
     default: 
       // if nothing else matches, do the default
     break;
   }

  
  OPTION_1_RADIO.active = 1;
  OPTION_2_RADIO.active = 1;
  OPTION_3_RADIO.active = 1;
  OPTION_4_RADIO.active = 1;
  
  //returnButton();
  //RETURN_BOX.active = 1;
}

void analogPopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(60, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Analog Mode");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);
  tft.setCursor(OPTION_2_RADIO.X-10, OPTION_2_RADIO.Y-20);
  tft.println("Inactive");
  tft.setCursor(OPTION_4_RADIO.X-5, OPTION_4_RADIO.Y-20);
  tft.println("Active");

  tft.drawCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 15, ILI9341_BLACK);

  if (mode_word.analog_en) {
    tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 10, DOT_COLOUR);
  } else {
    tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 10, DOT_COLOUR);
  }

  OPTION_2_RADIO.active = 1;
  OPTION_4_RADIO.active = 1;

}

void currentPopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(40, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Current Settings");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);

}

void railcomPopup()
{
  //tft.fillScreen(BACKGROUND);
  backgroundSettingsPage();
  tft.fillRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_WHITE);
  tft.drawRoundRect(SPLASH_BOX.X, SPLASH_BOX.Y, SPLASH_BOX.W, SPLASH_BOX.H, 10,ILI9341_BLACK);
  tft.setCursor(50, 30);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_24);
  tft.print("Railcom Cutout");
  deactivateBoxes();
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(Arial_12);
  tft.setCursor(OPTION_2_RADIO.X-10, OPTION_2_RADIO.Y-20);
  tft.println("Inactive");
  tft.setCursor(OPTION_4_RADIO.X-5, OPTION_4_RADIO.Y-20);
  tft.println("Active");

  tft.drawCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 15, ILI9341_BLACK);
  tft.drawCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 15, ILI9341_BLACK);

  if (mode_word.railcom) {
    tft.fillCircle(OPTION_4_RADIO.X + 20, OPTION_4_RADIO.Y + 20, 10, DOT_COLOUR);
  } else {
    tft.fillCircle(OPTION_2_RADIO.X + 20, OPTION_2_RADIO.Y + 20, 10, DOT_COLOUR);
  }

  OPTION_2_RADIO.active = 1;
  OPTION_4_RADIO.active = 1;

}

void deactivateBoxes() {
  CURRENT_FRAME.active = 0;
  CURRENT_BOX.active = 0;
  SESSIONS_BOX.active = 0;
  TRACK_STAT.active = 0;
  RAIL_COM.active = 0;
  SETTINGS_BOX.active = 0;
  SWAP_BOX.active = 0;
  ACTIVE_TIMEOUT.active = 0;
  INACTIVE_TIMEOUT.active = 0;
  DISPATCH_TIMEOUT.active = 0;
  ANALOG_SETTING.active = 0;
  CURRENT_SETTING.active = 0;
  OPTION_1_RADIO.active = 0;
  OPTION_2_RADIO.active = 0;
  OPTION_3_RADIO.active = 0;
  OPTION_4_RADIO.active = 0;
}

//===========================================================
// Try Draw using writeRect
void bmpDraw(const char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint16_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  uint16_t awColors[320];  // hold colors for one row at a time...

  if((x >= tft.width()) || (y >= tft.height())) return;

  //Serial.println();
  //Serial.print(F("Loading image '"));
  //Serial.print(filename);
  //Serial.println('\'');

  // Open requested file on SD card
  if (!(bmpFile = SD.open(filename))) {
    tft.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    tft.print(F("File size: ")); tft.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    tft.print(F("Image Offset: ")); tft.println(bmpImageoffset, DEC);
    // Read DIB header
    tft.print(F("Header size: ")); tft.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      tft.print(F("Bit Depth: ")); tft.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        //Serial.print(F("Image size: "));
        //Serial.print(bmpWidth);
        //Serial.print('x');
        //Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            awColors[col] = tft.color565(r,g,b);
          } // end pixel
          tft.writeRect(0, row, w, 1, awColors);
        } // end scanline
        //Serial.print(F("Loaded in "));
        //Serial.print(millis() - startTime);
        //Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) tft.println(F("BMP format not recognized."));
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

