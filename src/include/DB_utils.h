//Header for DB_utils.c
#ifndef SQLITE3
#define SQLITE3
#include "sqlite3.h"
#endif


int create_table(sqlite3 * db);

int add_tag(sqlite3 * db, char tagID[]);

int update_tag_item(sqlite3 * db, char itemID[], char tagID[]);

int tag_deactivate(sqlite3 * db, char tagID[]);

int get_tag(sqlite3 * db, char tagID[]);