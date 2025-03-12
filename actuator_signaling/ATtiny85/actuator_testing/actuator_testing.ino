#include <SoftwareServo.h>

#define SERVO_PIN 0 //PB0 (physical pin 5)
#define SERVO_UPDATE_DELAY 10

SoftwareServo lin_act;

void setup() {
  //Run clock at the full 8MHz
  CLKPR = (1 << CLKPCE); //Enable changes to the clock prescaler
  CLKPR = 0; //Set the prescaler to 0 (no clk division)

  //Initialize linear actuator servo motor
  lin_act.attach(SERVO_PIN);
  lin_act.setMinimumPulse(832);
}

void stepTo(int pos) {
  lin_act.write(pos);
  delay(SERVO_UPDATE_DELAY);
  SoftwareServo::refresh();
}

void loop() {
  int start_angle = 0;
  int end_angle = 180;

  for(int pos = start_angle; pos <= end_angle; pos += 1) {
    stepTo(pos);
  }
  delay(3000);
  for(int pos = end_angle; pos >= start_angle; pos -= 1) {
    stepTo(pos);
  }
  delay(3000);
}
