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


TMR_Reader r, *rp;
TMR_Status ret;
TMR_ReadPlan plan;
TMR_Region region;
int readpower = 1500;
uint8_t i;
uint8_t buffer[20];
uint8_t *antennaList = NULL;
uint8_t antennaCount = 0x0;
TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;
char string[100];
TMR_String model;

// This is a function to exit the application based on an error
void error_exit(int exitval, const char *fmt, ...)
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
  }

  token = strtok(args, str);
  if (NULL == token)
  {
    fprintf(stdout, "Missing argument after %s\n", args);
  }

  while(NULL != token)
  {
      scans = sscanf(token, "%"SCNu8, &antenna[i]);
    if (1 != scans)
    {
      fprintf(stdout, "Can't parse '%s' as an 8-bit unsigned integer value\n", token);
    }
    i++;
    token = strtok(NULL, str);
  }
  *antennaCount = i;

}

void reader_init() {


    uint8_t buffer[20];
    uint8_t *antennaList = NULL;
    uint8_t antennaCount = 0x0;

    parseAntennaList(buffer, &antennaCount, "1");
    antennaList = buffer;

    ret = TMR_create(rp, "tmr:///dev/ttyUSB0");
    checkerr(rp, ret, 1, "creating reader");

    ret = TMR_connect(rp);
    checkerr(rp, ret, 1, "Connecting reader");
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
    model.value = string;
    model.max   = sizeof(string);
    TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);
    checkerr(rp, ret, 1, "Getting version model");

    if (0 != strcmp("M3e", model.value))
    {
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

    }

    ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &readpower);
    checkerr(rp, ret, 1, "setting read power");
    ret = TMR_paramSet(rp, TMR_PARAM_METADATAFLAG, &metadata);
    checkerr(rp, ret, 1, "Setting Metadata Flags");

    /**
    * for antenna configuration we need two parameters
    * 1. antennaCount : specifies the no of antennas should
    *    be included in the read plan, out of the provided antenna list.
    * 2. antennaList  : specifies  a list of antennas for the read plan.
    **/
    // initialize the read plan
    if (0 != strcmp("M3e", model.value))
    {
      ret = TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);
    }
    else
    {
      ret = TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_ISO14443A, 1000);
    }
    checkerr(rp, ret, 1, "initializing the  read plan");

    /* Commit read plan */
    ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
    checkerr(rp, ret, 1, "setting read plan");
}
