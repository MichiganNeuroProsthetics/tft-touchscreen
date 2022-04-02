#include <VarSpeedServo.h> // servo position/speed control
#include <TouchScreen.h> // touch library
#include <LCDWIKI_GUI.h> // Core graphics library
#include <LCDWIKI_KBV.h> // Hardware-specific library

// create screen object
LCDWIKI_KBV my_lcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0xBDF7

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

#define TS_MINX 1000
#define TS_MAXX 100

#define TS_MINY 100
#define TS_MAXY 1000
// We have a status line for like, is FONA working
#define STATUS_X 10
#define STATUS_Y 65

#define MINPRESSURE 0
#define MAXPRESSURE 3000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

bool servoControlType = true;
int rgbState = 1;
int xposMenu = my_lcd.Get_Display_Width() - 110;
int servoSpeed = 127;
int servoPosition = 0;
int redPin = 47;
int greenPin = 43;
int bluePin = 39;

const int servoPin = 51;
VarSpeedServo myServo;

void show_string(uint8_t *str,int16_t x,int16_t y,uint8_t csize,uint16_t fc, uint16_t bc,boolean mode)
{
    my_lcd.Set_Text_Mode(mode);
    my_lcd.Set_Text_Size(csize);
    my_lcd.Set_Text_colour(fc);
    my_lcd.Set_Text_Back_colour(bc);
    my_lcd.Print_String(str,x,y);
}

void Draw_toggle(bool &onOff) {
  // x1, y1, x2, y2
  show_string("Angle | Speed", 14, 45, 1, WHITE, BLACK, 1);
  if (onOff == true) {
    my_lcd.Set_Draw_color(GREEN);
    my_lcd.Fill_Rectangle(10, 10, 50, 40);
    my_lcd.Set_Draw_color(WHITE);
    my_lcd.Fill_Rectangle(50, 10, 90, 40);
    // string, x, y, text size, font color, background color, textmode
    show_string("Ang", 14, 20, 2, BLACK, BLACK, 1);
    onOff = false;
  }
  else {
    my_lcd.Set_Draw_color(RED);
    my_lcd.Fill_Rectangle(50, 10, 90, 40);
    my_lcd.Set_Draw_color(WHITE);
    my_lcd.Fill_Rectangle(10, 10, 50, 40);
    // string, x, y, text size, font color, background color, textmode
    show_string("Spe", 54, 20, 2, BLACK, BLACK, 1);
    onOff = true;
  }
}

void Menu_button(int color) {
  if (color == 1) {
    my_lcd.Set_Draw_color(BLUE);
  }
  else {
    my_lcd.Set_Draw_color(CYAN);
  }
  int xpos = my_lcd.Get_Display_Width() - 110;
  my_lcd.Fill_Rectangle(xpos, 10, xpos + 100, 40);
  show_string("Servo", xpos + 10, 20, 2, BLACK, BLACK, 1);
}

int slider(TSPoint p) {
  int xStart = 0;
  p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
  p.x = my_lcd.Get_Display_Width()-map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
  my_lcd.Set_Draw_color(GREY);
  my_lcd.Fill_Rectangle(80, 220, 240, 260);
  my_lcd.Set_Draw_color(WHITE);
  xStart = map(p.x, my_lcd.Get_Display_Width(), 0, 96, 890);
  //Serial.print(xStart);
  //Serial.print('\n');
  my_lcd.Fill_Rectangle(xStart, 220, xStart + 20, 260);
  return xStart;
}

void sliderInitial(bool servoControlType) {
  my_lcd.Set_Draw_color(GREY);
  my_lcd.Fill_Rectangle(80, 220, 240, 260);
  my_lcd.Set_Draw_color(WHITE);
  my_lcd.Fill_Rectangle(80, 220, 100, 260);
  if (servoControlType) {
    show_string("Change Speed", 90, 200, 2, WHITE, BLACK, 1);
    show_string("0", 75, 280, 1, WHITE, BLACK, 1);
    show_string("255``", 245, 280, 1, WHITE, BLACK, 1);
  }
  else {
    show_string("Change Angle", 90, 200, 2, WHITE, BLACK, 1);
    show_string("0", 75, 280, 1, WHITE, BLACK, 1);
    show_string("180", 245, 280, 1, WHITE, BLACK, 1);
  }
}

void back_button() {
  my_lcd.Set_Draw_color(BLUE);
  int xpos = my_lcd.Get_Display_Width() - 110;
  my_lcd.Fill_Rectangle(xpos, 10, xpos + 100, 40);
  show_string("BACK", xpos + 10, 20, 2, BLACK, BLACK, 1);
}

void RGB_button(int &state) {
   if (state == 1) {
    my_lcd.Set_Draw_color(RED);
    my_lcd.Fill_Circle(160, 240, 60);
    // string, x, y, text size, font color, background color, textmode
    show_string("R", 160, 240, 2, BLACK, BLACK, 1);
    
    state = 2;
  }
  else if (state == 2) {
    my_lcd.Set_Draw_color(GREEN);
    my_lcd.Fill_Circle(160, 240, 60);
    show_string("G", 160, 240, 2, BLACK, BLACK, 1);
    state = 3;
  }
  else {
    my_lcd.Set_Draw_color(BLUE);
    my_lcd.Fill_Circle(160, 240, 60);
    show_string("B", 160, 240, 2, BLACK, BLACK, 1);
    state = 1;
  }
}

void newMenu(bool &servoControlType, int state, int &servoSpeed, int &servoPosition) {
  my_lcd.Fill_Screen(BLACK);
  back_button();
  sliderInitial(servoControlType);
  int angle = 0;
  int pressed = false;
  while (pressed == false) {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
      p.x = my_lcd.Get_Display_Width()-map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
      if (is_pressed(xposMenu, 10, xposMenu + 100, 40, p.x, p.y)) {
        pressed = true;
      }
      else if (is_pressed(80, 220, 240, 260, p.x, p.y)) {
          if (servoControlType) {
            servoSpeed = map(slider(p), 86, 220, 0, 255);
          }
          else {
            servoPosition = map(slider(p), 86, 220, 0, 180);
          }
      }
    }
    if (servoControlType) {
      if (angle == 0) angle = 180;
      else angle = 0;
      myServo.slowmove(angle, servoSpeed);
      delay(map(servoSpeed, 0, 255, 3000, 500));
    }
    else {
      myServo.slowmove(servoPosition, 180);
    }
  }
  if (servoControlType) {
    servoControlType = false;
  }
  else {
    servoControlType = true;
  }
  my_lcd.Fill_Screen(BLACK);
  Draw_toggle(servoControlType);
  Menu_button(1);
  RGB_button(state);
}

// Check if spot/button has been pressed
boolean is_pressed(int16_t x1,int16_t y1,int16_t x2,int16_t y2,int16_t px,int16_t py)
{
    if((px > x1 && px < x2) && (py > y1 && py < y2))
    {
        return true;  
    } 
    else
    {
        return false;  
    }
 }
void setup() {
  Serial.begin(9600);
  my_lcd.Init_LCD();
  my_lcd.Fill_Screen(BLACK);
  Draw_toggle(servoControlType);
  Menu_button(1);
  RGB_button(rgbState);
  myServo.attach(servoPin, 0, 180);
  myServo.slowmove(0, 180);
}

void loop() {
  // put your main code here, to run repeatedly:
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
    p.x = my_lcd.Get_Display_Width()-map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
    if (is_pressed(10, 10, 90, 40, p.x, p.y)) {
      Draw_toggle(servoControlType);
    } // if is_pressed()
    else if (is_pressed(xposMenu, 10, xposMenu + 100, 40, p.x, p.y)) {
      Menu_button(2);
      delay(300);
      newMenu(servoControlType, rgbState, servoSpeed, servoPosition);
      delay(300);
    }
    else if (is_pressed(100, 180, 220, 300, p.x, p.y)) {
      RGB_button(rgbState);
    }
    
    if (rgbState == 1) {
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
    }
    else if (rgbState == 2) {
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
    }
    else {
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
    }
  } // if pressure is withing bounds
  delay(100);
} // void loop()
