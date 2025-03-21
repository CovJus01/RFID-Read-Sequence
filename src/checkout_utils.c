// A file that contains random tools for the checkout system
#include <time.h>
#include "checkout_utils.h"
#include "DB_utils.h"
#include "RFID_Utils.h"
#include "serial_reader_imp.h"
#include "tm_reader.h"
#include "tmr_utils.h"
#include <stdio.h>

extern TMR_Reader r, *rp;
extern TMR_Status ret;
extern sqlite3 *db;


void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;

    // Storing start time
    clock_t start_time = clock();

    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds);
}

int authenticate() {

    int input;
    delay(1000);
    printf("Please input the admin password:\n");
    fflush(stdout);
    delay(1000);
    scanf(" %d", &input);
    if(input == 1234){
        printf("Correct Password!\n");
        fflush(stdout);
        return 1;
    }else {
        printf("Incorrect Password!\n");
        fflush(stdout);
        return 0;
    }
}

void handle_admin() {

    char input;

    //Handle user input and send to correct path
    while(1){
        printf("Select the process you would like to complete:\n");
        printf("\tAdd new tags (a)\n\tAssign existing tags (u)\n\tExit Admin (x)\n");
        fflush(stdout);
        delay(1000);
        scanf(" %c", &input);

        if(input == 'a'){
            handle_add_tags();
            break;
        }
        else if(input == 'u'){
            handle_update_tags();
            break;
        }
        else if(input == 'x'){
            printf("Exiting Admin\n");
            fflush(stdout);
            break;
        }
        else{
            printf("INVALID INPUT\n");
            fflush(stdout);
        }
    }

}

void handle_add_tags(){

    char input;

    //Prepare the tags
    printf("Please place the tags in the bin that you would like to add...\n");
    printf("When ready, initiate the request py pressing a key and enter\n");
    fflush(stdout);
    delay(1000);
    scanf(" %c", &input);

    //Read the tags
    TMR_read(rp, 500, NULL);

    //Add the tags to the database as null values
    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
        TMR_TagReadData trd;
        char idStr[128];

        ret = TMR_getNextTag(rp, &trd);
        checkerr(rp, ret, 1, "fetching tag");

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);
        add_tag(db, idStr);
        printf("Tag added: %s", idStr);
    }

    printf("Tags added to database!\n");
    fflush(stdout);

}

void handle_update_tags(){

    char input;
    char itemID[10];

    //Prepare the tags
    printf("Please place the tags in the bin that you would like to update...\n");
    delay(1000);
    printf("When ready, initiate the request py pressing a key and enter\n");
    fflush(stdout);
    delay(1000);
    scanf(" %c", &input);

    // Determine the Item to change them to
    printf("Input the ItemID that you would like to assign them to");
    fflush(stdout);
    delay(1000);
    scanf(" %s", itemID);

    //Read the tags
    TMR_read(rp, 500, NULL);

    //Add the tags to the database as null values
    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
        TMR_TagReadData trd;
        char idStr[128];

        ret = TMR_getNextTag(rp, &trd);
        checkerr(rp, ret, 1, "fetching tag");

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);
        update_tag_item(db, itemID, idStr);
    }

    printf("Tags updated to itemID = %s\n", itemID);
    fflush(stdout);
}

void handle_checkout(){

    char input;
    int price = 0;
    //Collect Tag Data
    while (TMR_SUCCESS == TMR_hasMoreTags(rp))
    {
        TMR_TagReadData trd;
        char idStr[128];
        char itemID[20];

        ret = TMR_getNextTag(rp, &trd);
        checkerr(rp, ret, 1, "fetching tag");

        TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);
        get_tag(db, idStr, itemID, sizeof(itemID));
        price += get_item(db, itemID);
    }

    printf("Total: $%d\n", price);
    printf("Proceed with checkout?(y/n)\n");
    scanf(" %c", &input);

    if(input == 'y'){
        printf("Completed Purchase\n");
    }
    else{
        printf("Checkout Cancelled\n");
    }

}
