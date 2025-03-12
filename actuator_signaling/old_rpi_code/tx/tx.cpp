#include <stdio.h>
#include "../rc-switch/RCSwitch.h"

#define SEND_LEN 16 //bits

int main(int argc, char *argv[]) {
    int PIN = 17; //GPIO pin 17 
	uint16_t code = 11981;

	if (argc > 1) {
		code = atoi(argv[1]);
	}

	//Initialize WiringPi for BCM GPIO numbering scheme
	wiringPiSetupGpio();
    RCSwitch mySwitch = RCSwitch();

    // Transmitter is connected
    mySwitch.enableTransmit(PIN);

    // Optional set pulse length.
    mySwitch.setPulseLength(1000);
    
    // Optional set protocol (default is 1, will work for most outlets)
    //mySwitch.setProtocol(2);
    
    // Optional set number of transmission repetitions.
    mySwitch.setRepeatTransmit(5);

	printf("Sending code %d over %d bits\n", code, SEND_LEN);
	mySwitch.send(code, SEND_LEN);

    return 0;
}
