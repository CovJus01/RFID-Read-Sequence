#include <RCSwitch.h>
#include <SoftwareServo.h>

#define RX_PIN 0 //Use PB2 (INT0) for receiving
#define SERVO_PIN 0 //PB0 (physical pin 5)
#define SERVO_UPDATE_DELAY 10
#define EXPECTED_CODE 11981

RCSwitch tiny_switch = RCSwitch();
SoftwareServo lin_act;

void setup() {
  //Run clock at the full 8MHz
  CLKPR = (1 << CLKPCE); //Enable changes to the clock prescaler
  CLKPR = 0; //Set the prescaler to 0 (no clk division)
  
  //Configure RF receiver
  tiny_switch.enableReceive(RX_PIN);   // Use pin 2 for receiving

  //Initialize linear actuator servo motor
  lin_act.attach(SERVO_PIN);
  lin_act.setMinimumPulse(832);
}

void actStepTo(int pos) {
  //Function for changing PWM of servo motor to reach desired postion (pos).
  //pos is a value between 0 and 180 degrees.
  lin_act.write(pos);
  delay(SERVO_UPDATE_DELAY);
  SoftwareServo::refresh();
}

void openTag() {
  //Function to open tag using servo motor.
  int start_angle = 150;
  int end_angle = 70;

  for(int pos = start_angle; pos >= end_angle; pos -= 1) {
    actStepTo(pos);
  }
  delay(500);
  for(int pos = end_angle; pos <= start_angle; pos += 1) {
    actStepTo(pos);
  }
}

void loop() {
  if (tiny_switch.available()) {
    //Non-zero code received over RF. Fetch it here
    uint16_t code = tiny_switch.getReceivedValue();

    //Move linear actuator (servo motor) if expected code is received
    if (code == EXPECTED_CODE) {
      openTag();
    }
     
    tiny_switch.resetAvailable();
  }
}
