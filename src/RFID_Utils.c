/**
*
* Group 20 - RFID Utilities
* This program will have various functions related to the RFID system and the
* RFID reader control. These may include some functions to control the TM
* reader in a specific way or to perform specific use cases.
*
*
**/


//Other
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include "RFID_Utils.h"

#define PRINT_TAG_METADATA 0
#define numberof(x) (sizeof((x))/sizeof((x)[0]))

#define usage() {error_exit(1, "Please provide valid reader URL, such as: reader-uri [--ant n] [--pow read_power]\n"\
                         "reader-uri : e.g., 'tmr:///COM1' or 'tmr:///dev/ttyS0/' or 'tmr://readerIP'\n"\
                         "[--ant n] : e.g., '--ant 1'\n"\
                         "[--pow read_power] : e.g, '--pow 2300'\n"\
                         "Example for UHF modules: 'tmr:///com4' or 'tmr:///com4 --ant 1,2' or 'tmr:///com4 --ant 1,2 --pow 2300'\n"\
                         "Example for HF/LF modules: 'tmr:///com4' \n");}



// This is a function to exit the application based on an error
void error_exit(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  exit(exitval);
}

// This is a function that checks for errors in the reader
void check_error(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    error_exit(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    error_exit(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}

// This function parses the antenna list given by the reader
void parseAntennaList(uint8_t *antenna, uint8_t *antennaCount, char *args)
{
  char *token = NULL;
  char *str = ",";
  uint8_t i = 0x00;
  int scans;

  /* get the first token */
  if (NULL == args)
  {
    fprintf(stdout, "Missing argument\n");
    usage();
  }

  token = strtok(args, str);
  if (NULL == token)
  {
    fprintf(stdout, "Missing argument after %s\n", args);
    usage();
  }

  while(NULL != token)
  {
      scans = sscanf(token, "%"SCNu8, &antenna[i]);
    if (1 != scans)
    {
      fprintf(stdout, "Can't parse '%s' as an 8-bit unsigned integer value\n", token);
      usage();
    }
    i++;
    token = strtok(NULL, str);
  }
  *antennaCount = i;

}


void initialize_reader(reader_info_ptr info_ptr, int argc, char* argv[]) {



    TMR_Reader r;
    TMR_Reader *rp;
    TMR_Status ret;
    TMR_ReadPlan plan;
    int readpower = 1000; //This is a read power of 1000 cdBm which is 10dBm
    TMR_Region region = info_ptr->region;
    uint8_t i;
    uint8_t buffer[20];
    uint8_t *antennaList = NULL;
    uint8_t antennaCount = 0x0;
    TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;
    rp = &r;

    //This first checks the arguements and that there are enough, if not
    //it calls the usage function which describes the usage of the function
    if (argc < 2)
    {
        fprintf(stdout, "Not enough arguments.  Please provide reader URL.\n");
        usage();
    }


    //Handles all the argument variables
    for (i = 2; i < argc; i+=2)
    {

        //Check to see if the argument is the --ant argument
        if(0x00 == strcmp("--ant", argv[i]))
        {
            //If antenna list is already defined raise an error
          if (NULL != antennaList)
          {
            fprintf(stdout, "Duplicate argument: --ant specified more than once\n");
            usage();
          }
          parseAntennaList(buffer, &antennaCount, argv[i+1]);
          antennaList = buffer;
        }
        else
        {
          fprintf(stdout, "Argument %s is not recognized\n", argv[i]);
          usage();
        }
    }


    //Creates an instance of a reader which holds the reader status
    ret = TMR_create(rp, argv[1]);
    check_error(rp, ret, 1, "Creating reader object");

    printf("Pointer 1 = %p\n", rp);
    info_ptr->reader = r;
    info_ptr->reader_ptr = rp;
    info_ptr->status = ret;
    info_ptr->plan = plan;
    info_ptr->region = region;
    info_ptr->antennaCount = antennaCount;
    info_ptr->antennaList = antennaList;
    info_ptr->readpower = readpower;
    info_ptr->metadata = metadata;
}

void connect_reader(reader_info_ptr info_ptr) {

      TMR_String model;
      TMR_Reader * reader_ptr = info_ptr->reader_ptr;
      int power_NULL =  (-12345);
      char string[100];
      //Attempts to connect to the reader, returns the status of the reader object

      printf("Pointer 2 = %p\n", reader_ptr);
      info_ptr->status = TMR_connect(reader_ptr);
      /* MercuryAPI tries connecting to the module using default baud rate of 115200 bps.
       * The connection may fail if the module is configured to a different baud rate. If
       * that is the case, the MercuryAPI tries connecting to the module with other supported
       * baud rates until the connection is successful using baud rate probing mechanism.
       */
      if((info_ptr->status == TMR_ERROR_TIMEOUT) &&
         (TMR_READER_TYPE_SERIAL == reader_ptr->readerType))
      {
        uint32_t currentBaudRate;
        /* Start probing mechanism. */
        info_ptr->status = TMR_SR_cmdProbeBaudRate(reader_ptr, &currentBaudRate);
        check_error(reader_ptr, info_ptr->status, 1, "Probe the baudrate");

        /* Set the current baudrate, so that
         * next TMR_Connect() call can use this baudrate to connect.
         */
        info_ptr->status = TMR_paramSet(reader_ptr, TMR_PARAM_BAUDRATE, &currentBaudRate);
        check_error(reader_ptr, info_ptr->status, 1, "Setting baudrate");

        /* Connect using current baudrate */
        info_ptr->status = TMR_connect(reader_ptr);
        check_error(reader_ptr, info_ptr->status, 1, "Connecting reader");
      }
      else
      {
        check_error(reader_ptr, info_ptr->status, 1, "Connecting reader");
      }

        //Initializes some parameter variables and gets them from the now connected Reader
      model.value = string;
      model.max   = sizeof(string);
      TMR_paramGet(reader_ptr, TMR_PARAM_VERSION_MODEL, &model);
      check_error(reader_ptr, info_ptr->status, 1, "Getting version model");
            //Gets region data
      info_ptr->region = TMR_REGION_NONE;
      info_ptr->status = TMR_paramGet(reader_ptr, TMR_PARAM_REGION_ID, &info_ptr->region);
      check_error(reader_ptr, info_ptr->status, 1, "getting region");

      if (TMR_REGION_NONE == info_ptr->region)
      {
        TMR_RegionList regions;
        TMR_Region _regionStore[32];
        regions.list = _regionStore;
        regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
        regions.len = 0;

        info_ptr->status = TMR_paramGet(reader_ptr, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
        check_error(reader_ptr, info_ptr->status, __LINE__, "getting supported regions");

        if (regions.len < 1)
        {
          check_error(reader_ptr, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't support any regions");
        }

        info_ptr->region = regions.list[0];
        info_ptr->status = TMR_paramSet(reader_ptr, TMR_PARAM_REGION_ID, &info_ptr->region);
        check_error(reader_ptr, info_ptr->status, 1, "setting region");
      }

      if (power_NULL != info_ptr->readpower)
      {
        int value;

        info_ptr->status = TMR_paramGet(reader_ptr, TMR_PARAM_RADIO_READPOWER, &value);
        check_error(reader_ptr, info_ptr->status, 1, "getting read power");
        printf("Old read power = %d dBm\n", value);

        value = info_ptr->readpower;
        info_ptr->status = TMR_paramSet(reader_ptr, TMR_PARAM_RADIO_READPOWER, &value);
        check_error(reader_ptr, info_ptr->status, 1, "setting read power");
      }

      {
        int value;
        info_ptr->status = TMR_paramGet(reader_ptr, TMR_PARAM_RADIO_READPOWER, &value);
        check_error(reader_ptr, info_ptr->status, 1, "getting read power");
        printf("Read power = %d dBm\n", value);
      }

      {
        // Set the metadata flags. Protocol is mandatory metadata flag and reader don't allow to disable the same
        // metadata = TMR_TRD_METADATA_FLAG_ANTENNAID | TMR_TRD_METADATA_FLAG_FREQUENCY | TMR_TRD_METADATA_FLAG_PROTOCOL;
        info_ptr->status = TMR_paramSet(reader_ptr, TMR_PARAM_METADATAFLAG, &info_ptr->metadata);
        check_error(reader_ptr, info_ptr->status, 1, "Setting Metadata Flags");
      }

      /**
      * for antenna configuration we need two parameters
      * 1. antennaCount : specifies the no of antennas should
      *    be included in the read plan, out of the provided antenna list.
      * 2. antennaList  : specifies  a list of antennas for the read plan.
      **/
      // initialize the read plan
      info_ptr->status = TMR_RP_init_simple(&info_ptr->plan, info_ptr->antennaCount, info_ptr->antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);
      check_error(reader_ptr, info_ptr->status, 1, "initializing the  read plan");


      info_ptr->status = TMR_paramSet(reader_ptr, TMR_PARAM_READ_PLAN, &info_ptr->plan);
      check_error(reader_ptr, info_ptr->status, 1, "setting read plan");

}


void RFID_read(TMR_Reader * rp) {

    TMR_Status ret;
    
    if (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)
    {
      /* In case of TAG ID Buffer Full, extract the tags present
      * in buffer.
      */
      fprintf(stdout, "reading tags:%s\n", TMR_strerr(rp, ret));
    }
    else
    {
      check_error(rp, ret, 1, "reading tags");
    }
}

int read_empty(TMR_Reader * rp) {

    if(TMR_SUCCESS != TMR_hasMoreTags(rp)){
        return 1;
    }

    return 0;

}

void print_tags(reader_info_ptr info_ptr) {

  while (TMR_SUCCESS == TMR_hasMoreTags(info_ptr->reader_ptr))
  {
    TMR_TagReadData trd;
    char idStr[128];
    char timeStr[128];

    info_ptr->status = TMR_getNextTag(info_ptr->reader_ptr, &trd);
    check_error(info_ptr->reader_ptr, info_ptr->status, 1, "fetching tag");

    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

  TMR_getTimeStamp(info_ptr->reader_ptr, &trd, timeStr);
  printf("Tag ID: %s ", idStr);
  }

}

void close_reader(reader_info_ptr info_ptr) {
    TMR_destroy(info_ptr->reader_ptr);
}
