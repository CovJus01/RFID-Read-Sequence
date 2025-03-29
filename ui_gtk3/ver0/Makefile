# Compiler and flags
CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0) -g -fPIC -I./include
LDFLAGS = -ldl -lm -lmercuryapi -lpthread
DEFINES = -D TMR_ENABLE_SERIAL_READER_ONLY=1
LIBS = $(shell pkg-config --libs gtk+-3.0)

# Source files
SRC = DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c tagsense0.c

# Output executable
TARGET=tagsense0

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

