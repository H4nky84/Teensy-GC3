#include <ILI9341_t3.h>
//#include <Adafruit_STMPE610.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000


boolean RecordOn = false;

//#define FRAME_X 210
//#define FRAME_Y 180
//#define FRAME_W 100
//#define FRAME_H 50

#define FRAME_X 100
#define FRAME_Y 100
#define FRAME_W 100
#define FRAME_H 50

#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W/2)
#define REDBUTTON_H FRAME_H

#define TRACK_STAT_X 80
#define TRACK_STAT_Y 160
#define TRACK_STAT_W 140
#define TRACK_STAT_H 60
#define TRACK_STAT_R 8



#define RAIL_COM_X 280
#define RAIL_COM_Y 200
#define RAIL_COM_W 30
#define RAIL_COM_H 30
#define RAIL_COM_R 4

#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W/2)
#define GREENBUTTON_H FRAME_H

struct rectangle{
  unsigned int X;
  unsigned int Y;
  unsigned int W;
  unsigned int H;
};

rectangle TRACK_STAT = {80, 160, 140, 60};
rectangle RAIL_COM = {280, 200, 30, 30};
rectangle CURRENT_FRAME = {7, 50, 306, 60};
rectangle CURRENT_BOX = {188, 60, 85, 40};
rectangle SESSIONS_BOX = {90, 10, 85, 40};


void drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, ILI9341_BLACK);
}

void redBtn()
{ 
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, ILI9341_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(GREENBUTTON_X + 8 , GREENBUTTON_Y + (REDBUTTON_H/3));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("ON");
  RecordOn = false;
}

void greenBtn()
{
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, ILI9341_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(REDBUTTON_X + 6 , REDBUTTON_Y + (REDBUTTON_H/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("OFF");
  RecordOn = true;
}

void trackOffMessage()
{
  tft.fillRoundRect(TRACK_STAT.X+2, TRACK_STAT.Y+2, TRACK_STAT.W, TRACK_STAT.H, 8, ILI9341_BLACK);
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
  analogIcon();
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

void initScreen()
{
  // See if there's any  touch data for us
  if (!ts.bufferEmpty())
  {   
    // Retrieve a point  
    TS_Point p = ts.getPoint(); 
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;

    if (RecordOn)
    {
      if((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W))) {
        if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H))) {
          Serial.println("Red btn hit"); 
          redBtn();
        }
      }
    }
    else //Record is off (RecordOn == false)
    {
      if((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W))) {
        if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H))) {
          Serial.println("Green btn hit"); 
          greenBtn();
        }
      }
    }

    Serial.println(RecordOn);
  }  
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

