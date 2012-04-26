/*
  Scalexercise Controller
  
*/

// inslude the SPI library:
#include <SPI.h>

//#define NUM_CONTROLLERS 3
#define NUM_CONTROLLERS 2
#define NUM_BUTTON_LEDS 4
#define THR_STEP 2

//int throttle_pin[NUM_CONTROLLERS] = {9, 7, 5};
//int button_pin[NUM_CONTROLLERS] = {10, 8, 6};
int throttle_pin[NUM_CONTROLLERS] = {9, 7};
int button_pin[NUM_CONTROLLERS] = {10, 8};

int button_led_pin[4] = {5, 4, 3, 2};
boolean button_led_state[4] = {0, 0, 0, 0};

// the command to set a value to the MCP41xxx
const int command = B10001;

int throttle_value[NUM_CONTROLLERS] = {0, 0};
int incomingByte = 0;

#define MAX_THROTTLE_VALUE 100

void setup() {
  // set the slaveSelectPin as an output:
  for (int i=0; i<NUM_CONTROLLERS; i++) {
    pinMode (throttle_pin[i], OUTPUT);
    pinMode (button_pin[i], OUTPUT);
  }
  // initialize SPI:
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);//SPI_MODE0, SPI_MODE1, SPI_MODE2, or SPI_MODE3
  SPI.setBitOrder(MSBFIRST); // LSBFIRST or MSBFIRST
  Serial.begin(9600);
  
  // init the chips.
  for (int i=0; i<NUM_CONTROLLERS; i++) {
    digitalPotWrite(throttle_pin[i], command, 0);
    digitalPotWrite(button_pin[i], command, 0);
  }
}

int counter = 0;

void loop() {
  // go through the six channels of the digital pot:
  // change the resistance on this channel from min to max:
  
  // 0-25 = 128 to 5100 ohm. = speed control
  // +41 = +8200ohm
  // +90 = +18000ohm
  //
  processIncomingCommands();
  //processIncomingButtons();
  writeAllPotValues();
  setButtonLeds();
  
  
  counter++;
  counter = counter % 1000;
  
  delay(1);
    
}

void set
void processIncomingCommands() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
 
    if ( incomingByte == 49 ) {
      throttle_value[0] = throttle_value[0] + THR_STEP;
    } else if ( incomingByte == 50 ) {
      throttle_value[0] = throttle_value[0] - THR_STEP;
    } else if ( incomingByte == 51 ) {
      throttle_value[1] = throttle_value[1] + THR_STEP;
    } else if ( incomingByte == 52 ) {
      throttle_value[1] = throttle_value[1] - THR_STEP;
    } else if ( incomingByte == 53 ) {
      throttle_value[2] = throttle_value[2] + THR_STEP;
    } else if ( incomingByte == 54 ) {
      throttle_value[2] = throttle_value[2] - THR_STEP;
    }
  }

  if ( digitalRead(A0) == HIGH ) {
    arcade_button_pins[0] = 0;
  else
    arcade_button_pins[0] = 1;
    
  if ( digitalRead(A1) == HIGH ) {
    arcade_button_pins[1] = 0;
  else
    arcade_button_pins[1] = 1;
    
  if ( digitalRead(A2) == HIGH ) {
    arcade_button_pins[2] = 0;
  else
    arcade_button_pins[2] = 1;
  
  if ( digitalRead(A3) == HIGH ) {
    arcade_button_pins[3] = 0;
  else
    arcade_button_pins[3] = 1;

}

void writeAllPotValues() {
  for ( int i=0; i<NUM_CONTROLLERS; i++) {
    throttle_value[i] = throttle_value[i] % MAX_THROTTLE_VALUE;
    Serial.print(i);
    Serial.print(": ");
    Serial.println(throttle_value[i]);
    digitalPotWrite(throttle_pin[i], command, throttle_value[i]);
  }
}

int digitalPotWrite(int pin, int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(pin,LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(pin,HIGH); 
}

void setButtonLeds() {
  for (int i=0; i<NUM_BUTTON_LEDS; i++) {
    digitalWrite(button_led_pin[i], button_led_state[i]);
  }
}
