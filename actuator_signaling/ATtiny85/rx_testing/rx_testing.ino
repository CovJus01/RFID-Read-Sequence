#include <RCSwitch.h>

#define RX_PIN 0 //Use PB2 (INT0) for receiving
#define LED_PIN 0 //Use PB0 to flash LED
#define SERIAL_PIN 1 //set output serial pin to PB1
#define BIT_DELAY 5 //milliseconds

#define EXPECTED_CODE 11981

RCSwitch tiny_switch = RCSwitch();

void setup() {
  //Run clock at the full 8MHz
  CLKPR = (1 << CLKPCE); //Enable changes to the clock prescaler
  CLKPR = 0; //Set the prescaler to 0 (no clk division)

  //Set serial pin as output
  pinMode(SERIAL_PIN, OUTPUT);
  sendBit(1);

  //Configure LED_PIN as output
  pinMode(LED_PIN, OUTPUT);
  
  //Configure RF receiver
  tiny_switch.enableReceive(RX_PIN);   // Use pin 2 for receiving

  //Flash LED to confirm setup complete
  for (int i=0; i<3; i++) {
    blink_led();
  }
}

void blink_led() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}

void sendBit(bool bit) {
  digitalWrite(SERIAL_PIN, bit);
  delay(BIT_DELAY);
}

void sendInteger(uint16_t data) {
  //Send start bit to indicate incoming transmission
  sendBit(0);

  //Send 16 bit integer
  for (int i=0; i<16; i++) {
    sendBit((data >> i) & 1);
  }

  //Stop bit
  sendBit(1);

  //Delay before next transmission
  delay(10);
}

void loop() {
  if (tiny_switch.available()) {
    //Non-zero code received over RF. Fetch it here
    uint16_t code = tiny_switch.getReceivedValue();

    //Send received code over serial to raspberry pi
    sendInteger(code);    

    //Flash LED if correct code received
    if (code == EXPECTED_CODE) {
      for (int i=0; i<2; i++) {
        blink_led();
      }
    }
     
    tiny_switch.resetAvailable();
  }
}
