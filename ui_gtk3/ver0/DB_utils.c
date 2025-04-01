// A set of various different functions to interact with the database
#include "sqlite3.h"
#include "DB_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Function to create a table if it doesnt exist
int create_table(sqlite3 * db) {

    //Initialize required vars
    int status;
    sqlite3_stmt * statement_ptr;

    //First check for a table ******* FIND LOGIC TO DO THIS

    status = sqlite3_prepare_v2(db,"CREATE TABLE Tags(tagID CHAR(50), itemID CHAR(20), status BIT(1));", -1, &statement_ptr, 0);

    //This creates the database if it has not been initialized
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}


// A function to add a tag to the DB
int add_tag(sqlite3 * db, char tagID[]) {

    int status;
    sqlite3_stmt * statement_ptr;
    char command[200];

    strcpy(command,"INSERT INTO Tags (tagID, itemID, status) VALUES('");
    strcat(command, tagID);
    strcat(command, "', '0', 0)");

    //NEED to work out logic to add tag ID into this string
    status = sqlite3_prepare_v2(db, command, -1, &statement_ptr, 0);

    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

// A function to update a tag itemID
int update_tag_item(sqlite3 * db, char itemID[], char tagID[]) {

    int status;
    sqlite3_stmt * statement_ptr;
    char command[200];

    strcpy(command, "UPDATE Tags SET itemID = '");
    strcat(command, itemID);
    strcat(command, "'WHERE tagID ='");
    strcat(command, tagID);
    strcat(command, "'");


    //Also add the setting of status bit to high
    status = sqlite3_prepare_v2(db, command, -1, &statement_ptr, 0);
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

// A function that deactivates the tag
int tag_deactivate(sqlite3 * db, char tagID[]) {

    int status;
    sqlite3_stmt * statement_ptr;
    char command[200];

    strcpy(command,"UPDATE Tags SET status = 0 WHERE tagID ='");
    strcat(command, tagID);
    strcat(command, "'");

    //Add logic to remove the itemID from it too
    status = sqlite3_prepare_v2(db, command, -1, &statement_ptr, 0);

    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

//A function to retrieve the tag details
void get_tag(sqlite3 * db, char tagID[], char* itemID,int buffersize) {

    int status;
    sqlite3_stmt * statement_ptr;
    char command[200];

    strcpy(command,"SELECT tagID,itemID,status FROM Tags WHERE tagID = '");
    strcat(command, tagID);
    strcat(command, "'");

    status = sqlite3_prepare_v2(db,command, -1, &statement_ptr, 0);

    //Figure out what to do with the retrieved data
    status = sqlite3_step(statement_ptr);

	if (status == SQLITE_ROW) {
		const char *tagID_ret = sqlite3_column_text(statement_ptr, 0);
		const char *itemID_ret = sqlite3_column_text(statement_ptr, 1);
		int status_ret = sqlite3_column_int(statement_ptr,2);
		strncpy(itemID, itemID_ret, buffersize-1);
		status = sqlite3_finalize(statement_ptr);

		itemID[buffersize-1] = '\0';
	} 
	else {
		strncpy(itemID, "0", buffersize-1);
		itemID[buffersize-1] = '\0';
	}		
}

int get_item(sqlite3* db, char* itemID, Item* item) {

    int status;
    sqlite3_stmt * statement_ptr;
    char command[200];

    // Retrieve Item Data
    strcpy(command,"SELECT itemID,itemDescription,price FROM Items WHERE itemID = '");
    strcat(command, itemID);
    strcat(command, "'");

    status = sqlite3_prepare_v2(db,command, -1, &statement_ptr, 0);

    status = sqlite3_step(statement_ptr);

	//Pack item information into Item struct instance
	const unsigned char *description = sqlite3_column_text(statement_ptr,1);
	item->description = malloc(strlen((const char*)description) + 1);
	strcpy((char*)item->description, (const char*)description);
    item->price = sqlite3_column_int(statement_ptr,2);

    status = sqlite3_finalize(statement_ptr);

    return 0;
}
