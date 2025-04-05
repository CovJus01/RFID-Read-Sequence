/* ********************************************
 * Includes
 * ********************************************/
//API Includes
#include "serial_reader_imp.h"
#include "tm_reader.h"
#include "tmr_utils.h"

// Necesarry Includes
#include "RFID_Utils.h"
#include "DB_utils.h"
#include "checkout_utils.h"
#include "gui_utils.h"
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
//Global GUI variables
extern GtkWidget *stack; //gtk stack for multiple page GUI
extern GtkListStore *checkout_store; //gtk store for checkout page's item table
extern GtkListStore *admin_store; //gtk store for admin page's item table
extern Item *read_item; //struct pointer for item information pulled from database
extern GtkWidget *checkout_item_count_label; //label for item count in checkout page
extern GtkWidget *price_details_label; //label for price information in checkout page
extern int item_count; //count for checkout information
extern double total; //total final price for checkout information
extern LoginInfo *login_info; //struct pointer for admin login information
extern char* idStrs[IDSTRS_SIZE]; //string array to hold RFIDs read

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
extern sqlite3 *db;

/* ********************************************
 * Main function
 * ********************************************/
int main(int argc, char *argv[]) {
	// Initialize system variable
	rp = &r;

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

	//Create top bar with back and admin login buttons
	GtkWidget *startup_top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	GtkWidget *startup_top_bar_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(startup_top_bar_spacer, TRUE);
	gtk_box_pack_start(GTK_BOX(startup_top_bar), startup_top_bar_spacer, TRUE, TRUE, 0);

	GtkWidget *button_admin_login = gtk_button_new_with_label("");
	g_signal_connect(button_admin_login, "clicked", G_CALLBACK(openAdminLoginPage), stack);
	gtk_widget_set_halign(button_admin_login, GTK_ALIGN_END);

    GtkStyleContext *admin_login_btn_context = gtk_widget_get_style_context(button_admin_login);
    gtk_style_context_add_class(admin_login_btn_context, "admin-button");

	gtk_box_pack_start(GTK_BOX(startup_top_bar), button_admin_login, FALSE, FALSE, 10);

	//Create start button
	GtkWidget *button_start = gtk_button_new_with_label("Start");
    GtkStyleContext *start_btn_context = gtk_widget_get_style_context(button_start);
    gtk_style_context_add_class(start_btn_context, "start-button");
	g_signal_connect(button_start, "clicked", G_CALLBACK(openCheckoutPage), stack);

	//Create image with nice tagsense title font
	GtkWidget *tagsense_title_image = gtk_image_new_from_file("./images/title.png");

	//Pack startup page
	gtk_box_pack_start(GTK_BOX(startup_page), startup_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(startup_page), tagsense_title_image, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(startup_page), button_start, FALSE, FALSE, 0);
	gtk_stack_add_named(GTK_STACK(stack), startup_page, "startup");

	/* **************
	* checkout page
	***************** */
	GtkWidget *checkout_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkStyleContext *checkout_page_context = gtk_widget_get_style_context(checkout_page);
    gtk_style_context_add_class(checkout_page_context, "default-background");

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

	GtkWidget *button_checkout_admin_login = gtk_button_new_with_label("");
	g_signal_connect(button_checkout_admin_login, "clicked", G_CALLBACK(openAdminLoginPage), stack);
	gtk_widget_set_halign(button_admin_login, GTK_ALIGN_END);

    GtkStyleContext *checkout_admin_login_btn_context = gtk_widget_get_style_context(button_checkout_admin_login);
    gtk_style_context_add_class(checkout_admin_login_btn_context, "admin-button");

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

    price_details_label = gtk_label_new("Subtotal:\t\t\t$0.00\nTax:\t\t\t\t$0.00\nTotal:\t\t\t\t$0.00");
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
    GtkStyleContext *username_context = gtk_widget_get_style_context(username_label);
    gtk_style_context_add_class(username_context, "admin-label");
	GtkWidget *username_entry = gtk_entry_new();
    username_context = gtk_widget_get_style_context(username_entry);
    gtk_style_context_add_class(username_context, "admin-field");

	GtkWidget *password_label = gtk_label_new("Password:");
    GtkStyleContext *password_context = gtk_widget_get_style_context(password_label);
    gtk_style_context_add_class(password_context, "admin-label");
	GtkWidget *password_entry = gtk_entry_new();
    password_context = gtk_widget_get_style_context(password_entry);
    gtk_style_context_add_class(password_context, "admin-field");
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE); //hides password text

	GtkWidget *button_submit_login = gtk_button_new_with_label("Submit");
    GtkStyleContext *submit_context = gtk_widget_get_style_context(button_submit_login);
    gtk_style_context_add_class(submit_context, "submit-button");

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
    tree_context = gtk_widget_get_style_context(admin_tree_view);
    gtk_style_context_add_class(tree_context, "tree-view");

	//Add columns
	GtkCellRenderer *admin_renderer;
	GtkTreeViewColumn *admin_col;

	//Define item id column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Item ID", admin_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_fixed_width(admin_col,200);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	//Define name column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Name", admin_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_fixed_width(admin_col,800);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	//Define price column
	admin_renderer = gtk_cell_renderer_text_new();
	admin_col = gtk_tree_view_column_new_with_attributes("Price ($)", admin_renderer, "text", 2, NULL);
	gtk_tree_view_column_set_fixed_width(admin_col,200);
	gtk_tree_view_append_column(GTK_TREE_VIEW(admin_tree_view), admin_col);

	GtkWidget *admin_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    container_context = gtk_widget_get_style_context(admin_box);
    gtk_style_context_add_class(container_context, "container-box");

	//Create scroll window for table
	GtkWidget *admin_scroll_window = gtk_scrolled_window_new(NULL, NULL);
    scroll_context = gtk_widget_get_style_context(admin_scroll_window);
    gtk_style_context_add_class(scroll_context, "scroll-summary");
	gtk_container_add(GTK_CONTAINER(admin_scroll_window), admin_tree_view);

	//Create update tags button
	GtkWidget *button_update = gtk_button_new_with_label("Update Tags");
    GtkStyleContext *update_btn_context = gtk_widget_get_style_context(button_update);
    gtk_style_context_add_class(update_btn_context, "update-button");
	g_signal_connect(button_update, "clicked", G_CALLBACK(updateTags), NULL);

	//Create add tags button
	GtkWidget *button_add = gtk_button_new_with_label("Add Tags");
    GtkStyleContext *add_btn_context = gtk_widget_get_style_context(button_add);
    gtk_style_context_add_class(add_btn_context, "add-button");
	g_signal_connect(button_add, "clicked", G_CALLBACK(addTags), NULL);

	//Pack update tags and add tags buttons into a box
	GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
	gtk_box_pack_start(GTK_BOX(button_box), button_update, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(button_box), button_add, TRUE, TRUE, 0);
	
	//Pack admin page
	gtk_box_pack_start(GTK_BOX(admin_page), admin_top_bar, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(admin_box), admin_scroll_window, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_page), admin_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(admin_page), button_box, FALSE, FALSE, 0);
	gtk_stack_add_named(GTK_STACK(stack), admin_page, "admin");


	//Link stack switcher with stack
	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stack_switcher), GTK_STACK(stack));

	//Show everything
	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

