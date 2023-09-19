const int STEPS_PER_REV = 4096;
int inByte = 0;        // incoming serial byte
int NumberReceived = 0;
int stepsPerMove = STEPS_PER_REV / 35;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() {
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    Serial.println(inByte);

    /*
    Character	ASCII
    \n        10
    ,         44
    <         60
    .         46
    >         62
    0         48
    :         :
    9         57
    */

    if( ( inByte == 44 ) || ( inByte == 60 ) ) {
      Serial.println("Table moving CCW");
      moveLeft();
      NumberReceived=0;
    } // ( inByte == 44 ) || ( inByte == 60 )

    else if( ( inByte == 46 ) || ( inByte == 62 ) ) {
      Serial.println("Table moving CW");
      moveRight();
      NumberReceived=0;
    } // ( inByte == 46 ) || ( inByte == 62 )

    else if( ( inByte == 10 ) || ( inByte == 13 ) ) {
      if( (NumberReceived>0) && (NumberReceived<STEPS_PER_REV) ) {
        Serial.printf("Number of stops per revolution: %d\n", NumberReceived );
        stepsPerMove = (int) (STEPS_PER_REV / NumberReceived + 0.5);
        Serial.printf("Steps per move: %d\n", stepsPerMove );
      } // NumberReceived>0
      NumberReceived=0;
    } // ( inByte == 13 )

    else if( ( inByte >= 48 ) && ( inByte <= 57 ) ) {
      NumberReceived = 10*NumberReceived + (inByte-48);
    }  // ( inByte >= 48 ) && ( inByte <= 57 ) )

  } // Serial.available() > 0 

} // loop() 

void moveLeft() {
  //
}

void moveRight() {
  //
}
