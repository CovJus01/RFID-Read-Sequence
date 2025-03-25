
# Compiler and flags
CC = gcc
CFLAGS = -g -fPIC -I./include
LDFLAGS = -ldl -lm -lmercuryapi -lreadline -lpthread
DEFINES = -D TMR_ENABLE_SERIAL_READER_ONLY=1

# Source files
COMMON_SRC = DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c
MAIN_SRC = SmartTagSystem.c
CLI_SRC = SmartTagSystem_CLI.c

# Object files
COMMON_OBJ = $(COMMON_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)
CLI_OBJ = $(CLI_SRC:.c=.o)

# Output executables
TARGET_MAIN = SmartTagSystem
TARGET_CLI = SmartTagSystem_CLI

# Default target (build both)
all: $(TARGET_MAIN) $(TARGET_CLI)

# Compilation rule for main system
$(TARGET_MAIN): $(MAIN_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(MAIN_OBJ) $(COMMON_OBJ) -o $(TARGET_MAIN) $(LDFLAGS)

# Compilation rule for CLI version
$(TARGET_CLI): $(CLI_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(CLI_OBJ) $(COMMON_OBJ) -o $(TARGET_CLI) $(LDFLAGS)

# Rule for compiling .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

# Clean rule to remove object files and executables
clean:
	$(RM) $(COMMON_OBJ) $(MAIN_OBJ) $(CLI_OBJ) $(TARGET_MAIN) $(TARGET_CLI)

.PHONY: all clean

