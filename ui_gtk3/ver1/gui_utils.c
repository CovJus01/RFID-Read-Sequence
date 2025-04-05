#include "gui_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//Function to load CSS styling
void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkScreen *screen = gdk_screen_get_default();
    gtk_css_provider_load_from_path(provider, "styles.css", NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);  // Free the provider when done
}

//Button callback function to switch to startup page
void openStartupPage(GtkWidget *widget, gpointer stack) {
	//Clear tables
	clearCheckout(checkout_store);
	clearAdminTable(admin_store);

	//Open startup page
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "startup");
}

//Button callback function to switch to admin login page
void openAdminLoginPage(GtkWidget *widget, gpointer stack) {
	//Clear checkout list (in case opened from checkout page)
	clearCheckout(checkout_store);

	gtk_stack_set_visible_child_name(GTK_STACK(stack), "admin_login");
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
		// Display error message
		GtkWidget *dialog = gtk_message_dialog_new(NULL,
												   GTK_DIALOG_MODAL,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_OK,
												   "Invalid username or password");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

//Button callback function to switch to checkout page
void openCheckoutPage(GtkWidget *widget, gpointer stack) {
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "checkout");

	//Read RFID tags and update checkout store
	readRFIDTagsInCheckout();
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

//Function to clear checkout table
void clearCheckout(GtkListStore *store) {
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
	snprintf(price_details_str, sizeof(price_details_str),"Subtotal:\t\t\t$%.2f\nTax:\t\t\t\t$%.2f\nTotal:\t\t\t\t$%.2f", subtotal, tax, total);

	//Update the checkout price details label
	gtk_label_set_text(GTK_LABEL(price_details_label), price_details_str);
}

//Button callback function to re-read the RFID tags while on the checkout page ("refresh")
void refreshCheckout(GtkWidget *widget, gpointer data) {
	//Clear checkout list
	clearCheckout(checkout_store);

	//Read RFID tags and update checkout store
	readRFIDTagsInCheckout();
}

//Button callback function to checkout and pay
void checkout(GtkWidget *widget, gpointer data) {
	//Create dialog to confirm purchase
	GtkWidget *confirm_purchase_dialog = gtk_dialog_new_with_buttons(
														"Confirm Purchase",
														NULL,
														GTK_DIALOG_MODAL,
														"Cancel", GTK_RESPONSE_CANCEL,
														"Confirm", GTK_RESPONSE_ACCEPT,
														NULL
													);

    GtkStyleContext *confirm_context = gtk_widget_get_style_context(confirm_purchase_dialog);
    gtk_style_context_add_class(confirm_context, "confirmation-box");
	
	//Create dialog content box
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(confirm_purchase_dialog));

	gtk_widget_set_size_request(content_area, 300,300);

	//Create label with formatted text
	char content_str[100];
	snprintf(content_str, sizeof(content_str), "Items:\t %d\nTotal:\t $%.2f", item_count, total);
	GtkWidget *content_label = gtk_label_new(content_str);
	gtk_box_pack_start(GTK_BOX(content_area), content_label, FALSE, FALSE, 10);

	//Create payment options images
	GtkWidget *payment_options_image = gtk_image_new_from_file("./images/payment_options.png");
	gtk_container_add(GTK_CONTAINER(content_area), payment_options_image);

	gtk_widget_show_all(confirm_purchase_dialog);
		
	//Run dialog and fetch user response
	gint response = gtk_dialog_run(GTK_DIALOG(confirm_purchase_dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		system("./tx 11981");
		openStartupPage(NULL, stack);
	}

	gtk_widget_destroy(confirm_purchase_dialog);
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

	int idStr_index = 0; //For writing idStrs to the idStrs array
	while (TMR_SUCCESS == TMR_hasMoreTags(rp))
	{
		TMR_TagReadData trd;
		char *idStr = (char*)malloc(128 * sizeof(char));
		char itemID[20];

		ret = TMR_getNextTag(rp, &trd);
		checkerr(rp, ret, 1, "fetching tag");

		TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, idStr);

		idStrs[idStr_index++] = idStr;

		if (check_assigned(db, idStr)) {
			//If tags exist in the database, display correct information in the table
			get_tag(db, idStr, itemID, sizeof(itemID));
			get_item(db, itemID, read_item);

			//Add item to checkout store
			addAdminTableItem(admin_store, itemID, read_item->description, read_item->price);
		}
		else {
			//Otherwise, show that there is no record of the tag
			addAdminTableItem(admin_store, NULL, "No record of tag", 0);
		}
			
	}
	idStrs[idStr_index+1] = NULL; //Assign null to index at which we should stop reading idStrs
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

//Button callback function to re-read the RFID tags while on the admin page ("refresh")
void refreshAdminTable(GtkWidget *widget, gpointer data) {
	//Clear checkout list
	clearAdminTable(admin_store);

	//Read RFID tags and update admin table
	readRFIDTagsInAdmin();
}

//Function to clear admin table
void clearAdminTable(GtkListStore *store) {
	gtk_list_store_clear(GTK_LIST_STORE(store));
}

void addTags(GtkButton *button, gpointer data) {
	//Create dialog box to accept admin input
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Tags",
		NULL,
		GTK_DIALOG_MODAL,
		"Cancel", GTK_RESPONSE_CANCEL,
		"Confirm", GTK_RESPONSE_ACCEPT,
		NULL);

	GtkStyleContext *context = gtk_widget_get_style_context(dialog);
	gtk_style_context_add_class(context, "confirmation-box");

	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_widget_set_size_request(content_area, 300,300);

	//Calculate how many items will be added to database
	int add_tag_count = 0;
	for (int i = 0; idStrs[i] != NULL; i++) {
		//Only count tags that are not in the database
		if (!check_assigned(db, idStrs[i])) {
			add_tag_count++;
		}
	}

	//Add label to show how many tags will be added to database
	char add_item_str[100];
	snprintf(add_item_str, sizeof(add_item_str),"Add %d new tags to the database?", add_tag_count);
	GtkWidget *content_label = gtk_label_new(add_item_str);

	gtk_box_pack_start(GTK_BOX(content_area), content_label, FALSE, FALSE, 10);

	gtk_widget_show_all(dialog);

	//Get response from dialog, and handle accordingly
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		//Add tags to database
		for (int i = 0; idStrs[i] != NULL; i++) {
			//Only add tags that are not in the database 
			if (!check_assigned(db, idStrs[i])) {
				add_tag(db, idStrs[i]);
			}
		}
	}

	//Free dialog memory after use
    gtk_widget_destroy(dialog);
}

//Button callback function to assign item IDs to tags from admin page
void updateTags(GtkButton *button, gpointer data) {
	//Create dialog box to accept admin input
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Update Tags",
													NULL,
													GTK_DIALOG_MODAL,
													"Cancel", GTK_RESPONSE_CANCEL,
													"Confirm", GTK_RESPONSE_ACCEPT,
													NULL);

	GtkStyleContext *context = gtk_widget_get_style_context(dialog);
	gtk_style_context_add_class(context, "confirmation-box");

	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_widget_set_size_request(content_area, 300,300);
	
	// Create a vertical box to arrange label and entry
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(content_area), box);

	//Create a label as a prompt
    GtkWidget *prompt_label = gtk_label_new("Enter item ID to assign:");
    gtk_box_pack_start(GTK_BOX(box), prompt_label, FALSE, FALSE, 0);

    //Create an entry field
    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    // Show all widgets in the dialog
    gtk_widget_show_all(dialog);

	//Get response from dialog, and handle accordingly
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *input_item_id = gtk_entry_get_text(GTK_ENTRY(entry));

		//Check for invalid input. atoi returns 0 if no conversion from string to int is possible
		if ((atoi(input_item_id) != 0) && (check_itemID_exists(db, input_item_id))) {
			//Assign item id to all tags in idStrs array
			for (int i = 0; idStrs[i] != NULL; i++) {
				//Only update tag if it is already in the database
				if (check_assigned(db, idStrs[i])) {
					//Update the tag to have input item id
					update_tag_item(db, input_item_id, idStrs[i]); 
				}
			}

			//Show that tags were updated
			GtkWidget *update_successful_dialog = gtk_message_dialog_new(NULL,
													   GTK_DIALOG_MODAL,
													   GTK_MESSAGE_INFO,
													   GTK_BUTTONS_OK,
													   "Update request successfully sent to database!\nIt may take a minute or so for these changes to be made.");
			GtkStyleContext *confirmation_context = gtk_widget_get_style_context(update_successful_dialog);
			gtk_style_context_add_class(confirmation_context, "confirmation-box");

			gtk_dialog_run(GTK_DIALOG(update_successful_dialog));
			gtk_widget_destroy(update_successful_dialog);

			clearAdminTable(admin_store);
			readRFIDTagsInAdmin();
		}
		else {
			//Show that tags could not be updated
			GtkWidget *update_failed_dialog = gtk_message_dialog_new(NULL,
													   GTK_DIALOG_MODAL,
													   GTK_MESSAGE_ERROR,
													   GTK_BUTTONS_OK,
													   "Could not update tags because input item ID is not valid.");

			GtkStyleContext *confirmation_context = gtk_widget_get_style_context(update_failed_dialog);
			gtk_style_context_add_class(confirmation_context, "confirmation-box");

			gtk_dialog_run(GTK_DIALOG(update_failed_dialog));
			gtk_widget_destroy(update_failed_dialog);
		}
	}
	
	//Free dialog memory after use
    gtk_widget_destroy(dialog);
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

	//Free memory of idStrs
	printf("freeing all strings in idStrs...\n");
	for (int i=0; i<IDSTRS_SIZE; i++) {
		free(idStrs[i]);
	}

	printf("quitting main gtk thread...\n");
	gtk_main_quit();
}
