#include "stdint.h"
#include <stdio.h>
#include <wiringPi.h>

#define SERIAL_PIN 18 //GPIO pin 18
#define BIT_DELAY 5 //milliseconds

uint16_t readSerialInt() {
	while (digitalRead(SERIAL_PIN) == 1); //wait for start bit
	delay(BIT_DELAY); //sync with sender

	// Receive code, bit by bit
	uint16_t code = 0;
	for (int i=0; i<16; i++) {
		code |= (digitalRead(SERIAL_PIN) << i);
		delay(BIT_DELAY);
	}

	// Read stop bit
	delay(BIT_DELAY);

	return code;
}

int main() {
	// Setup input pin
	wiringPiSetupGpio();
	pinMode(SERIAL_PIN, INPUT);

	// Read logic high or low on input pin
	while (1) {
		uint16_t received_code = readSerialInt();
		printf("Received code: %hu\n", received_code);
	}

	return 0;
}
	
