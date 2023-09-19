#pragma once

#include "key.h"

#define FSS9   &FreeSans9pt7b
#define FSSB12 &FreeSansBold12pt7b
#define FSS12  &FreeSans12pt7b
#define FSSB18 &FreeSansBold18pt7b
#define FSS18  &FreeSans18pt7b
#define FSSB24 &FreeSansBold24pt7b

// Screen fonts
#define FONT_MENU_HEADER        "Noway_Medium24"  // Font Noway_Medium24 height 24
#define FONT_MENU               "Noway_Light18"   // Font Noway_Light18 height 17
#define FONT_SUBMENU            "Noway_Light18"   // Font Noway_Light18 height 17
#define FONT_DATE               "Noway_Light24"   // Font Noway_Light24 height 23
#define FONT_TIME               "Noway_Medium24"  // Font Noway_Medium24 height 24
#define FONT_TEMP_MEASURED      "Noway_Medium60"  // Font Noway_Medium60 height 58
#define FONT_TEMP_MEASURED_DEG  "Noway_Light32"   // Font Noway_Light32 height 31
#define FONT_TEMP_SETPOINT      "Noway_Medium32"  // Font Noway_Medium32 height 31
#define FONT_TEMP_SETPOINT_DEG  "Noway_Light18"   // Font Noway_Light18 height 17
#define FONT_NORMAL             "Noway_Regular24" // Font Noway_Regular24 height 23
#define FONT_MEDIUM             "Noway_Medium24"  // Font Noway_Medium24 height 24

#define CLR_BACKGROUND       0x0000   // 00, 00, 00 = black
#define CLR_BUTTON_FACE      0x52AA   // 55, 55, 55 = dark grey
#define CLR_STEP             0x2945   // 2B, 2B, 2B = grey
#define CLR_BUTTON_TEXT      0xFFFF   // FF, FF, FF = white
#define CLR_HEADER_TEXT      0xFFE3   // FF, FF, 19 = yellow
#define CLR_LIGHT_TEXT       0xB596   // B0, B0, B0 = light grey

std::map<const GFXfont*,const char*> Fonts = { 
  { FSS9,   "FSS9 FreeSans9pt7b"      },
  { FSSB12, "FSSB12 FreeSansBold12pt7b" },
  { FSS12,  "FSS12 FreeSans12pt7b"     },
  { FSSB18, "FSSB18 FreeSansBold18pt7b" },
  { FSS18,  "FSS18 FreeSans18pt7b"     },
  { FSSB24, "FSSB24 FreeSansBold24pt7b" }
};

// Screen class
class Screen {

  public:
    Key left_key, middle_key, right_key;
    void (*draw_callback)(bool);

    Screen();
    Screen(void (*draw_callback)(bool), Key left_key, Key middle_key, Key right_key);
    void draw(bool first_time);
    bool check_keys();
};

Screen::Screen() {
}  

Screen::Screen(void (*draw_callback)(bool), Key left_key, Key middle_key, Key right_key) {
  this->draw_callback = draw_callback;
  this->left_key      = left_key;
  this->middle_key    = middle_key;
  this->right_key     = right_key; 
}

void Screen::draw(bool first_time) {
  /*
  M5.Lcd.setTextColor(CLR_BUTTON_TEXT, CLR_BUTTON_FACE);

  M5.Lcd.setFreeFont(FSSB12);                 
  M5.Lcd.setTextDatum(MC_DATUM);

  M5.Lcd.fillScreen(CLR_BACKGROUND);  

  // Left button
  M5.Lcd.fillRoundRect( 10, 198, 93, 32, 8, CLR_BUTTON_FACE);
  M5.Lcd.drawString(this->left_key.label, 57, 214);

  // Middle button
  M5.Lcd.fillRoundRect(113, 198, 93, 32, 8, CLR_BUTTON_FACE);
  M5.Lcd.drawString(this->middle_key.label, 160, 214);

  // Right button
  M5.Lcd.fillRoundRect(217, 198, 93, 32, 8, CLR_BUTTON_FACE);
  M5.Lcd.drawString(this->right_key.label, 263, 214);

  // Rectangle for the remaining part of the screen
  M5.Lcd.fillRoundRect( 10, 10, 300, 178, 8, CLR_BUTTON_FACE);
  */

  draw_callback(first_time);
}

bool Screen::check_keys() {
  if (M5.BtnA.wasPressed() ) {
    left_key.key_callback();
    return true;
  }

  if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)) {
    middle_key.key_callback();
    return true;
  }

  if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
    right_key.key_callback();
    return true;
  }

  return false;
}
