# Compiler and flags
CC = gcc
CFLAGS = -g -fPIC -I./include
LDFLAGS = -lmercuryapi -lreadline -lpthread

# Source files
SRC = SmartTagSystem.c DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c

# Object files
OBJ = $(SRC:.c=.o)

# Output executable
TARGET = SmartTagSystem

# Define the macro for conditional compilation
DEFINES = -D TMR_ENABLE_SERIAL_READER_ONLY=1

# Compilation rule
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Rule for compiling the .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

# Clean rule to remove object files and the target executable
clean:
	rm -f $(OBJ) $(TARGET)

# .PHONY is used to tell Make that 'clean' is not a real file
.PHONY: clean