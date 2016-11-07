#include <ILI9341_t3.h>
//#include <Adafruit_STMPE610.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3700
#define TS_MINY 3700
#define TS_MAXX 350
#define TS_MAXY 400

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
  } else{
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
  tft.fillRoundRect(SETTINGS_BOX.X, SETTINGS_BOX.Y, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_WHITE);
  tft.drawRoundRect(SETTINGS_BOX.X, SETTINGS_BOX.Y, SETTINGS_BOX.W, SETTINGS_BOX.H, 4, ILI9341_BLACK);
  tft.setCursor(SETTINGS_BOX.X+3, SETTINGS_BOX.Y+3);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(AwesomeF000_40);
  tft.print((char)100);
  tft.setCursor(SETTINGS_BOX.X+2, SETTINGS_BOX.Y+2);
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
  CURRENT_FRAME.active = 0;
  CURRENT_BOX.active = 0;
  SESSIONS_BOX.active = 0;
  TRACK_STAT.active = 0;
  RAIL_COM.active = 1;
  SETTINGS_BOX.active = 0;
  SWAP_BOX.active = 0;
  
  returnButton();
  railComIcon();
  RETURN_BOX.active = 1;
}

void mainPage()
{
  tft.fillScreen(BACKGROUND);
  RETURN_BOX.active = 0;
  CURRENT_FRAME.active = 1;
  CURRENT_BOX.active = 1;
  SESSIONS_BOX.active = 1;
  TRACK_STAT.active = 1;
  RAIL_COM.active = 0;
  SETTINGS_BOX.active = 1;
  SWAP_BOX.active = 1;
  initScreenCurrent();
  swapButton();
  settingsButton();
  
}

