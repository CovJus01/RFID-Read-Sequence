CC=g++
CFLAGS=$(shell pkg-config --cflags gtk+-3.0) -D RPI
LIBS=$(shell pkg-config --libs gtk+-3.0)
LDFLAGS = -lwiringPi
TARGET=ui-test
SRC=ui-test.c actuator_signal_send.cpp /home/pi/actuator_signaling/rc-switch/RCSwitch.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

