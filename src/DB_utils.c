// A set of various different functions to interact with the database
#include "../dependencies/sqlite/sqlite-amalgamation-3490000/sqlite3.c"


// Function to create a table if it doesnt exist
int create_table(sqlite3 * db) {

    //Initialize required vars
    int status;
    sqlite3_stmt * statement_ptr;

    //First check for a table ******* FIND LOGIC TO DO THIS

    status = sqlite3_prepare_v2(db,"CREATE TABLE Tags(tagID CHAR(50), itemID CHAR(20), status BIT(1));", -1, &statement_ptr, NULL);

    //This creates the database if it has not been initialized
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}


// A function to add a tag to the DB
int add_tag(sqlite3 * db) {

    int status;
    sqlite3_stmt * statement_ptr;

    //NEED to work out logic to add tag ID into this string
    status = sqlite3_prepare_v2(db,"INSERT INTO Tags VALUES(tagID, NULL, 0)", -1, &statement_ptr, NULL);

    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

// A function to update a tag itemID
int update_tag_item(sqlite3 * db) {

    int status;
    sqlite3_stmt * statement_ptr;

    //Also add the setting of status bit to high
    status = sqlite3_prepare_v2(db,"UPDATE Tags SET itemID = {INSERT itemID HERE} WHERE tagID = {ID}", -1, &statement_ptr, NULL);
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

// A function that deactivates the tag
int tag_deactivate(sqlite3 * db) {

    int status;
    sqlite3_stmt * statement_ptr;

    //Add logic to remove the itemID from it too
    status = sqlite3_prepare_v2(db,"UPDATE Tags SET status = 0 WHERE tagID = {ID}", -1, &statement_ptr, NULL);

    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

//A function to retrieve the tag details
int get_tag(sqlite3 * db) {

    int status;
    sqlite3_stmt * statement_ptr;

    status = sqlite3_prepare_v2(db,"SELECT FROM Tags WHERE tagID = {ID}", -1, &statement_ptr, NULL);

    //Figure out what to do with the retrieved data
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;
}

//A function to retrieve multiple tag details
int get_tags(sqlite3 * db) {

    int status;
    sqlite3_stmt * statement_ptr;

    // ADD TAGID_LIST to string
    status = sqlite3_prepare_v2(db,"SELECT FROM Tags WHERE tagID in (TAGID_LIST)", -1, &statement_ptr, NULL);

    //Figure out what to do with the retrieved data
    //Most likely will need to loop over the step function and parse data
    status = sqlite3_step(statement_ptr);
    status = sqlite3_finalize(statement_ptr);

    return status;

}

