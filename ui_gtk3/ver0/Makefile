# Compiler and flags
CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk+-3.0) -g -fPIC -I./include
LDFLAGS = -ldl -lm -lmercuryapi -lpthread
DEFINES = -D TMR_ENABLE_SERIAL_READER_ONLY=1
LIBS = $(shell pkg-config --libs gtk+-3.0)

# Source files
RFID_SRC = DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c 
GUI_SRC = tagsense0.c

# Object files
OBJ = $(RFID_SRC:.c=.o)

# Output executable
TARGET=tagsense0

all: $(TARGET)

# Compilation rule
$(TARGET): $(RFID_SRC) $(GUI_SRC)
	$(CC) $(CFLAGS) $(DEFINES) $(RFID_SRC) $(GUI_SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Clean rule to remove object files and executables
clean:
	rm -f $(TARGET)

