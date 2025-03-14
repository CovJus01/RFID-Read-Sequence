#include "../rc-switch/RCSwitch.h" //must clone git project rc-switch to appropriate location
//TODO add include of WiringPi (previously done with load flag Makefile)
//TODO probably need to add -D RPI flag to compile command so rc-switch works correctly (defines RPI macro)

#define GPIO_PIN 17
#define SEND_LEN_BITS 16

int send_actuator_code_16bit(uint16 code);