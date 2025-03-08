//This file is meant to display the system process of connecting, initializing
//and reading RFID tags using the ThinkMagic Reader. Using the MercuryAPI
//we will be able to complete these things.


//API files
#include <serial_reader_imp.h>
#include <tm_reader.h>
#include <tmr_utils.h>

//Other
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#define PRINT_TAG_METADATA 0
#define numberof(x) (sizeof((x))/sizeof((x)[0]))

#define usage() {errx(1, "Please provide valid reader URL, such as: reader-uri [--ant n] [--pow read_power]\n"\
                         "reader-uri : e.g., 'tmr:///COM1' or 'tmr:///dev/ttyS0/' or 'tmr://readerIP'\n"\
                         "[--ant n] : e.g., '--ant 1'\n"\
                         "[--pow read_power] : e.g, '--pow 2300'\n"\
                         "Example for UHF modules: 'tmr:///com4' or 'tmr:///com4 --ant 1,2' or 'tmr:///com4 --ant 1,2 --pow 2300'\n"\
                         "Example for HF/LF modules: 'tmr:///com4' \n");}

void errx(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  exit(exitval);
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}


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



//Main Function

int main(int argc, char *argv[])
{
  TMR_Reader r, *rp;
  TMR_Status ret;
  TMR_ReadPlan plan;
  TMR_Region region;
#define READPOWER_NULL (-12345)
  int readpower = READPOWER_NULL;
  uint8_t i;
  uint8_t buffer[20];
  uint8_t *antennaList = NULL;
  uint8_t antennaCount = 0x0;
  TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;
  char string[100];
  TMR_String model;
  rp = &r;

  if (argc < 2)
  {
    fprintf(stdout, "Not enough arguments.  Please provide reader URL.\n");
    usage();
  }

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
    //Checks for the power argument
    else if (0 == strcmp("--pow", argv[i]))
    {
        //Conversion of requested read strength to integer value
      long retval;
      char *startptr;
      char *endptr;
      startptr = argv[i+1];
      retval = strtol(startptr, &endptr, 0);
      if (endptr != startptr)
      {
        readpower = retval;
        fprintf(stdout, "Requested read power: %d cdBm\n", readpower);
      }
      else
      {
        fprintf(stdout, "Can't parse read power: %s\n", argv[i+1]);
      }
    }
    else
    {
      fprintf(stdout, "Argument %s is not recognized\n", argv[i]);
      usage();
    }
  }

  //Creates an instance of a reader which holds the reader status
  ret = TMR_create(rp, argv[1]);
  checkerr(rp, ret, 1, "creating reader");

  //Attempts to connect to the reader, returns the status of the reader object
  ret = TMR_connect(rp);
  /* MercuryAPI tries connecting to the module using default baud rate of 115200 bps.
   * The connection may fail if the module is configured to a different baud rate. If
   * that is the case, the MercuryAPI tries connecting to the module with other supported
   * baud rates until the connection is successful using baud rate probing mechanism.
   */
  if((ret == TMR_ERROR_TIMEOUT) &&
     (TMR_READER_TYPE_SERIAL == rp->readerType))
  {
    uint32_t currentBaudRate;

    /* Start probing mechanism. */
    ret = TMR_SR_cmdProbeBaudRate(rp, &currentBaudRate);
    checkerr(rp, ret, 1, "Probe the baudrate");

    /* Set the current baudrate, so that
     * next TMR_Connect() call can use this baudrate to connect.
     */
    ret = TMR_paramSet(rp, TMR_PARAM_BAUDRATE, &currentBaudRate);
    checkerr(rp, ret, 1, "Setting baudrate");

    /* Connect using current baudrate */
    ret = TMR_connect(rp);
    checkerr(rp, ret, 1, "Connecting reader");
  }
  else
  {
    checkerr(rp, ret, 1, "Connecting reader");
  }
    //Initializes some parameter variables and gets them from the now connected Reader
  model.value = string;
  model.max   = sizeof(string);
  TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
  checkerr(rp, ret, 1, "Getting version model");

        //Gets region data
  region = TMR_REGION_NONE;
  ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
  checkerr(rp, ret, 1, "getting region");

  if (TMR_REGION_NONE == region)
  {
    TMR_RegionList regions;
    TMR_Region _regionStore[32];
    regions.list = _regionStore;
    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
    regions.len = 0;

    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
    checkerr(rp, ret, __LINE__, "getting supported regions");

    if (regions.len < 1)
    {
      checkerr(rp, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't support any regions");
    }

    region = regions.list[0];
    ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
    checkerr(rp, ret, 1, "setting region");
  }

  if (READPOWER_NULL != readpower)
  {
    int value;

    ret = TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value);
    checkerr(rp, ret, 1, "getting read power");
    printf("Old read power = %d dBm\n", value);

    value = readpower;
    ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &value);
    checkerr(rp, ret, 1, "setting read power");
  }

  {
    int value;
    ret = TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value);
    checkerr(rp, ret, 1, "getting read power");
    printf("Read power = %d dBm\n", value);
  }


  {
	// Set the metadata flags. Protocol is mandatory metadata flag and reader don't allow to disable the same
	// metadata = TMR_TRD_METADATA_FLAG_ANTENNAID | TMR_TRD_METADATA_FLAG_FREQUENCY | TMR_TRD_METADATA_FLAG_PROTOCOL;
	ret = TMR_paramSet(rp, TMR_PARAM_METADATAFLAG, &metadata);
	checkerr(rp, ret, 1, "Setting Metadata Flags");
  }

  /**
  * for antenna configuration we need two parameters
  * 1. antennaCount : specifies the no of antennas should
  *    be included in the read plan, out of the provided antenna list.
  * 2. antennaList  : specifies  a list of antennas for the read plan.
  **/
  // initialize the read plan
  ret = TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);
  checkerr(rp, ret, 1, "initializing the  read plan");

  /* Commit read plan */
  ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
  checkerr(rp, ret, 1, "setting read plan");

  ret = TMR_read(rp, 500, NULL);
  if (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)
  {
    /* In case of TAG ID Buffer Full, extract the tags present
    * in buffer.
    */
    fprintf(stdout, "reading tags:%s\n", TMR_strerr(rp, ret));
  }
  else
  {
    checkerr(rp, ret, 1, "reading tags");
  }

    //Print out Tag metadata
  while (TMR_SUCCESS == TMR_hasMoreTags(rp))
  {
    TMR_TagReadData trd;
    char idStr[128];
    char timeStr[128];

    ret = TMR_getNextTag(rp, &trd);
    checkerr(rp, ret, 1, "fetching tag");

    TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

  TMR_getTimeStamp(rp, &trd, timeStr);
  printf("Tag ID: %s ", idStr);
  }

  if (READPOWER_NULL != readpower)
      {
        int value;

        ret = TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value);
        checkerr(rp, ret, 1, "getting read power");
        printf("Old read power = %d dBm\n", value);

        value = readpower;
        ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &value);
        checkerr(rp, ret, 1, "setting read power");
      }

      {
        int value;
        ret = TMR_paramGet(rp, TMR_PARAM_RADIO_READPOWER, &value);
        checkerr(rp, ret, 1, "getting read power");
        printf("Read power = %d dBm\n", value);
      }
 TMR_destroy(rp);
  return 0;
}
