/**
*
* Group 20 - RFID_Utils Header
* Header file to support RFID_Utils.c
*
*
**/

#include "serial_reader_imp.h"
#include "tm_reader.h"
#include "tmr_utils.h"

typedef struct {
    TMR_Reader reader;
    TMR_Reader *reader_ptr;
    TMR_Status status;
    TMR_ReadPlan plan;
    TMR_Region region;
    int readpower; //This is a read power of 2000 cdBm which is 20dBm
    uint8_t *antennaList;
    uint8_t antennaCount;
    TMR_TRD_MetadataFlag metadata;
} reader_info, *reader_info_ptr;

void error_exit(int exitval, const char *fmt, ...);

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg);

void check_error(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg);

void parseAntennaList(uint8_t *antenna, uint8_t *antennaCount, char *args);

void initialize_reader(reader_info_ptr info_ptr, int argc, char* argv[]);

void connect_reader(reader_info_ptr info_ptr);

void RFID_read(TMR_Reader * rp);

int read_empty(TMR_Reader * rp);

void print_tags(reader_info_ptr info_ptr);

void close_reader(reader_info_ptr info_ptr);

void originalSetup( int argc, char* argv[]);
