#include "actuator_signal_send.h"

int send_actuator_code_16bit(uint16_t code) {
    int PIN = 17; //GPIO pin 17 

	//Initialize WiringPi for BCM GPIO numbering scheme
	wiringPiSetupGpio();

    //Initialize RPi as radio controller
    RCSwitch mySwitch = RCSwitch();

    //Connect transmitter
    mySwitch.enableTransmit(GPIO_PIN);

    //Set pulse length.
    mySwitch.setPulseLength(1000);
    
    //Optional set protocol (default is 1, will work for most outlets)
    //mySwitch.setProtocol(2);
    
    //Set number of transmission repetitions.
    mySwitch.setRepeatTransmit(5);

    //Send code
	mySwitch.send(code, SEND_LEN_BITS);

    return 0;
}
