/*
  Scalexercise Controller
  
*/

// include the SPI library for Digital Pots
#include <SPI.h>

//#define DEBUG_CONTROLLER_VALUES 
//#define DEBUG_SMOOTH
//#define DEBUG_SERIAL_INPUT

#define DISPLAY_CONTROLLER_VALUES
#define THROTTLE_DISPLAY_INTERVAL 1000
#define NUM_CONTROLLERS 6
#define NUM_BUTTON_LEDS 4
#define THR_STEP_UP 4   // 
#define THR_STEP_DOWN 10   // 
#define BRAKE_VAL 91    // 18200 ohm
#define CL_VAL    40    // 8000 ohm

#define WRITE_DELAY 1 // so we can see which LED is being written

#define MAX_THROTTLE_VALUE 60

// shift register setup

//Pin connected to ST_CP of 74HC595
const int latchPin = 14;
//Pin connected to SH_CP of 74HC595
const int clockPin = 15;
//Pin connected to DS of 74HC595
const int dataPin = 13;

// blinky light so we know speed of looping
const int statusLEDPin = 12;


// 1q0 == nothing
// 1q1 == nothing
// 1q2 == br10k
// 1q3 == br50k
// 1q4 == re10k
// 1q5 == re0k
// 1q6 == or10k
// 1q7 == or50k
// 
// 2q0 == nothing
// 2q1 == nothing
// 2q2 == ye10k
// 2q3 == ye50k
// 2q4 == gr10k
// 2q5 == gr50k
// 2q6 == bl10k
// 2q7 == bl50k
// 

//int throttle_pin[NUM_CONTROLLERS] = {9, 7, 5};

// the command to set a value to the MCP41xxx
const int command = B10001;

const unsigned int diff_threshold[NUM_CONTROLLERS] = {30, 30, 30, 30, 30, 30};
const float smooth_filter_val[NUM_CONTROLLERS]     = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

unsigned int debug_button_state[NUM_CONTROLLERS]     = {0, 0, 0, 0, 0, 0};
unsigned int controller_enabled[NUM_CONTROLLERS]     = {1, 1, 1, 0, 0, 0};
const unsigned int random_chlane[NUM_CONTROLLERS]    = {1, 1, 1, 1, 1, 1};
const unsigned int throttle_pin[NUM_CONTROLLERS]     = {4, 5, 6, 0, 0, 0};
const unsigned int brake_pin[NUM_CONTROLLERS]        = {0, 0, 0, 0, 0, 0};
const unsigned int changelane_pin[NUM_CONTROLLERS]   = {0, 0, 0, 0, 0, 0};

const unsigned int throttle_pulse_timeout[NUM_CONTROLLERS]     = {500, 500, 3000, 100, 500, 500};
const unsigned int default_pulse_multiplier[NUM_CONTROLLERS]   = {20000, 20000, 50000, 10000, 10000, 10000}; 
unsigned long pulse_multiplier[NUM_CONTROLLERS]                = {20000, 10000, 30000, 10000, 10000, 10000};
unsigned int throttle_value[NUM_CONTROLLERS]                   = {0, 0, 0, 0, 0, 0};
unsigned int brake_button_value[NUM_CONTROLLERS]               = {0, 0, 0, 0, 0, 0};
unsigned int changelane_button_value[NUM_CONTROLLERS]          = {0, 0, 0, 0, 0, 0};
unsigned int throttle_pulse_timeout_count[NUM_CONTROLLERS]     = {0, 0, 0, 0, 0, 0};
unsigned int throttle_pulse_timeout_count_max[NUM_CONTROLLERS] = {1, 1, 1, 1, 1, 1};
unsigned long last_press_time[NUM_CONTROLLERS] = {0, 0, 0, 0, 0, 0};
unsigned long last_debounce_time[NUM_CONTROLLERS] = {0, 0, 0, 0, 0, 0};
float target[NUM_CONTROLLERS] = {0, 0, 0, 0, 0, 0};

unsigned int debounce_delay[NUM_CONTROLLERS] = {0,   0,     0,    0,    0,    0 };
bool button_off_state[NUM_CONTROLLERS]       = {HIGH, LOW,  LOW,  LOW,  LOW,  LOW}; //{HIGH, LOW,  HIGH, HIGH, HIGH, HIGH};
bool button_state[NUM_CONTROLLERS]           = {HIGH, LOW,  LOW,  LOW,  LOW,  LOW}; //{HIGH, LOW,  HIGH, HIGH, HIGH, HIGH};
bool last_button_state[NUM_CONTROLLERS]      = {HIGH, LOW,  LOW,  LOW,  LOW,  LOW}; //{HIGH, LOW,  HIGH, HIGH, HIGH, HIGH};

unsigned int incomingByte = 0;

void setup() {
  // set the slaveSelectPin as an output:
  //for (int i=0; i<NUM_CONTROLLERS; i++) {
  //  pinMode (throttle_pin[i], OUTPUT);
  //  pinMode (throttle_pin[i], OUTPUT);
  //}
  // initialize SPI:
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);//SPI_MODE0, SPI_MODE1, SPI_MODE2, or SPI_MODE3
  SPI.setBitOrder(MSBFIRST); // LSBFIRST or MSBFIRST
  Serial.begin(9600);
  
  // set up shift registers
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(statusLEDPin, OUTPUT);

  // init the chips.
  for (int i=0; i<NUM_CONTROLLERS; i++) {
    digitalPotWrite(2*i, command, 255);
    digitalPotWrite(2*i+1, command, 0);
    if ( throttle_pin[i] > 0 ) {
      pinMode(throttle_pin[i], INPUT);
      digitalWrite(throttle_pin[i], HIGH); // enable internal pull-up
    }
    if ( brake_pin[i] > 0 ) {
      pinMode(brake_pin[i], INPUT);
      digitalWrite(brake_pin[i], HIGH); // enable internal pull-up
    }
    if ( changelane_pin[i] > 0 ) {
      pinMode(changelane_pin[i], INPUT);
      digitalWrite(changelane_pin[i], HIGH); // enable internal pull-up
    }
  }

}

int counter = 0;       // increases each loop, 0-1000
long tick_counter = 0; // increases every 1000 loops

void loop() {
  // go through the six channels of the digital pot:
  // change the resistance on this channel from min to max:
  
  // 0-25 = 128 to 5100 ohm. = speed control
  // +41 = +8200ohm
  // +90 = +18000ohm
  //
  processIncomingCommands();
  processRandomChangeLane();

  displayControllerValues();
  writeAllPotValues();
  setButtonLeds();
  
  counter++;
  counter = counter % 1000;

  if ( counter % 500 < 250 ) {
    digitalWrite(statusLEDPin, HIGH);
  } else {
    digitalWrite(statusLEDPin, LOW);
  }

  if ( counter == 0 ) {
    tick_counter++;
  }

}

void processIncomingCommands() {
  processSerialData();
  processIncomingButtons();
}

void processRandomChangeLane() {
  // if enabled, toggle the changelane button every so often
  for ( int i=0; i<NUM_CONTROLLERS; i++) {
    if ( random_chlane[i] != 0 ) {
      if ( tick_counter % NUM_CONTROLLERS == i ) {
        changelane_button_value[i] = changelane_button_value[i] == 0 ? CL_VAL : 0;
      }
    }
  }
}

void processIncomingButtons() {

  for ( int i=0; i<NUM_CONTROLLERS; i++) {

    if ( controller_enabled[i] == 0 ) {
      continue;
    }

    if ( changelane_pin[i] > 0 ) {
      if ( digitalRead(changelane_pin[i]) == LOW ) {
        changelane_button_value[i] = CL_VAL;
      } else {
        changelane_button_value[i] = 0;
      }
    } 

    if ( brake_pin[i] > 0 ) {
      if ( digitalRead(brake_pin[i]) == LOW ) {
        brake_button_value[i] = BRAKE_VAL;
      } else {
        brake_button_value[i] = 0;
      }
    } 

    int reading;
    if ( throttle_pin[i] > 0 ) {
      reading = digitalRead(throttle_pin[i]);
    } else {
      reading = last_button_state[i]; // spoof no input from unassigned button
    }

    if ( reading != last_button_state[i] ) {
      last_debounce_time[i] = millis();
    }
  
    if ((millis() - last_debounce_time[i]) >= debounce_delay[i] ) {
      button_state[i] = reading;
    } 

    if ( millis() - last_press_time[i] > throttle_pulse_timeout[i] ) {
      target[i] = 0; // timeout
      if ( throttle_value[i] != 0 && debug_button_state[i] ) {
        printControllerAndMillis(i+1);
        Serial.print(": timeout! ");
        Serial.println(millis() - last_press_time[i]);
      }
      throttle_value[i] = smooth(target[i], 0.9, throttle_value[i]);
      printThrottleValue(i+1, target[i], throttle_value[i]);
      if ( throttle_pulse_timeout_count[i] < throttle_pulse_timeout_count_max[i] ) {
        last_press_time[i] = millis();
        throttle_pulse_timeout_count[i]++;
      }
    }

    if ( button_state[i] != button_off_state[i] and last_button_state[i] == button_off_state[i] ) {
      // pressed!
      if ( throttle_pulse_timeout_count[i] > 0 ) {
        if ( debug_button_state[i] ) {
          printControllerAndMillis(i+1);
          Serial.println(": first pulse after timeout");
        }
        throttle_pulse_timeout_count[i] = 0; // reset timeout counter
        // add a bit of speed, but nothing too crazy -- this is because we can't trust (millis - last_press_time)
        throttle_value[i] = min( smooth((throttle_value[i] + 5), smooth_filter_val[i], throttle_value[i]), MAX_THROTTLE_VALUE );
      } else {
        unsigned long diff = millis() - last_press_time[i];
        if ( debug_button_state[i] ) {
          printControllerAndMillis(i+1);
          Serial.print(": pulse! -- diff=");
          Serial.println(diff, DEC);
        }
        if ( diff > diff_threshold[i] ) {
          target[i] = min( (1.0 / diff) * pulse_multiplier[i], MAX_THROTTLE_VALUE);
          throttle_value[i] = min( smooth(target[i], smooth_filter_val[i], throttle_value[i]), MAX_THROTTLE_VALUE );
          printThrottleValue(i+1, target[i], throttle_value[i]);
        }
      }
      last_press_time[i] = millis();

    } else if ( button_state[i] == button_off_state[i] and last_button_state[i] != button_off_state[i] ) {
      //Serial.print(i+1);
      //Serial.println(": button up!");
    }

    last_button_state[i] = reading;

  }

}

void displayControllerValues() {
    if ( counter % THROTTLE_DISPLAY_INTERVAL == 0 )  {
      for ( int i=0; i<NUM_CONTROLLERS; i++) {
#ifdef DISPLAY_CONTROLLER_VALUES
        Serial.print(i+1);
        Serial.print(": a:");
        Serial.print(controller_enabled[i], DEC);
        Serial.print(": c:");
        Serial.print(changelane_button_value[i], DEC);
        Serial.print(", b:");
        Serial.print(brake_button_value[i], DEC);
        Serial.print(", pm:");
        Serial.print(pulse_multiplier[i], DEC);
        Serial.print(", t:");
        Serial.println(throttle_value[i], DEC);
#endif
      }
    }
}

void writeAllPotValues() {
  for ( int i=0; i<NUM_CONTROLLERS; i++) {
    throttle_value[i] = max(throttle_value[i],  0);
    throttle_value[i] = min(throttle_value[i],  MAX_THROTTLE_VALUE);
    digitalPotWrite(i*2, command, 255 - (brake_button_value[i] + changelane_button_value[i]) );
    digitalPotWrite(i*2+1, command, throttle_value[i]);
  }
}

int digitalPotWrite(int pot, int address, int value) {

  // pot 1  = bl50 (2q7)
  // pot 2  = bl10 (2q6)
  // pot 3  = gr50 (2q5)
  // pot 4  = gr10 (2q4)
  // pot 5  = ye50 (2q3)
  // pot 6  = ye10 (2q2)
  // pot 7  = or50 (1q7)
  // pot 8  = or10 (1q6)
  // pot 9  = re50 (1q5)
  // pot 10 = re10 (1q4)
  // pot 11 = br50 (1q3)
  // pot 12 = br10 (1q2)

  // take the SS pin low to select the chip:
  // -- so basically we're writing 01111111 11111111 for pot 1
  //                               11011111 11111111 for pot 3
  //                               11111111 10111111 for pot 8
  
  int bit_to_set;
  // using shift regs, treat pin as the pot #.
  digitalWrite(latchPin, LOW); // so we don't set pots whilst writing
  if ( pot >= 0 && pot < 6 ) {
     // set low sr
     bit_to_set = pot + 2;
     shiftOut(dataPin, clockPin, MSBFIRST, 255); // sr2 11111111
     shiftOut(dataPin, clockPin, MSBFIRST, 255 - (1 << bit_to_set) ); // sr1 data 11110111 (eg)
     //Serial.print(255 - (1 << bit_to_set), DEC);
     //Serial.println();
  } else if ( pot >= 6 && pot < 13 ) {
     // set low sr
     bit_to_set = pot - 6 + 2;
     shiftOut(dataPin, clockPin, MSBFIRST, 255 - (1 << bit_to_set) ); // sr2 11110111 (eg)
     shiftOut(dataPin, clockPin, MSBFIRST, 255); // sr1 11111111
     //Serial.print(255 - (1 << bit_to_set), DEC);
     //Serial.println();
  }
  digitalWrite(latchPin, HIGH); // write out value

  if ( WRITE_DELAY > 1  )
    delay(WRITE_DELAY);

  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);

  // take all SS pins high to deselect pots
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 255 );
  shiftOut(dataPin, clockPin, MSBFIRST, 255 );
  digitalWrite(latchPin, HIGH);

}

void setButtonLeds() {
  //for (int i=0; i<NUM_BUTTON_LEDS; i++) {
  //  digitalWrite(button_led_pin[i], button_led_state[i]);
  //}
}

void processSerialData() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
#ifdef DEBUG_SERIAL_INPUT
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
#endif
 
    if      ( incomingByte == '1' ) { throttle_value[0] = throttle_value[0] + THR_STEP_UP; }
    else if ( incomingByte == '!' ) { controller_enabled[0] = controller_enabled[0] == 0 ? 1 : 0; }
    else if ( incomingByte == 'q' ) { throttle_value[0] = throttle_value[0] > THR_STEP_DOWN ? throttle_value[0] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 'a' ) { changelane_button_value[0] = changelane_button_value[0] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'z' ) { brake_button_value[0] = brake_button_value[0] == 0 ? BRAKE_VAL : 0; } 
    else if ( incomingByte == 'Q' ) { debug_button_state[0] = debug_button_state[0] == 0 ? 1 : 0; } 
    else if ( incomingByte == 'A' ) { pulse_multiplier[0] = pulse_multiplier[0] + 1000; }
    else if ( incomingByte == 'Z' ) { pulse_multiplier[0] = pulse_multiplier[0] - 1000; }

    else if ( incomingByte == '2' ) { throttle_value[1] = throttle_value[1] + THR_STEP_UP; }
    else if ( incomingByte == '@' ) { controller_enabled[1] = controller_enabled[1] == 0 ? 1 : 0; }
    else if ( incomingByte == 'w' ) { throttle_value[1] = throttle_value[1] > THR_STEP_DOWN ? throttle_value[1] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 's' ) { changelane_button_value[1] = changelane_button_value[1] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'x' ) { brake_button_value[1] = brake_button_value[1] == 0 ? BRAKE_VAL : 0; } 
    else if ( incomingByte == 'W' ) { debug_button_state[1] = debug_button_state[1] == 0 ? 1 : 0; }
    else if ( incomingByte == 'S' ) { pulse_multiplier[1] = pulse_multiplier[1] + 1000; }
    else if ( incomingByte == 'X' ) { pulse_multiplier[1] = pulse_multiplier[1] - 1000; }

    else if ( incomingByte == '3' ) { throttle_value[2] = throttle_value[2] + THR_STEP_UP; }
    else if ( incomingByte == '#' ) { controller_enabled[2] = controller_enabled[2] == 0 ? 1 : 0; }
    else if ( incomingByte == 'e' ) { throttle_value[2] = throttle_value[2]  > THR_STEP_DOWN ? throttle_value[2] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 'd' ) { changelane_button_value[2] = changelane_button_value[2] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'c' ) { brake_button_value[2] = brake_button_value[2] == 0 ? BRAKE_VAL : 0; } 
    else if ( incomingByte == 'E' ) { debug_button_state[2] = debug_button_state[2] == 0 ? 1 : 0; }
    else if ( incomingByte == 'D' ) { pulse_multiplier[2] = pulse_multiplier[2] + 1000; }
    else if ( incomingByte == 'C' ) { pulse_multiplier[2] = pulse_multiplier[2] - 1000; }

    else if ( incomingByte == '4' ) { throttle_value[3] = throttle_value[3] + THR_STEP_UP; }
    else if ( incomingByte == '$' ) { controller_enabled[3] = controller_enabled[3] == 0 ? 1 : 0; }
    else if ( incomingByte == 'r' ) { throttle_value[3] = throttle_value[3] > THR_STEP_DOWN ? throttle_value[3] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 'f' ) { changelane_button_value[3] = changelane_button_value[3] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'v' ) { brake_button_value[3] = brake_button_value[3] == 0 ? BRAKE_VAL : 0; } 
    else if ( incomingByte == 'R' ) { debug_button_state[3] = debug_button_state[3] == 0 ? 1 : 0; }
    else if ( incomingByte == 'F' ) { pulse_multiplier[3] = pulse_multiplier[3] + 1000; }
    else if ( incomingByte == 'V' ) { pulse_multiplier[3] = pulse_multiplier[3] - 1000; }

    else if ( incomingByte == '5' ) { throttle_value[4] = throttle_value[4] + THR_STEP_UP; }
    else if ( incomingByte == '%' ) { controller_enabled[4] = controller_enabled[4] == 0 ? 1 : 0; }
    else if ( incomingByte == 't' ) { throttle_value[4] = throttle_value[4] > THR_STEP_DOWN ? throttle_value[4] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 'g' ) { changelane_button_value[4] = changelane_button_value[4] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'b' ) { brake_button_value[4] = brake_button_value[4] == 0 ? BRAKE_VAL : 0; }
    else if ( incomingByte == 'T' ) { debug_button_state[4] = debug_button_state[4] == 0 ? 1 : 0; }
    else if ( incomingByte == 'G' ) { pulse_multiplier[4] = pulse_multiplier[4] + 1000; }
    else if ( incomingByte == 'B' ) { pulse_multiplier[4] = pulse_multiplier[4] - 1000; }

    else if ( incomingByte == '6' ) { throttle_value[5] = throttle_value[5] + THR_STEP_UP; }
    else if ( incomingByte == '^' ) { controller_enabled[5] = controller_enabled[5] == 0 ? 1 : 0; }
    else if ( incomingByte == 'y' ) { throttle_value[5] = throttle_value[5] > THR_STEP_DOWN ? throttle_value[5] - THR_STEP_DOWN : 0; }
    else if ( incomingByte == 'h' ) { changelane_button_value[5] = changelane_button_value[5] == 0 ? CL_VAL : 0; } 
    else if ( incomingByte == 'n' ) { brake_button_value[5] = brake_button_value[5] == 0 ? BRAKE_VAL : 0; }
    else if ( incomingByte == 'Y' ) { debug_button_state[5] = debug_button_state[5] == 0 ? 1 : 0; }
    else if ( incomingByte == 'H' ) { pulse_multiplier[5] = pulse_multiplier[5] + 1000; }
    else if ( incomingByte == 'N' ) { pulse_multiplier[5] = pulse_multiplier[5] - 1000; }

    // ensure all values are sensible before we start using them
    for ( int i=0; i<NUM_CONTROLLERS; i++) {
      if ( throttle_value[i] < 0 ) 
        throttle_value[i] = 0;
      if ( pulse_multiplier[i] < 0 )
        pulse_multiplier[i] = 0;
    } 

  }
}

float smooth(float data, float filterVal, float smoothedVal){
  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }
  float newSmoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);
#ifdef DEBUG_SMOOTH
  if ( smoothedVal != 0 ) {
     Serial.print("smoothedVal: ");
     Serial.println(newSmoothedVal, DEC);
  }
#endif 
  return newSmoothedVal;
}
 
void printThrottleValue(int controller, int target, int throttle_value) {
   if ( throttle_value != 0 ) {
#ifdef DEBUG_CONTROLLER_VALUES
        printControllerAndMillis(controller);
        Serial.print(": target: ");
        Serial.print(target);
        Serial.print(", throttle_value: ");
        Serial.println(throttle_value);
#endif
   }
}

void printControllerAndMillis(int controller) {
        Serial.print(controller);
        Serial.print(": millis: ");
        Serial.print(millis());
}
