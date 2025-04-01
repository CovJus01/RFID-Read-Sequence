//Header for DB_utils.c
#ifndef SQLITE3
#define SQLITE3
#include "sqlite3.h"
#endif

//Define struct for returning item data
typedef struct {
	unsigned char *description;
	int price;
} Item;

//Function prototypes
int create_table(sqlite3 * db);

int add_tag(sqlite3 * db, char tagID[]);

int update_tag_item(sqlite3 * db, char itemID[], char tagID[]);

int tag_deactivate(sqlite3 * db, char tagID[]);

void get_tag(sqlite3 * db, char tagID[], char * itemID, int buffersize);

int get_item(sqlite3 * db, char * itemID, Item* item);

