CC = gcc
LDFLAGS = -lwiringPi

SRC = rpi_serial.c
OBJ = $(SRC:.cpp=.o)
TARGET = rpi_serial

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)
