/**
Slave loop codeg block:
-----------------------
Micro: Attiny84
*/

#include <EEPROM.h>
#include <TinyWireS.h>

// The default buffer size
#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

byte i2c_slave_address = 0x02;

#define LED_PIN                 3
#define GATE_PIN                5
#define RESET_PIN               7
#define SHIFT_DATA_PIN          0
#define SHIFT_LATCH_PIN         1
#define SHIFT_CLOCK_PIN         2
#define ENCODER_SW_ACTION_PIN   7
#define ENCODER_SW_PIN          8
#define ENCODER_A_PIN           9
#define ENCODER_B_PIN           10

volatile uint8_t i2c_regs[] =
{
    0,        // Set new I2C address
    0,        // Activate to any child slave
    0,        // Flash the LED
    0         // Activated block
};
volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);

// Needed for software reset
void(* resetFunc) (void) = 0;

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
    A 
  F   B
    G  
  E   C
    D   DP

  A B C D E F G DP
*/
const byte digit_pattern[17] =
{
  B11111100,  // 0
  B01100000,  // 1
  B11011010,  // 2
  B11110010,  // 3
  B01100110,  // 4
  B10110110,  // 5
  B10111110,  // 6
  B11100000,  // 7
  B11111110,  // 8
  B11110110,  // 9
  B11101110,  // A
  B00111110,  // b
  B10011100,  // C
  B01111010,  // d
  B10011110,  // E
  B10001110,  // F
  B00000001   // .
};


// Slave address is stored at EEPROM address 0x00
byte getAddress() {
  byte i2c_new_address = EEPROM.read(0x00);
  if (i2c_new_address == 0x00) { 
    i2c_new_address = i2c_slave_address; 
  }else if(i2c_new_address != i2c_slave_address){
    // Write back the original placeholder address, in case of an unplug
    EEPROM.write(0x00, i2c_slave_address);
    activate_child();
  }

  return i2c_new_address;
}

// Gets called when the ATtiny receives an i2c command
// First byte is the starting reg address, next is the value
void receiveEvent(uint8_t howMany)
{
  if (howMany < 1)
  {
      // Sanity-check
      reg_position = 0;
      return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE)
  {
      // Also insane number
      reg_position = 0;
      return;
  }

  reg_position = TinyWireS.receive();
  howMany--;
  if (!howMany)
  {
      // This write was only to set the buffer for next read
      reg_position = 0;
      return;
  }
  while(howMany--)
  {
      i2c_regs[reg_position] = TinyWireS.receive();
      // reset position
      reg_position = 0;
      /*reg_position++;
      if (reg_position >= reg_size)
      {
          reg_position = 0;
      }*/
  }
}


// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
  if(i2c_regs[3]){
    TinyWireS.send(i2c_regs[reg_position]);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
      reg_position = 0;
    }
  }
}



void setup() {

  pinMode(RESET_PIN, INPUT);            // Soft RESET
  pinMode(LED_PIN, OUTPUT);             // Status LED
  pinMode(GATE_PIN, OUTPUT);            // Status GATE for child slave

  TinyWireS.begin(getAddress());        // Join i2c network
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  
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
  
  ////////////////////////////////////
  // This needs to be here
  TinyWireS_stop_check();
  ////////////////////////////////////

  set_new_address();
  blink_led();
  activate_child();

  if (readEncoder()){
    updateShiftRegister(encoder_count);
  }
  if (readEncoderButton()){
    clickEncoder();
  }

  readReset();
}


void blink_led()
{
  if(i2c_regs[3] && i2c_regs[2])
  {
    static byte led_on = 1;
    static int blink_interval = 500;
    static unsigned long blink_timeout = millis() + blink_interval;
    
    if(blink_timeout < millis()){
      led_on = !led_on;
      blink_timeout = millis() + blink_interval;
      if(!led_on)
      {
        i2c_regs[2]--;
      }
    }
    digitalWrite(LED_PIN, led_on);
    
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    i2c_regs[2] = 0;
  }

}

void activate_child()
{
  if(i2c_regs[3] && i2c_regs[1])
  {
    digitalWrite(GATE_PIN, HIGH);
  }
  else
  {
    digitalWrite(GATE_PIN, LOW);
    i2c_regs[1] = 0;
  }
}


void set_new_address()
{
  if(i2c_regs[3] && i2c_regs[0])
  {
    //write EEPROM and reset
    EEPROM.write(0x00, i2c_regs[0]);
    i2c_regs[0] = 0;
    resetFunc();  
  }else{
    i2c_regs[0] = 0;
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


void readReset(){
  static const unsigned int REFRESH_INTERVAL = 200; // ms 
  static unsigned long lastRefreshTime = 0;
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
    lastRefreshTime = millis();
    //if (analogRead(RESET_PIN) > 1000 ) {  // reset pin is near Vcc
    if(!digitalRead(RESET_PIN)){
      if(i2c_regs[3]){      // If it is soft resetting for the first time, reset it for real
        resetFunc();
      }
      i2c_regs[1] = 0;                    // disable slave
      i2c_regs[3] = 0;   // Set itself as an inactive block
    } else {                              // reset pin is less than 1000/1024 * 5 vcc
      i2c_regs[3] = 1;   // Set itself as an active block
    }
  }
}