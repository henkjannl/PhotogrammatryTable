#include "WiFi.h"
#include <esp_now.h>
#include "Stepper.h"

#define STEPS_PER_REV  4096
#define DEBUG true

// mac address of the remote controller
const uint8_t broadcastAddress[] = {0xB4, 0xE6, 0x2D, 0xFB, 0x05, 0x11};

enum comm_state_t { cmsIdle, cmsRequestFromRemote, cmsError };

// == GLOBAL VARIABLES ==
// Variables for serial control
int inByte = 0;        // incoming serial byte
int NumberReceived = 0;
int counter = 1;

esp_now_peer_info_t peerInfo;
comm_state_t comm_state = cmsIdle;

// Variables for the stepper motor
Stepper table = Stepper(26, 17, 4, 25);
int stepsPerMove = STEPS_PER_REV / 35;

// == ESP-NOW interface ==
enum message_type_t { msgRequestTurn, msgAcknowledge, msgMotionCompleted, msgError };

struct message_t {
  message_type_t message_type;
  float angle;
};

void send_message(message_type_t message_type ) {

  // Prepare message for remote controller
  message_t message = { message_type, 0 };

  // Send message to remote controller
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &message, sizeof(message));

  if(result==ESP_NOW_SEND_FAIL) {
    comm_state = cmsError;
  }
};

//typedef void (*esp_now_send_cb_t)(const uint8_t *mac_addr, esp_now_send_status_t status)
void send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status!=ESP_NOW_SEND_SUCCESS) {
    comm_state = cmsError;
  }
};

//typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
void receive_callback(const uint8_t * mac, const uint8_t *data, int len) {
  message_t message;
  memcpy(&message, data, sizeof(message_t) );

  // Confirm message was received
  send_message(msgAcknowledge);

  int stepsPerMove = STEPS_PER_REV * message.angle / 360;
  table.moveRelative( 1000, stepsPerMove );   

  comm_state = cmsRequestFromRemote;
}

void setup() {

  // Initialize serial interface
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  WiFi.mode(WIFI_MODE_STA);
  if (esp_now_init() != ESP_OK) Serial.println("Error initializing ESP-NOW");

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Setup callback function for ESP-NOW
  esp_now_register_recv_cb(receive_callback);
  esp_now_register_send_cb(send_callback);

  // Initialize stepper motor
  table.moveRelative( 10000, STEPS_PER_REV );   
  while(table.moving) {
    table.run();
  }

  #if DEBUG
    Serial.println(WiFi.macAddress());
  #endif

  Serial.println("Rotary table initialized");
}

void loop() {
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();

    /*    Character	ASCII
          \n         10
          \r         13
          R          82
          r         114
          ,          44
          <          60
          .          46
          >          62
          0          48
          :          :
          9          57        */

    if( ( inByte == 82 ) || ( inByte == 114 ) ) {
      // 'r' or 'R' reset the counter
      counter = 1;
      Serial.printf("Counter reset to %d\n", counter);
    }

    else if( ( inByte == 44 ) || ( inByte == 60 ) ) {
      // '<' or ',' make the table run counter clockwise
      Serial.printf("Photo %d. Table moving CCW %d steps\n", counter, stepsPerMove);
      table.moveRelative( 1000, -stepsPerMove );   
      NumberReceived=0;
      counter++;
    } // ( inByte == 44 ) || ( inByte == 60 )

    else if( ( inByte == 46 ) || ( inByte == 62 ) ) {
      // '>' or '.' make the table run clockwise
      Serial.printf("Photo %d. Table moving CW %d steps\n", counter, stepsPerMove);
      table.moveRelative( 1000, stepsPerMove );   
      NumberReceived=0;
      counter++;
    } // ( inByte == 46 ) || ( inByte == 62 )

    else if( ( inByte == 10 ) || ( inByte == 13 ) ) {
      // CR or LF enter the new number
      if( (NumberReceived>0) && (NumberReceived<STEPS_PER_REV) ) {
        stepsPerMove = (int) (STEPS_PER_REV / NumberReceived + 0.5);
        Serial.printf("Number of stops per revolution: %d. Steps per move: %d\n", NumberReceived, stepsPerMove );
      } // NumberReceived>0
      NumberReceived=0;
    } // ( inByte == 13 )

    else if( ( inByte >= 48 ) && ( inByte <= 57 ) ) {
      // 0..9 add something to the number being entered
      NumberReceived = 10*NumberReceived + (inByte-48);
    }  // ( inByte >= 48 ) && ( inByte <= 57 ) )

  } // Serial.available() > 0 

  // Run the table
  table.run();

  // Report that the move is completed
  if ( (comm_state==cmsRequestFromRemote) and (not table.moving) ) {
    comm_state = cmsIdle;
    send_message(msgMotionCompleted);
  }

} // loop() 
