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
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
int lastButtonState = LOW;           // the previous reading from the input pin
int buttonState;                     // the current reading from the input pin
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
  static boolean last_aState = 0;
  
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
  
  boolean newEncode = false;
  boolean state = digitalRead(ENCODER_SW_PIN);

  if (state != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (state != buttonState) {
      buttonState = state;

      /*if (buttonState == HIGH) {
        //Turn on something
      }*/
      newEncode = true;
    }
  }
  lastButtonState = state;

  return newEncode;
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
