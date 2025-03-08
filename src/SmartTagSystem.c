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
#include "../c/src/api/serial_reader_imp.h"
#include "../c/src/api/tm_reader.h"
#include "../c/src/api/tmr_utils.h"

// Necesarry Includes
#include "RFID_UTILS.c"
#include "DB_utils.c"
#include "checkout_utils.c"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <conio.h>
#include "../dependencies/sqlite/sqlite-amalgamation-3490000/sqlite3.c"

/**
 * *****************************************************************
 * ************************* MAIN FUNCTION *************************
 * *****************************************************************
**/

int main(int argc, char *argv[]) {

    // Initialize reader variables
    reader_info reader_i;
    reader_info_ptr reader_i_ptr = &reader_i;

    // Initialize system variables
    int admin_request;
    char input;

    // Initialize Database variables
    sqlite3 *db;
    int sql_status;
    sql_status = sqlite3_open("RFID_SYSTEM_DB.db", &db);

    /** ************************************************************************
     * TM Reader Setup
     * ************************************************************************/

    // Initialize the RFID reader
    printf("INITIALIZING RFID READER...\n");
    initialize_reader(reader_i_ptr, argc, argv);

    // Connect the reader module
    printf("CONNECTING TO RFID READER...\n");
    connect_reader(reader_i_ptr);

    printf("Setup Complete!\n");

    /** ***********************************************************************/

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
        close_reader(reader_i_ptr);
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
        close_reader(reader_i_ptr);
        return 1;
    }
    /** ***********************************************************************/

    // USER ARRIVES
    // LISTEN FOR TAGS
    while (read_empty(reader_i_ptr)) {

        printf("LISTENING FOR TAGS...\n");

        //Read tags
        read(reader_i_ptr);

        //Do a 2 second delay in between reads
        delay(2);
    }

    printf("TAGS SENSED!!!\n");
    printf("FETCHING TAG DATA...\n");

    //GET TAG DATA
    sql_status = get_tags(db);


    // SUMMARIZE CHECKOUT INFORMATION
    printf("Checkout summary:\n");
    print_tags(reader_i_ptr);

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
    //READ TAGS
    read(reader_i_ptr);

    //USER SELECTS ITEM TO ASSIGN

    //UPDATE DATABASE WITH ASSIGNED ITEMS
    //LOOP OVER the FOLLOWING fucntion for the updating of value
    sql_status = update_tag_item(db);

    //Close all the different processes
    close_reader(reader_i_ptr);
    sqlite3_close(db);
    return 0;
}

