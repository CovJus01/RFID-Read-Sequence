#include <stdio.h>
#include "../../rc-switch/RCSwitch.h"

#define TX_PIN 17 //Raspberry Pi GPIO pin for RF transmissions on 434.17 MHz
#define SEND_LEN 16 //bits

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Insufficient number of paramters given. Please pass transmit code as first arg.\n");
		return -1;
	}

	//Parse input code
	uint16_t code = atoi(argv[1]);

	//Initialize WiringPi for BCM GPIO numbering scheme
	wiringPiSetupGpio();

	//Enable Raspberry Pi as an RF transmitter
    RCSwitch mySwitch = RCSwitch();
    mySwitch.enableTransmit(TX_PIN);

    //Set transmit pulse length and number of Tx repetitions to increase robustness of sent signal
    mySwitch.setPulseLength(1000);
    mySwitch.setRepeatTransmit(5);

	//Send code
	printf("Sending code %d over %d bits\n", code, SEND_LEN);
	mySwitch.send(code, SEND_LEN);

    return 0;
}
