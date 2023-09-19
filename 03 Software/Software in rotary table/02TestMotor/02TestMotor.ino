//====================================================================================
//                                    Constants
//====================================================================================
#define PI 3.1415926535
#define STEPS_PER_REV  4096

//====================================================================================
//                                         Stepper
//====================================================================================

class Stepper {
  public:
    uint8_t pin1, pin2, pin3, pin4;
    bool moving;
    float originTime, duration;
    int originPos, currentPos, targetPos;

    Stepper( int pin1, int pin2, int pin3, int pin4 );
    void move( float currentTime, float duration, int stroke);
    void setOutputs();
    void idle();
    void run( float currentTime );
} ;

Stepper::Stepper( int pin1, int pin2, int pin3, int pin4 ) {
  
  this->pin1 = pin1;
  this->pin2 = pin2;
  this->pin3 = pin3;
  this->pin4 = pin4;
  
  this->moving=false;
  this->currentPos=0;

  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  pinMode(pin3, OUTPUT);
  pinMode(pin4, OUTPUT);
};

void Stepper::move( float currentTime, float duration, int stroke) {
  moving=true;
  originTime=currentTime;
  this->duration=duration;
  originPos=currentPos;
  targetPos+=stroke;
}

void Stepper::setOutputs() {

  int state = currentPos % 8;
  if(state<0) state+=8;
  
  switch (state) {
    case 0:
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW );
      digitalWrite(pin3, LOW );
      digitalWrite(pin4, LOW );
      break;

    case 1:
      digitalWrite( pin1, HIGH );
      digitalWrite( pin2, HIGH );
      digitalWrite( pin3, LOW  );
      digitalWrite( pin4, LOW  );
      break;

    case 2:
      digitalWrite( pin1, LOW  );
      digitalWrite( pin2, HIGH );
      digitalWrite( pin3, LOW  );
      digitalWrite( pin4, LOW  );
      break;

    case 3:
      digitalWrite( pin1, LOW  );
      digitalWrite( pin2, HIGH );
      digitalWrite( pin3, HIGH );
      digitalWrite( pin4, LOW  );
      break;

    case 4:
      digitalWrite( pin1, LOW  );
      digitalWrite( pin2, LOW  );
      digitalWrite( pin3, HIGH );
      digitalWrite( pin4, LOW  );
      break;

    case 5:
      digitalWrite( pin1, LOW  );
      digitalWrite( pin2, LOW  );
      digitalWrite( pin3, HIGH );
      digitalWrite( pin4, HIGH );
      break;

    case 6:
      digitalWrite( pin1, LOW  );
      digitalWrite( pin2, LOW  );
      digitalWrite( pin3, LOW  );
      digitalWrite( pin4, HIGH );
      break;

    case 7:
      digitalWrite( pin1, HIGH );
      digitalWrite( pin2, LOW  );
      digitalWrite( pin3, LOW  );
      digitalWrite( pin4, HIGH );
      break;
  }
  
  // Give the motor some time to respond
  delayMicroseconds(600);
}

void Stepper::idle() {
  moving=false;
  digitalWrite( pin1, LOW );
  digitalWrite( pin2, LOW );
  digitalWrite( pin3, LOW );
  digitalWrite( pin4, LOW );
}

void Stepper::run( float currentTime ) {

  // Directly go back if the stepper state is idle
  if (!moving) return;
  
  // Calculate where we are in time and place
  float relTime = ( currentTime - originTime ) / duration;

  if (relTime>=1) relTime=1;

  float relPos = -4.311125*pow(relTime,5)+10.77781*pow(relTime,4)-11.18382*pow(relTime,3)+5.997917*pow(relTime,2)-0.28631*relTime+0.0027674;
  int desiredPos = (int) originPos+( targetPos-originPos ) * relPos + 0.5;

  // Move the stepper backward or forward
  if( desiredPos > currentPos ) {
    // Stepper needs to move forward
    while( desiredPos> currentPos ) {
      currentPos++;
      setOutputs();
    }
  }
  else {
    // Stepper needs to move backward
    while( desiredPos < currentPos) {
      currentPos--;
      setOutputs();
    }
  }

  // Switch off the motor to preserve power
  if ( currentTime > originTime + duration ) {
    idle();
  }
}

int timeOffset;
float timeSec;
int eventIndex = 0;
Stepper table = Stepper(26, 17, 4, 25);

void setup() {
  // Initialize debug channel
  Serial.begin(115200);

  delay(100);
  Serial.println("Rotary table init");

  timeOffset=millis();
  table.move( 0, 3, STEPS_PER_REV );   
  while(table.moving) {
    timeSec = (float) 0.001 * ( millis() - timeOffset );
    table.run(timeSec);
  }

  timeOffset=millis();
  printf("- table initialized\n");

}

void loop() {

}
