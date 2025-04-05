# Compiler and flags for GUI and RFID system
CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0) -g -fPIC -I./include
LDFLAGS = -ldl -lm -lmercuryapi -lpthread
DEFINES = -D TMR_ENABLE_SERIAL_READER_ONLY=1
LIBS = $(shell pkg-config --libs gtk+-3.0)

# Compiler and flags for actuator transmission code
CC_TX = g++
DEFINES_TX = -D RPI
LDFLAGS_TX = -lwiringPi

# Source files
RFID_SRC = DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c 
GUI_SRC = gui_utils.c tagsense1.c
TX_SRC = ../../rc-switch/RCSwitch.cpp tx.cpp

# Object files
#OBJ = $(RFID_SRC:.c=.o)

# Output executables
TARGET = tagsense1
TARGET_TX = tx

all: $(TARGET) $(TARGET_TX)

# Compilation rule for GUI and RFID system
$(TARGET): $(RFID_SRC) $(GUI_SRC)
	$(CC) $(CFLAGS) $(DEFINES) $(RFID_SRC) $(GUI_SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Compilation rule for actuator transmission code
$(TARGET_TX): $(TX_SRC)
	$(CC_TX) $(DEFINES_TX) $(TX_SRC) -o $(TARGET_TX) $(LDFLAGS_TX)

# Clean rule to remove object files and executables
clean:
	rm -f $(TARGET)
	rm -f $(TARGET_TX)

