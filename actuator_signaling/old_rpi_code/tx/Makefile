CC = g++
CFLAGS = -D RPI
LDFLAGS = -lwiringPi

SRC = tx.cpp ../rc-switch/RCSwitch.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = tx

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)
