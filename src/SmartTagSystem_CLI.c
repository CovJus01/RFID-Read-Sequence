// CLI IMPLEMENTATION OF THE FULL SYSTEM.
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

//Initialized reader global variables, required memory for the whole process flow


extern TMR_Reader r, *rp;
extern TMR_Status ret;
extern TMR_ReadPlan plan;
extern TMR_Region region;
extern int readpower;
extern uint8_t i;
extern uint8_t buffer[20];
extern uint8_t *antennaList;
extern uint8_t antennaCount;
extern TMR_TRD_MetadataFlag metadata;
extern char string[100];
extern TMR_String model;
char input;
int admin_request;
int close_request = 0;
sqlite3 *db;

//RFID thread implementation for async reads
void* RFID_thread(void* vargp)
{
  do {

            printf("LISTENING FOR TAGS...\n");
            fflush(stdout);

          //Read tags
          ret = TMR_read(rp, 500, NULL);

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
          delay(2200);
      }while(TMR_SUCCESS != TMR_hasMoreTags(rp) && admin_request != 1 && close_request == 0);

    printf("Tags or Admin Request!\n");
    fflush(stdout);
}

// Async input thread
void* input_thread(void* vargp)
{
    delay(1000);
    printf("LISTENING FOR ADMIN REQUEST\n");
    fflush(stdout);
    scanf(" %c", &input);
    if(input == 'r'){
        admin_request = 1;
    }
    else if(input == 'x'){
        close_request = 1;
    }
}



int main(int argc, char *argv[]) {

    // Initialize system variables
    admin_request = 0;
    rp = &r;
    int system_status;
    // Initialize Database variables
    int sql_status;
    sql_status = sqlite3_open("RFID_SYSTEM_DB.db", &db);

    // Initialize the reader
    reader_init();

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

    pthread_t RFID_scan;
    pthread_t input_scan;

    //BEGIN SYSTEM LOOP
    while(1) {

        //Listen for User and Listen for admin requests
        pthread_create(&RFID_scan, NULL, RFID_thread, NULL);
        pthread_create(&input_scan, NULL, input_thread, NULL);



        pthread_join(RFID_scan, NULL);
        pthread_join(input_scan, NULL);

        if(admin_request == 1) {

            //Go down admin path
            admin_request = 0;
            system_status = authenticate();

            //After proper authentication, request user input and perform task
            if(system_status == 1) {
                handle_admin();
            }

        }
        else if(close_request ==1) {
            break;
        }
        else {
            //Go down scanned path
            admin_request = 0;
            handle_checkout();
        }
    }


    sqlite3_close(db);
    TMR_destroy(rp);
    return 0;
}

