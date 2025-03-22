#include <stdio.h>
#include "/home/pi/actuator_signaling/rc-switch/RCSwitch.h" //must clone git project rc-switch to appropriate location

#define GPIO_PIN 17
#define SEND_LEN_BITS 16

int send_actuator_code_16bit(uint16_t code);
