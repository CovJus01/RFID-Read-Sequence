/* ********************************************
 * Includes
 * ********************************************/
//For GTK3 (apt package libgtk-3-dev)
#include <gtk/gtk.h>

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
#include <unistd.h> //for sleep

/* ********************************************
 * Global variables
 * ********************************************/
//Global GUI variables to work with threading
GtkWidget *stack;
GtkListStore *checkout_store;

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

/* ********************************************
 * Function definitions
 * ********************************************/
//Function to add item to the checkout list
void addCheckoutItem(GtkListStore *checkout_store, int quantity, const gchar *name, double price) {
	GtkTreeIter iter;
	gtk_list_store_append(checkout_store, &iter);
	gtk_list_store_set(checkout_store, &iter,
						0, quantity,
						1, name,
						2, price,
						-1);
}

//Function to add item to the checkout list with predefined values
gboolean addCheckoutItemValues(gpointer data) {
	GtkTreeIter iter;
	gtk_list_store_append(checkout_store, &iter);
	gtk_list_store_set(checkout_store, &iter,
						0, 1,
						1, "item from thread",
						2, 10.50,
						-1);

	return FALSE; //removes function from the main loop after execution
}

//Function to clear checkout list before a new scan
void clearCheckout(GtkListStore *checkout_store) {
	gtk_list_store_clear(GTK_LIST_STORE(checkout_store));
}

//Define thread worker to be called when reading RFID tags
void *RFIDReadWorker(void *param) {
	printf("listening for tags...\n");
	ret = TMR_read(rp, 1500, NULL);

	while (TMR_SUCCESS == TMR_hasMoreTags(rp))
	{
		TMR_TagReadData trd;
		char idStr[128];
		char itemID[20];

		ret = TMR_getNextTag(rp, &trd);
		checkerr(rp, ret, 1, "fetching tag");

		TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

		get_tag(db, idStr, itemID, sizeof(itemID));
		Item* new_item = get_item(db, itemID);

		//Add item to checkout store
		addCheckoutItem(checkout_store, 1, new_item->description, new_item->price);

		//Free memory of read item
		if (new_item->description) 
			free(new_item->description);

		free(new_item);
	}
}

//Deconstructor callback function to clear memory when UI is closed
void clearMemoryOnClose(GtkWidget *widget, gpointer data) {
	if (db)
		sqlite3_close(db);
	if (rp)
		TMR_destroy(rp);
	
	gtk_main_quit();
}

//Button callback function to switch to checkout page
void openCheckoutPage(GtkWidget *widget, gpointer stack) {
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "checkout");

	//Start an RFID read thread when the checkout page opens
	pthread_t rfid_read_thread;
	pthread_create(&rfid_read_thread, NULL, RFIDReadWorker, NULL);
}

//Button callback function to switch to startup page
void openStartupPage(GtkWidget *widget, gpointer stack) {
	//Clear checkout list
	clearCheckout(checkout_store);

	//Open startup page
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "startup");
}

/* ********************************************
 * Main function
 * ********************************************/
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

	printf("RFID reader initialized\n");

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

	gtk_init(&argc, &argv);

	//Create main window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "TagSense v0");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(clearMemoryOnClose), NULL);

	//Create a vertical box layout
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	//Create a GtkStackSwitcher
	GtkWidget *stack_switcher = gtk_stack_switcher_new();
	gtk_box_pack_start(GTK_BOX(vbox), stack_switcher, FALSE, FALSE, 0);

	//Create a GtkStack
	stack = gtk_stack_new();
	gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
	gtk_stack_set_transition_duration(GTK_STACK(stack), 500);
	gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0);

	/* ************** 
	* startup page
	***************** */
	GtkWidget *startup_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	//Create start button
	GtkWidget *button_start = gtk_button_new_with_label("Start");

	//Create label
	GtkWidget *label1 = gtk_label_new("startup page");
	g_signal_connect(button_start, "clicked", G_CALLBACK(openCheckoutPage), stack);

	//Pack startup page
	gtk_box_pack_start(GTK_BOX(startup_page), button_start, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(startup_page), label1, TRUE, TRUE, 0);
	gtk_stack_add_named(GTK_STACK(stack), startup_page, "startup");

	/* ************** 
	* checkout page
	***************** */
	GtkWidget *checkout_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	//Create top bar with back button
	GtkWidget *checkout_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *button_back = gtk_button_new_with_label("Back");
	g_signal_connect(button_back, "clicked", G_CALLBACK(openStartupPage), stack);
	gtk_widget_set_halign(button_back, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(checkout_top_bar), button_back, FALSE, FALSE, 5);

	//Create checkout list store
	checkout_store = gtk_list_store_new(3, G_TYPE_INT,  G_TYPE_STRING, G_TYPE_DOUBLE);

	//Create checkout tree view
	GtkWidget *checkout_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(checkout_store));

	//Add columns
	GtkCellRenderer *checkout_renderer;
	GtkTreeViewColumn *checkout_col;

	//Define quantity column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Quantity", checkout_renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);
	
	//Define name column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Name", checkout_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);

	//Define price column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Price ($)", checkout_renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);

	//Pack checkout page
	GtkWidget *checkout_scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(checkout_scroll_window), checkout_tree_view);
	gtk_box_pack_start(GTK_BOX(checkout_page), checkout_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(checkout_page), checkout_scroll_window, TRUE, TRUE, 5);

	gtk_stack_add_named(GTK_STACK(stack), checkout_page, "checkout");

	//Link stack switcher with stack
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));

	//Show everything
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

