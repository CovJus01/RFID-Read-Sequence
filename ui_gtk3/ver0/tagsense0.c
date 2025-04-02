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
 * Definitions
 * ********************************************/
 #define ADMIN_USER "admin"
 #define ADMIN_PASS "1234"

 //Struct to gather admin login information for button callback function
 typedef struct {
	GtkEntry *username;
	GtkEntry *password;
} LoginInfo;

/* ********************************************
 * Global variables
 * ********************************************/
//Global GUI variables
GtkWidget *stack; //gtk stack for multiple page GUI
GtkListStore *checkout_store; //gtk store for checkout page's item table
GtkListStore *admin_store; //gtk store for admin page's item table
Item *read_item; //struct pointer for item information pulled from database
GtkWidget *checkout_item_count_label; //label for item count in checkout page
GtkWidget *price_details_label; //label for price information in checkout page
int item_count; //count for checkout information
double total; //total final price for checkout information
LoginInfo *login_info; //struct pointer for admin login information

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
void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkScreen *screen = gdk_screen_get_default();
    gtk_css_provider_load_from_path(provider, "styles.css", NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);  // Free the provider when done
}

//Function to add item to the checkout table
void addCheckoutItem(GtkListStore *checkout_store, int quantity, const gchar *name, int price) {
	GtkTreeIter iter;
	gtk_list_store_append(checkout_store, &iter);
	gtk_list_store_set(checkout_store, &iter,
						0, quantity,
						1, name,
						2, price,
						-1);
}

//Function to add item to the admin table
void addAdminTableItem(GtkListStore *store, const gchar *item_id, const gchar *name, int price) {
	GtkTreeIter iter;
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
						0, item_id,
						1, name,
						2, price,
						-1);
}

//Function to clear checkout table
void clearCheckout(GtkListStore *store) {
	gtk_list_store_clear(GTK_LIST_STORE(store));
}

//Function to clear admin table
void clearAdminTable(GtkListStore *store) {
	gtk_list_store_clear(GTK_LIST_STORE(store));
}

void updateCheckoutItemCount(int count) {
	//Format a string for the new item count label
	char item_count_str[100];
	snprintf(item_count_str, sizeof(item_count_str),"Item Count: %d", count);

	//Update the checkout price details label
	gtk_label_set_text(GTK_LABEL(checkout_item_count_label), item_count_str);
}

//Function to update price details in checkout page price summary box
void updatePriceDetails(double subtotal, double tax, double total) {
	//Format a string for the new price details
	char price_details_str[100];
	snprintf(price_details_str, sizeof(price_details_str),"Subtotal: $%.2f\nTax: $%.2f\nTotal: $%.2f", subtotal, tax, total);

	//Update the checkout price details label
	gtk_label_set_text(GTK_LABEL(price_details_label), price_details_str);
}

//Define function to be called when reading RFID tags in checkout page
void readRFIDTagsInCheckout() {
	//Initialize variables for checkout summary
	item_count = 0;
	double subtotal = 0;
	total = 0;

	printf("listening for tags...\n");
	ret = TMR_read(rp, 500, NULL);

	while (TMR_SUCCESS == TMR_hasMoreTags(rp))
	{
		TMR_TagReadData trd;
		char idStr[128];
		char itemID[20];

		ret = TMR_getNextTag(rp, &trd);
		checkerr(rp, ret, 1, "fetching tag");

		TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

		get_tag(db, idStr, itemID, sizeof(itemID));

		//Filter out non-registered tags from checkout table
		if (strcmp(itemID, "0") != 0) {
			get_item(db, itemID, read_item);

			//Add item to checkout store
			addCheckoutItem(checkout_store, 1, read_item->description, read_item->price);

			//Keep checkout summary information
			item_count++;
			subtotal += read_item->price;
		}
	}

	//Update checkout summary information
	updateCheckoutItemCount(item_count);
	double tax = subtotal * 0.13;
	total = subtotal + tax;
	updatePriceDetails(subtotal, tax, total);
}

//Define function to be called when reading RFID tags in admin page
void readRFIDTagsInAdmin() {
	printf("listening for tags...\n");
	ret = TMR_read(rp, 500, NULL);

	while (TMR_SUCCESS == TMR_hasMoreTags(rp))
	{
		TMR_TagReadData trd;
		char idStr[128];
		char itemID[20];

		ret = TMR_getNextTag(rp, &trd);
		checkerr(rp, ret, 1, "fetching tag");

		TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

		get_tag(db, idStr, itemID, sizeof(itemID));
		get_item(db, itemID, read_item);

		//Add item to checkout store
		addAdminTableItem(admin_store, itemID, read_item->description, read_item->price);
	}
}

//Deconstructor callback function to clear memory when UI is closed
void clearMemoryOnClose(GtkWidget *widget, gpointer data) {
	if (db) {
		printf("closing database...\n");
		sqlite3_close(db);
	}

	if (rp) {
		TMR_destroy(rp);
		printf("freeing rp...\n");
	}

	//Free memory of read item
	if (read_item->description) {
		free(read_item->description);
		printf("freeing item struct description...\n");
	}

	if (read_item) {
		printf("freeing item struct...\n");
		free(read_item);
	}

	if (login_info)	{
		printf("freeing login struct...\n");
		g_free(login_info);
	}

	printf("quitting main gtk thread...\n");
	gtk_main_quit();
}

//Button callback function to switch to checkout page
void openCheckoutPage(GtkWidget *widget, gpointer stack) {
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "checkout");

	//Read RFID tags and update checkout store
	readRFIDTagsInCheckout();
}

//Button callback function to switch to admin login page
void openAdminLoginPage(GtkWidget *widget, gpointer stack) {
	//Clear checkout list (in case opened from checkout page)
	clearCheckout(checkout_store);

	gtk_stack_set_visible_child_name(GTK_STACK(stack), "admin_login");
}

//Button callback function to switch to startup page
void openStartupPage(GtkWidget *widget, gpointer stack) {
	//Clear tables
	clearCheckout(checkout_store);
	clearAdminTable(admin_store);

	//Open startup page
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "startup");
}

//Button callback function to re-read the RFID tags while on the checkout page ("refresh")
void refreshCheckout(GtkWidget *widget, gpointer data) {
	//Clear checkout list
	clearCheckout(checkout_store);

	//Read RFID tags and update checkout store
	readRFIDTagsInCheckout();
}

//Button callback function to re-read the RFID tags while on the admin page ("refresh")
void refreshAdminTable(GtkWidget *widget, gpointer data) {
	//Clear checkout list
	clearAdminTable(admin_store);

	//Read RFID tags and update admin table
	readRFIDTagsInAdmin();
}

//Button callback function to checkout and pay
void checkout(GtkWidget *widget, gpointer data) {
	//Create dialog to confirm purchase
	GtkWidget *confirm_purchase_dialog = gtk_dialog_new_with_buttons(
														"confirm purchase",
														NULL,
														GTK_DIALOG_MODAL,
														"Cancel", GTK_RESPONSE_CANCEL,
														"Confirm", GTK_RESPONSE_ACCEPT,
														NULL
													);
	
	//Create dialog content box
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(confirm_purchase_dialog));

	//Create label with formatted text
	char content_str[100];
	snprintf(content_str, sizeof(content_str), "Total Items: %d\nTotal: $%.2f", item_count, total);
	GtkWidget *content_label = gtk_label_new(content_str);

	gtk_box_pack_start(GTK_BOX(content_area), content_label, FALSE, FALSE, 10);
	gtk_widget_show_all(confirm_purchase_dialog);
		
	//Run dialog and fetch user response
	gint response = gtk_dialog_run(GTK_DIALOG(confirm_purchase_dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		system("./tx 11981");
		openStartupPage(NULL, stack);
	}

	gtk_widget_destroy(confirm_purchase_dialog);
}

//Button callback function to attempt admin login from admin login page
void attemptAdminLogin(GtkButton *button, gpointer data) {
	LoginInfo *login_info = (LoginInfo*) data;
	const char *username = gtk_entry_get_text(login_info->username);
	const char *password = gtk_entry_get_text(login_info->password);

	if (g_strcmp0(username, ADMIN_USER) == 0 && g_strcmp0(password, ADMIN_PASS) == 0) {
		// Login successful, switch to the admin page
		gtk_stack_set_visible_child_name(GTK_STACK(stack), "admin");

		//Read RFID tags and update admin table
		readRFIDTagsInAdmin();
	} else {
		// Display an error message (you can improve this later)
		GtkWidget *dialog = gtk_message_dialog_new(NULL,
												   GTK_DIALOG_MODAL,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_OK,
												   "Invalid username or password");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

//Button callback function to assign item IDs to tags from admin page
void assignTags(GtkButton *button, gpointer data) {
	//Fetch item ID string from input data
	const char *item_id_assign = gtk_entry_get_text((GtkEntry*)data);

	printf("Received item id: %s\n", item_id_assign);
}

/* ********************************************
 * Main function
 * ********************************************/
int main(int argc, char *argv[]) {
	// Initialize system variables
	admin_request = 0;
	rp = &r;
	int system_status;

	// Initialize database variables
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
	if (sql_status != SQLITE_OK) {

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
	gtk_window_set_default_size(GTK_WINDOW(window), 970, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(clearMemoryOnClose), NULL);

    load_css();
	//Create a vertical box layout
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	//Create a GtkStackSwitcher
	GtkWidget *stack_switcher = gtk_stack_switcher_new();
	gtk_box_pack_start(GTK_BOX(vbox), stack_switcher, FALSE, FALSE, 0);

	//Create a GtkStack
	stack = gtk_stack_new();
	gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
	gtk_stack_set_transition_duration(GTK_STACK(stack), 1000);
	gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0);

	//Create instance of item struct to store item information read from database
	read_item = malloc(sizeof(Item));
	read_item->description = NULL;

	//Create login info struct to store potential admin login information
	login_info = g_malloc(sizeof(LoginInfo));

	//Define initial checkout item count and total
	item_count = 0;
	total = 0;

	/* **************
	* startup page
	***************** */
	GtkWidget *startup_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkStyleContext *startup_context = gtk_widget_get_style_context(startup_page);
    gtk_style_context_add_class(startup_context, "default-background");

	//Create top bar with back andadmin login buttons
	GtkWidget *startup_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	GtkWidget *startup_top_bar_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(startup_top_bar_spacer, TRUE);
	gtk_box_pack_start(GTK_BOX(startup_top_bar), startup_top_bar_spacer, TRUE, TRUE, 0);

	GtkWidget *button_admin_login = gtk_button_new_with_label("Admin Login");
	g_signal_connect(button_admin_login, "clicked", G_CALLBACK(openAdminLoginPage), stack);
	gtk_widget_set_halign(button_admin_login, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(startup_top_bar), button_admin_login, FALSE, FALSE, 5);

	//Create start button
	GtkWidget *button_start = gtk_button_new_with_label("Start");
    GtkStyleContext *start_btn_context = gtk_widget_get_style_context(button_start);
    gtk_style_context_add_class(start_btn_context, "start-button");

	//Create label
	GtkWidget *label1 = gtk_label_new("TagSense");
    GtkStyleContext *tagsense_context = gtk_widget_get_style_context(label1);
    gtk_style_context_add_class(tagsense_context, "tagsense-label");
	g_signal_connect(button_start, "clicked", G_CALLBACK(openCheckoutPage), stack);

	//Pack startup page
	gtk_box_pack_start(GTK_BOX(startup_page), startup_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(startup_page), label1, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(startup_page), button_start, FALSE, FALSE, 0);
	gtk_stack_add_named(GTK_STACK(stack), startup_page, "startup");

	/* **************
	* checkout page
	***************** */
	GtkWidget *checkout_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	//Create top bar with back, refresh, and admin login buttons
	GtkWidget *checkout_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	GtkWidget *button_back = gtk_button_new_with_label("Back");
	g_signal_connect(button_back, "clicked", G_CALLBACK(openStartupPage), stack);
	gtk_widget_set_halign(button_back, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(checkout_top_bar), button_back, FALSE, FALSE, 5);

	GtkWidget *button_refresh = gtk_button_new_with_label("Refresh");
	g_signal_connect(button_refresh, "clicked", G_CALLBACK(refreshCheckout), NULL);
	gtk_widget_set_halign(button_refresh, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(checkout_top_bar), button_refresh, FALSE, FALSE, 5);

	GtkWidget *top_bar_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(top_bar_spacer, TRUE);  //make spacer expand to take up middle space of top bar
	gtk_box_pack_start(GTK_BOX(checkout_top_bar), top_bar_spacer, TRUE, TRUE, 0);

	GtkWidget *button_checkout_admin_login = gtk_button_new_with_label("Admin Login");
	g_signal_connect(button_checkout_admin_login, "clicked", G_CALLBACK(openAdminLoginPage), stack);
	gtk_widget_set_halign(button_admin_login, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(checkout_top_bar), button_checkout_admin_login, FALSE, FALSE, 5);

	//Create checkout list store
	checkout_store = gtk_list_store_new(3, G_TYPE_INT,  G_TYPE_STRING, G_TYPE_INT);

	//Create checkout tree view
	GtkWidget *checkout_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(checkout_store));
    GtkStyleContext *tree_context = gtk_widget_get_style_context(checkout_tree_view);
    gtk_style_context_add_class(tree_context, "tree-view");


	//Add columns
	GtkCellRenderer *checkout_renderer;
	GtkTreeViewColumn *checkout_col;

	//Define quantity column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Quantity", checkout_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_fixed_width(checkout_col,200);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);

	//Define name column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Name", checkout_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_fixed_width(checkout_col,600);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);

	//Define price column
	checkout_renderer = gtk_cell_renderer_text_new();
	checkout_col = gtk_tree_view_column_new_with_attributes("Price ($)", checkout_renderer, "text", 2, NULL);
	gtk_tree_view_column_set_fixed_width(checkout_col,200);
	gtk_tree_view_append_column(GTK_TREE_VIEW(checkout_tree_view), checkout_col);


		//Left side: checkout summary
	GtkWidget *checkout_summary_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkStyleContext *checkout_context = gtk_widget_get_style_context(checkout_summary_box);
    gtk_style_context_add_class(checkout_context, "checkout-box");
    gtk_widget_set_halign(checkout_summary_box, GTK_ALIGN_START);
    gtk_widget_set_valign(checkout_summary_box, GTK_ALIGN_CENTER);

    checkout_item_count_label = gtk_label_new("Item Count: 0");
    GtkStyleContext *item_count_context = gtk_widget_get_style_context(checkout_item_count_label);
    gtk_style_context_add_class(item_count_context, "summary-label");
    //gtk_label_set_markup(GTK_LABEL(checkout_item_count_label), "Item Count: 0");

    price_details_label = gtk_label_new("Subtotal: $0.00\nTax: $0.00\nTotal: $0.00");
    GtkStyleContext *price_details_context = gtk_widget_get_style_context(price_details_label);
    gtk_style_context_add_class(price_details_context, "summary-label");
    gtk_box_pack_start(GTK_BOX(checkout_summary_box), checkout_item_count_label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(checkout_summary_box), price_details_label, TRUE, TRUE, 0);


		//Right side: pay button
	GtkWidget *button_pay = gtk_button_new_with_label("Pay Now");
    GtkStyleContext *pay_btn_context = gtk_widget_get_style_context(button_pay);
    gtk_style_context_add_class(pay_btn_context, "pay-button");

	g_signal_connect(button_pay, "clicked", G_CALLBACK(checkout), NULL);

	GtkWidget *checkout_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkStyleContext *container_context = gtk_widget_get_style_context(checkout_box);
    gtk_style_context_add_class(container_context, "container-box");

	//Pack checkout page
	GtkWidget *checkout_scroll_window = gtk_scrolled_window_new(NULL, NULL);
    GtkStyleContext *scroll_context = gtk_widget_get_style_context(checkout_scroll_window);
    gtk_style_context_add_class(scroll_context, "scroll-summary");

	gtk_container_add(GTK_CONTAINER(checkout_scroll_window), checkout_tree_view);
	gtk_box_pack_start(GTK_BOX(checkout_box), checkout_scroll_window, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(checkout_box), checkout_summary_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(checkout_page), checkout_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(checkout_page), checkout_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(checkout_page), button_pay, FALSE, FALSE, 0);

	gtk_stack_add_named(GTK_STACK(stack), checkout_page, "checkout");

	/* **************
	* admin login page
	***************** */
	GtkWidget *admin_login_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	//Create top bar with back button
	GtkWidget *admin_login_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	GtkWidget *button_admin_login_back = gtk_button_new_with_label("Back");
	g_signal_connect(button_admin_login_back, "clicked", G_CALLBACK(openStartupPage), stack);
	gtk_widget_set_halign(button_admin_login_back, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(admin_login_top_bar), button_admin_login_back, FALSE, FALSE, 5);

	//Create username and password prompts
	GtkWidget *username_label = gtk_label_new("Username:");
	GtkWidget *username_entry = gtk_entry_new();

	GtkWidget *password_label = gtk_label_new("Password:");
	GtkWidget *password_entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE); //hides password text

	GtkWidget *button_submit_login = gtk_button_new_with_label("Submit");

	//Prepare login information
	login_info->username = GTK_ENTRY(username_entry);
	login_info->password = GTK_ENTRY(password_entry);

	//Connect the login button to the callback function
	g_signal_connect(button_submit_login, "clicked", G_CALLBACK(attemptAdminLogin), login_info);

	//Pack admin login page
	gtk_box_pack_start(GTK_BOX(admin_login_page), admin_login_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(admin_login_page), username_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_login_page), username_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_login_page), password_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_login_page), password_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_login_page), button_submit_login, FALSE, FALSE, 0);
	gtk_stack_add_named(GTK_STACK(stack), admin_login_page, "admin_login");

	/* **************
	* admin page
	***************** */
	GtkWidget *admin_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	//Create top bar with back and refresh buttons
	GtkWidget *admin_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	GtkWidget *button_admin_back = gtk_button_new_with_label("Back");
	g_signal_connect(button_admin_back, "clicked", G_CALLBACK(openStartupPage), stack);
	gtk_widget_set_halign(button_admin_back, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(admin_top_bar), button_admin_back, FALSE, FALSE, 5);

	GtkWidget *button_admin_refresh = gtk_button_new_with_label("Refresh");
	g_signal_connect(button_admin_refresh, "clicked", G_CALLBACK(refreshAdminTable), NULL);
	gtk_widget_set_halign(button_admin_refresh, GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(admin_top_bar), button_admin_refresh, FALSE, FALSE, 5);

	//Create admin list store (columns: item id, item description, item price)
	admin_store = gtk_list_store_new(3, G_TYPE_STRING,  G_TYPE_STRING, G_TYPE_INT);

	//Create admin tree view
	GtkWidget *admin_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(admin_store));

	//Add columns
	GtkCellRenderer *admin_renderer;
	GtkTreeViewColumn *admin_col;

	//Define item id column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Item ID", admin_renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	//Define name column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Name", admin_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	//Define price column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Price ($)", admin_renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	//Create scroll window for table
	GtkWidget *admin_scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(admin_scroll_window), admin_tree_view);

	//Create label entry, and button for assigning tags to a desired item id
	GtkWidget *item_id_prompt_label = gtk_label_new("Assign item id:");
	GtkWidget *item_id_prompt_entry = gtk_entry_new();
	GtkWidget *button_assign_tags = gtk_button_new_with_label("Assign");

	//Prepare read item id prompt
	GtkEntry *item_id_assign = GTK_ENTRY(item_id_prompt_entry);

	//Connect the assign button to the callback function
	g_signal_connect(button_assign_tags, "clicked", G_CALLBACK(assignTags), item_id_assign);

	//Pack admin page
	gtk_box_pack_start(GTK_BOX(admin_page), admin_top_bar, FALSE, FALSE, 5);
	gtk_stack_add_named(GTK_STACK(stack), admin_page, "admin");
	gtk_box_pack_start(GTK_BOX(admin_page), admin_scroll_window, TRUE, TRUE, 5);


	//Link stack switcher with stack
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));

	//Show everything
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

