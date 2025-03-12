/**
*
* Smart Tag Checkout System
* Group 20
*
* This is the main system file. It controls the various different processes
* and interactions between them. It will also consist of the system config,
* system syncronization and various other logical sequences needed to run the
* system effectively. The various parts of the system include th RFID tag
* reading, the database updates, the checkout UI and the Tx signal output.
*
* The RFID system uses the MercuryAPI to configure the ThinkMagic RFID module.
* We use the SparkFun integration in the m7e to make the communication and
* setup more fluid.
*
* This will be following the system flow diagram for the checkout system
* added changes will be registered and updated there.
* */

//API Includes
#include "serial_reader_imp.h"
#include "tm_reader.h"
#include "tmr_utils.h"

// Necesarry Includes
#include "RFID_Utils.h"
#include "DB_utils.h"
#include "checkout_utils.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include "sqlite3.h"
#include <pthread.h>

/**
 * *****************************************************************
 * ************************* MAIN FUNCTION *************************
 * *****************************************************************
**/

#define usage() {error_exit(1, "Please provide valid reader URL, such as: reader-uri [--ant n] [--pow read_power]\n"\
                         "reader-uri : e.g., 'tmr:///COM1' or 'tmr:///dev/ttyS0/' or 'tmr://readerIP'\n"\
                         "[--ant n] : e.g., '--ant 1'\n"\
                         "[--pow read_power] : e.g, '--pow 2300'\n"\
                         "Example for UHF modules: 'tmr:///com4' or 'tmr:///com4 --ant 1,2' or 'tmr:///com4 --ant 1,2 --pow 2300'\n"\
                         "Example for HF/LF modules: 'tmr:///com4' \n");}

// void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
// {
//   if (TMR_SUCCESS != ret)
//   {
//     error_exit(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
//   }
// } 
int admin_request;
TMR_Reader r, *rp;
TMR_Status ret;
char input;

void* RFID_thread(void* vargp)
{
  do {

          printf("LISTENING FOR TAGS...\n");

          //Read tags
          ret = TMR_read(rp, 1000, NULL);

          if (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)
          {
            /* In case of TAG ID Buffer Full, extract the tags present
            * in buffer.
            */
            #ifndef BARE_METAL
                fprintf(stdout, "reading tags:%s\n", TMR_strerr(rp, ret));
            #endif /* BARE_METAL */
          }
          else
          {
            checkerr(rp, ret, 1, "reading tags");
          }

          //Do a 0.5 second delay in between reads
          delay(500);
      }while(TMR_SUCCESS != TMR_hasMoreTags(rp) && admin_request != 1); 

    printf("Tags or Admin Request! Hit enter to continue...\n");
}

void* input_thread(void* vargp)
{
  printf("LISTENING FOR ADMIN REQUEST\n");
  scanf("%c", &input);
  if(input == 'r'){
    admin_request = 1;
  }
}



int main(int argc, char *argv[]) {
  
    // Initialize system variables
    admin_request = 0;
    // Initialize Database variables
    sqlite3 *db;
    int sql_status;
    sql_status = sqlite3_open("RFID_SYSTEM_DB.db", &db);

    /** ************************************************************************
     * TM Reader Setup
     * ************************************************************************/

    TMR_ReadPlan plan;
    TMR_Region region;
  #define READPOWER_NULL (-12345)
    int readpower = READPOWER_NULL;
  #ifndef BARE_METAL
    uint8_t i;
  #endif /* BARE_METAL*/
    uint8_t buffer[20];
    uint8_t *antennaList = NULL;
    uint8_t antennaCount = 0x0;
    TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;
    char string[100];
    TMR_String model;
  #if USE_TRANSPORT_LISTENER
    TMR_TransportListenerBlock tb;
  #endif /* USE_TRANSPORT_LISTENER */
    rp = &r;

  #ifndef BARE_METAL
    if (argc < 2)
    {
      fprintf(stdout, "Not enough arguments.  Please provide reader URL.\n");
      usage();
    }

    for (i = 2; i < argc; i+=2)
    {
      if(0x00 == strcmp("--ant", argv[i]))
      {
        if (NULL != antennaList)
        {
          fprintf(stdout, "Duplicate argument: --ant specified more than once\n");
          usage();
        }
        parseAntennaList(buffer, &antennaCount, argv[i+1]);
        antennaList = buffer;
      }
      else if (0 == strcmp("--pow", argv[i]))
      {
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
    ret = TMR_create(rp, argv[1]);
    checkerr(rp, ret, 1, "creating reader");
  #else
    ret = TMR_create(rp, "tmr:///com1");

  #ifdef TMR_ENABLE_UHF
    buffer[0] = 1;
    antennaList = buffer;
    antennaCount = 0x01;
  #endif /* TMR_ENABLE_UHF */
  #endif /* BARE_METAL */

  #if USE_TRANSPORT_LISTENER
    if (TMR_READER_TYPE_SERIAL == rp->readerType)
    {
      tb.listener = serialPrinter;
    }
    else
    {
      tb.listener = stringPrinter;
    }
    tb.cookie = stdout;

    TMR_addTransportListener(rp, &tb);
  #endif /* USE_TRANSPORT_LISTENER */

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
    }

  #ifdef TMR_ENABLE_LLRP_READER
    if (0 != strcmp("Mercury6", model.value))
  #endif /* TMR_ENABLE_LLRP_READER */
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

    /** ***********************************************************************/
    // END OF TMR SETUP
    /** ************************************************************************
     * SQLite DB setup
     * ************************************************************************/


    if(sql_status != SQLITE_OK) {

        //Get error
        const char * errmsg = sqlite3_errmsg(db);
        printf("ERROR OPENING DATABASE, ERROR:\n\n %s\n", errmsg);
        printf("SHUTTING DOWN\n");

        //Close system
        sqlite3_close(db);
        return 1;
    }

    // Create the table if it is not created yet
    sql_status = create_table(db);
    if (sql_status != SQLITE_OK){

        //Get error
        const char * errmsg = sqlite3_errmsg(db);
        printf("ERROR CREATING TABLE, ERROR:\n\n %s\n", errmsg);
        printf("SHUTTING DOWN\n");

        //Close system
        sqlite3_close(db);
        return 1;
    }
    /** ***********************************************************************/

    // USER ARRIVES
    // LISTEN FOR TAGS
    pthread_t RFID_scan;
    pthread_t input_scan;


    pthread_create(&RFID_scan, NULL, RFID_thread, NULL);
    pthread_create(&input_scan, NULL, input_thread, NULL);



    pthread_join(RFID_scan, NULL);
    pthread_join(input_scan, NULL);

    printf("TAGS SENSED!!!\n");
    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
        TMR_TagReadData trd;
        char idStr[128];
        char timeStr[128];


        ret = TMR_getNextTag(rp, &trd);
        checkerr(rp, ret, 1, "fetching tag");

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

        TMR_getTimeStamp(rp, &trd, timeStr);
        printf("Tag ID: %s\n", idStr);
    }


    printf("FETCHING TAG DATA...\n");

    //GET TAG DATA
    sql_status = get_tags(db);


    // SUMMARIZE CHECKOUT INFORMATION
    printf("Checkout summary:\n");


    // COMPLETE CHECKOUT + UPDATE DATABASE
    while(1){
        printf("Would you like to proceed with the purchase?(y/n):\n");
        scanf("%c", &input);

        if(input == 'y'){
            printf("COMPLETING PURCHASE...\n");
            //complete_purchase();
            printf("PURCHASE COMPLETE!!!\n");
            //update_database();
            //^^^^This function should just loop over the tag array with the following function
            sql_status = tag_deactivate(db);
            printf("DATABASE UPDATED.\n");

            // SEND RELEASE SIGNAL

            /**
             * ADD RELEASE LOGIC HERE
             **/
            break;

        }else if (input == 'n') {

            printf("CANCELLING PURCHASE...\n");
            break;

        }else {
            printf("INVALID INPUT\n");
        }

    }


    /**
    *   PATH 2, USER REQUESTS ADMIN ACCESS
    **/

    //AUTHORIZE USER
    //
    // //READ TAGS
    // RFID_read(reader_i_ptr);

    // //USER SELECTS ITEM TO ASSIGN

    // //UPDATE DATABASE WITH ASSIGNED ITEMS
    // //LOOP OVER the FOLLOWING fucntion for the updating of value
    // sql_status = update_tag_item(db);

    //Close all the different processes
    // close_reader(reader_i_ptr);
    sqlite3_close(db);
    TMR_destroy(rp);
    return 0;
}

