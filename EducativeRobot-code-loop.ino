/**
Slave loop codeg block:
-----------------------
Micro: Attiny84
*/

#include <EEPROM.h>
#include <TinyWireS.h>


#define SHIFT_LATCH_PIN   1
#define SHIFT_CLOCK_PIN   2
#define SHIFT_DATA_PIN    0
#define ENCODER_SW_PIN    8
#define ENCODER_SW_ACTION_PIN 7
#define ENCODER_A_PIN     9
#define ENCODER_B_PIN     10

byte number = 0;
int encoder_count = 0;
byte encoder_max = 99;

// Encoder Switch Debouncing (Kenneth A. Kuhn algorithm)
#define DEBOUNCE_TIME               0.3
#define DEBOUNCE_SAMPLE_FREQUENCY   10
#define DEBOUNCE_MAXIMUM            (DEBOUNCE_TIME * DEBOUNCE_SAMPLE_FREQUENCY)
boolean buttonInput = true;        /* 0 or 1 depending on the input signal */
unsigned int buttonIntegrator;      /* Will range from 0 to the specified MAXIMUM */
boolean buttonOutput = true;       /* Cleaned-up version of the input signal */

/*
    F 
  A   G
    B  
  E   C
    D   DP

  A B C D E F G DP
*/
const byte digit_pattern[17] =
{
  B10111110,  // 0
  B00100010,  // 1
  B01011110,  // 2
  B01110110,  // 3
  B11100010,  // 4
  B11110100,  // 5
  B11111100,  // 6
  B00100110,  // 7
  B11111110,  // 8
  B11100110,  // 9
  B11101110,  // A
  B11111000,  // b
  B10011100,  // C
  B01111010,  // d
  B11011100,  // E
  B11001100,  // F
  B00000001   // .
};





void setup() {
  
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  pinMode(SHIFT_DATA_PIN, OUTPUT);  
  pinMode(SHIFT_CLOCK_PIN, OUTPUT);

  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_ACTION_PIN, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT);
  pinMode(ENCODER_B_PIN, INPUT);

  updateShiftRegister(0);

}

void loop() {
  
  if (readEncoder()){
    updateShiftRegister(encoder_count);
  }
  if (readEncoderButton()){
    clickEncoder();
  }
  
}


boolean readEncoder(){

  boolean newEncode = false;
  static boolean last_aState = true;
  
  boolean aState = digitalRead(ENCODER_A_PIN);
  boolean bState = digitalRead(ENCODER_B_PIN);

  if (aState && aState != last_aState){
    if (aState != bState){
      encoder_count--;
      if(encoder_count < 0){
        encoder_count = encoder_max;
      }
    }else{
      encoder_count++;
      if(encoder_count > encoder_max){
        encoder_count = 0;
      }
    }
    newEncode = true;
  }

  last_aState = aState;

  return newEncode;
}

boolean readEncoderButton(){
  
  boolean newPushButton = false;

  /* Step 1: Update the integrator based on the input signal.  Note that the
  integrator follows the input, decreasing or increasing towards the limits as
  determined by the input state (0 or 1). */
  buttonInput = digitalRead(ENCODER_SW_PIN);
  if (buttonInput == false){
    if (buttonIntegrator > 0){
      buttonIntegrator--;
    }
  }else if (buttonIntegrator < DEBOUNCE_MAXIMUM){
    buttonIntegrator++;
  }
  /* Step 2: Update the output state based on the integrator.  Note that the
  output will only change states if the integrator has reached a limit, either
  0 or DEBOUNCE_MAXIMUM. */
  if (buttonIntegrator == 0){
    buttonOutput = false;
  }else if (buttonIntegrator >= DEBOUNCE_MAXIMUM){
    if(buttonInput != buttonOutput){
      newPushButton = true;
    }
    buttonOutput = true;
    buttonIntegrator = DEBOUNCE_MAXIMUM;  /* defensive code if integrator got corrupted */
  }
  //digitalWrite(ENCODER_SW_ACTION_PIN, output);

  return newPushButton;
  
}

void clickEncoder(){
  // Code to execut on encoder click
  digitalWrite(ENCODER_SW_ACTION_PIN, !digitalRead(ENCODER_SW_ACTION_PIN));

}



/*
 * This function update the number in the 7-segment displays
 */
void updateShiftRegister(int number)
{
  uint8_t tens = 0;
  uint8_t ones = 0;

  if(number > 0){
    tens = floor(number/10);
    ones = number - (tens * 10);
  }
  
  digitalWrite(SHIFT_LATCH_PIN, LOW);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, LSBFIRST, digit_pattern[ones]);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, LSBFIRST, digit_pattern[tens]);
  
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}
