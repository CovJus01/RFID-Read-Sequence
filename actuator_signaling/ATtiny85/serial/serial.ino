#define SERIAL_PIN 1 //set output serial pin to PB1
#define BIT_DELAY 5 //milliseconds

void setup() {
  //Run clock at the full 8MHz
  CLKPR = (1 << CLKPCE); //Enable changes to the clock prescaler
  CLKPR = 0; //Set the prescaler to 0 (no clk division)

  //Set pre-defined serial pin as output
  pinMode(SERIAL_PIN, OUTPUT);
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
  uint16_t code = 12345;
  sendInteger(code);
  delay(2000);
}
