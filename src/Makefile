# IDIR =./include
# CC=gcc

# ODIR=obj
# LDIR =./lib

# LIBS=-lmercuryapi -lreadline -pthread

# CFLAGS=-I$(IDIR) -Wall -Werror

# _DEPS = RFID_Utils.h
# _DEPS += DB_utils.h
# _DEPS += checkout_utils.h
# _DEPS += sqlite3.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# _OBJ = sqlite3.o DB_utils.o checkout_utils.o RFID_Utils.o SmartTagSystem.o
# OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


# $(ODIR)/%.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

# SmartTagSystem: $(OBJ)
# 	$(CC) -D TMR_ENABLE_SERIAL_READER_ONLY=1 -g -fPIC -o $@ $^ $(CFLAGS) $(LIBS)

# read: read.o
# 	$(CC) -D TMR_ENABLE_SERIAL_READER_ONLY=1 -g -fPIC -o read read.o $(CFLAGS) $(LIBS)

# .PHONY: clean

# clean:
# 	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
SmartTagSystem:
	gcc -D TMR_ENABLE_SERIAL_READER_ONLY=1 -g -fPIC -I./include -o SmartTagSystem SmartTagSystem.c DB_utils.c checkout_utils.c RFID_Utils.c sqlite3.c -lmercuryapi -lreadline -lpthread