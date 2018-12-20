// Micro: Attiny84

int latchPin = A1;
int clockPin = A2;
int dataPin = A0;
byte number = 0;


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
  
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}

void loop() {
  
  for(int i = 0; i < 100; i++){
  
    updateShiftRegister(i);
    delay(300);
  
  }
  
}


/*
 * updateShiftRegister() - This function sets the latchPin to low, 
 * then calls the Arduino function 'shiftOut' to shift out contents 
 * of variable 'leds' in the shift register before putting the 'latchPin' high again.
 */
void updateShiftRegister(byte number)
{
  uint8_t tens = 0;
  uint8_t ones = 0;

  if(number > 0){
    tens = floor(number/10);
    ones = number - (tens * 10);
  }
  
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, digit_pattern[ones]);
  shiftOut(dataPin, clockPin, LSBFIRST, digit_pattern[tens]);
  
  digitalWrite(latchPin, HIGH);
}
