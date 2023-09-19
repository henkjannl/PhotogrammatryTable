#include <M5Stack.h>
#include "WiFi.h"
#include <esp_now.h>

//#include <JPEGDecoder.h>

#include <map>
#include <vector>

#include "key.h"
#include "screen.h"

#define VERSION "1.0"
#define DEBUG true

/*
Version info
  1.0 First working version

To do:
- eliminate key class, move callback functions for buttons to screen
*/

// mac address of the turntable
const uint8_t broadcastAddress[] = {0x78, 0x21, 0x84, 0xE1, 0x13, 0x9C};

// Types
enum screens_t { scnSplash, scnMain, scnSettings, scnSteps, scnPhoto };

enum keys_t { btnSettingsMenu, btnStepsMenu, btnPhotoMenu, btnBack, 
  btnTurnCCW, btnTurnCW,
  btnLessSteps, btnMoreSteps, 
  btnResetPhotoCounter, btnResetAngle };

enum message_type_t { msgRequestTurn, msgAcknowledge, msgMotionCompleted, msgError };

struct message_t {
  message_type_t message_type;
  float angle;
};

enum comm_state_t { cmsIdle, cmsRequestSent, cmsAcknowedgeReceived, cmsError };

// Number of steps per revolution of the turntable  
std::vector<int> steps = { 4,5,6,7,8,10,12,16,20,24,28,
                    32,36,40,48,56,60,64,68,75,80,90,100 }; 

// Global variables
screens_t current_screen = scnMain;
int16_t photo = 1;
float angle = 0;
uint8_t step_index = 0;
esp_now_peer_info_t peerInfo;
comm_state_t comm_state = cmsIdle;

// SPIFFS test
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}


// Callback functions to draw the screen
void refresh_icon() {
  std::map<comm_state_t, const char*> icons = { 
    { cmsIdle,                "/icon_idle.jpg" },
    { cmsRequestSent,         "/icon_request.jpg" },
    { cmsAcknowedgeReceived,  "/icon_running.jpg" },
    { cmsError,               "/icon_error.jpg" }
  };

  M5.Lcd.drawJpgFile(SPIFFS, icons[comm_state], 276, 12);
}

void draw_splash_screen(bool first_time) {
  // Overwrite background if screen is shown for the first time
  if(first_time) M5.Lcd.drawJpgFile(SPIFFS, "/splash_screen.jpg");
  M5.Lcd.loadFont(FONT_TEMP_SETPOINT);
  M5.Lcd.setTextColor(CLR_BUTTON_TEXT, CLR_STEP);

  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.drawString(VERSION, 160,  120);  
}

void draw_main_screen(bool first_time) {
  // Overwrite background if screen is shown for the first time
  if(first_time) M5.Lcd.drawJpgFile(SPIFFS, "/main_screen.jpg");

  M5.Lcd.loadFont(FONT_TEMP_SETPOINT);
  M5.Lcd.setTextColor(CLR_BUTTON_TEXT, CLR_BUTTON_FACE);

  char buffer[20];
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.fillRoundRect(15, 62, 120, 70, 10, CLR_BUTTON_FACE);
  sprintf(buffer, "%03d", photo);
  M5.Lcd.drawString(buffer, 75,  97);

  M5.Lcd.setTextDatum(MR_DATUM);
  M5.Lcd.fillRoundRect(154, 62, 123, 70, 10, CLR_BUTTON_FACE);
  sprintf(buffer, "%.1f", angle);
  M5.Lcd.drawString(buffer, 273, 97);

  refresh_icon();
}

void draw_reset_screen(bool first_time) {
  // Ignore first time flag so we don't need fillRoundRect
  M5.Lcd.drawJpgFile(SPIFFS, "/reset_screen.jpg");

  M5.Lcd.loadFont(FONT_TEMP_SETPOINT);
  M5.Lcd.setTextColor(CLR_BUTTON_TEXT, CLR_BUTTON_FACE);

  char buffer[20];
  M5.Lcd.setTextDatum(MC_DATUM);
  sprintf(buffer, "%03d", photo);
  M5.Lcd.drawString(buffer, 75,  97);

  M5.Lcd.setTextDatum(MR_DATUM);
  sprintf(buffer, "%.1f", angle);
  M5.Lcd.drawString(buffer, 273, 97);
}

void draw_settings_screen(bool first_time) {
  if(first_time) M5.Lcd.drawJpgFile(SPIFFS, "/settings_screen.jpg");
}

void draw_steps_screen(bool first_time) {
  if(first_time) M5.Lcd.drawJpgFile(SPIFFS, "/steps_screen.jpg");

  M5.Lcd.loadFont(FONT_TIME); 
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextPadding(5);

  char buffer[30];

  uint16_t center = 36;
  uint16_t left = 8;

  for(int8_t i=-2; i<3; i++) {

    if ( (i+step_index>=0) and ( i+step_index<steps.size() ) ) {
      M5.Lcd.fillRoundRect(center-28, 40, 56, 50, 10, i==0 ? CLR_BUTTON_FACE : CLR_STEP);

      M5.Lcd.setTextColor(i==0 ? CLR_BUTTON_TEXT : CLR_LIGHT_TEXT, i==0 ? CLR_BUTTON_FACE : CLR_STEP);
      sprintf(buffer, "%d", steps[i+step_index]);
      M5.Lcd.drawString(buffer, center, 65);
    }
    else {
      M5.Lcd.fillRect(center-30, 38, 60, 54, CLR_BACKGROUND);
    }

    center+=62;
  }

  M5.Lcd.loadFont(FONT_TIME); 
  M5.Lcd.setTextDatum(MR_DATUM);
  M5.Lcd.setTextColor(CLR_LIGHT_TEXT, CLR_BACKGROUND);
  sprintf(buffer, "%.1f", 360.0/steps[step_index]);
  M5.Lcd.fillRect(0, 134, 185, 40, CLR_BACKGROUND);
  M5.Lcd.drawString(buffer, 180, 154);
}

// Functions for ESP-NOW communication
// Send message via ESP-NOW
void send_message(float requested_angle ) {

  // Prepare message for turntable
  message_t message = { msgRequestTurn, requested_angle };

  // Send message to turntable
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));

  // Modify state machine upon result
  comm_state = (result == ESP_OK) ? cmsAcknowedgeReceived : cmsError;
};

void receive_callback(const uint8_t * mac, const uint8_t *data, int len) {

  // Prepare message for turntable
  message_t message;

  // Copy incoming message to message struct
  memcpy(&message, data, sizeof(message_t) );

  // Change state machine depending on incoming message
  switch(message.message_type) {
    case msgRequestTurn: 
      // This message type is not expected from the turntable
      comm_state = cmsError;
    break;

    case msgAcknowledge:
      comm_state = cmsAcknowedgeReceived;
    break;

    case msgMotionCompleted:
      comm_state = cmsIdle;
    break;

    case msgError:
      comm_state = cmsError;
    break;
  };
}

void send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) comm_state = cmsError;
}

// Callback functions for buttons
void menu_settings(void) {
  current_screen = scnSettings;
}

void menu_back(void) {
  if (current_screen == scnSettings) {
    current_screen = scnMain;
  } else {
    current_screen = scnSettings;
  }
}

void steps_menu(void) {
  current_screen = scnSteps;
}

void photo_menu(void) {
  current_screen = scnPhoto;
}

void turn_CCW(void) {
  photo++;
  float diff_angle = -360/steps[step_index];
  angle += diff_angle;
  while (angle<-360) angle+=360;
  send_message( diff_angle );
}

void turn_CW(void) {
  photo++;
  float diff_angle = +360/steps[step_index];
  angle += diff_angle;
  while (angle>360) angle-=360;
  send_message( diff_angle );
}

void fewer_steps(void) {
  if (step_index>0) {
    step_index--;
  }
}

void more_steps(void) {
  if (step_index<steps.size()-1) {
    step_index++;
  }
}

void reset_photo_counter(void) {
  photo=0;
}

void reset_angle(void) {
  angle=0;
}

// Mapping of buttons to callback functions
// Buttons are called Keys because Button was already taken by the M5stack
std::map<keys_t,Key> Keys = { 
  { btnSettingsMenu,      Key("MENU",   &menu_settings       ) }, 
  { btnBack,              Key("BACK",   &menu_back           ) }, 
  { btnStepsMenu,         Key("STEPS",  &steps_menu          ) }, 
  { btnPhotoMenu,         Key("RESET",  &photo_menu          ) }, 
  { btnTurnCCW,           Key("LEFT",   &turn_CCW            ) }, 
  { btnTurnCW,            Key("RIGHT",  &turn_CW             ) }, 
  { btnLessSteps,         Key("FEWER",  &fewer_steps         ) }, 
  { btnMoreSteps,         Key("MORE",   &more_steps          ) }, 
  { btnResetPhotoCounter, Key("PHOTO",  &reset_photo_counter ) },
  { btnResetAngle,        Key("ANGLE",  &reset_angle         ) }
};

// Mapping of screens, to callback functions for redraw and to buttons below each screen
std::map<screens_t,Screen> screens = { 
  { scnMain,     Screen( &draw_main_screen,     Keys[btnTurnCCW],           Keys[btnTurnCW],     Keys[btnSettingsMenu] ) }, 
  { scnSettings, Screen( &draw_settings_screen, Keys[btnStepsMenu],         Keys[btnPhotoMenu],  Keys[btnBack]         ) },
  { scnSteps,    Screen( &draw_steps_screen,    Keys[btnLessSteps],         Keys[btnMoreSteps],  Keys[btnBack]         ) }, 
  { scnPhoto,    Screen( &draw_reset_screen,    Keys[btnResetPhotoCounter], Keys[btnResetAngle], Keys[btnBack]         ) } 
};

// Main program
void setup() {
  M5.begin(); 
  M5.Power.begin();  

  if (!SPIFFS.begin()) Serial.println("Error initializing SPIFFS");

  listDir(SPIFFS, "/", 0);

  WiFi.mode(WIFI_MODE_STA);

  if (esp_now_init() != ESP_OK) Serial.println("Error initializing ESP-NOW");

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK) Serial.println("Failed to add peer");

  esp_now_register_send_cb(send_callback);
  esp_now_register_recv_cb(receive_callback);
  
  // Select 36 steps per revolution by default
  while(steps[step_index]<36) step_index++;

  // For debug: publish mac address of the unit and some font size data
  #if DEBUG
    Serial.println(WiFi.macAddress());
    for (const auto &fnt : Fonts) {
      M5.Lcd.setFreeFont(fnt.first);                 
      Serial.printf("Font %s has height %d XXX %d\n", 
        fnt.second, 
        M5.Lcd.fontHeight(), 
        M5.Lcd.textWidth("XXX") );
    }
  #endif

  draw_splash_screen(true);
  delay(2000);
  draw_main_screen(true);
  Serial.println("Init completed");
}

void loop() {
  static comm_state_t prev_comm_state = cmsError;
  static screens_t prev_screen = scnPhoto;

  M5.update();  

  // Update screen after key was pressed
  if(screens[current_screen].check_keys() ) {
    screens[current_screen].draw(current_screen!=prev_screen);
    prev_screen=current_screen;
  }
  
  // Update icon if status was changed
  if(comm_state != prev_comm_state) {
    prev_comm_state = comm_state;
    if(current_screen == scnMain) refresh_icon();
  };
};
