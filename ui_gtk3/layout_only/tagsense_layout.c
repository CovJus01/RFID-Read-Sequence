/* ********************************************
 * Includes
 * ********************************************/
//For GTK3 (apt package libgtk-3-dev)
#include <gtk/gtk.h>
#include <pthread.h>
#include <unistd.h>

/* ********************************************
 * Global variables
 * ********************************************/
//Global GUI variables to work with threading
GtkWidget *stack;
GtkListStore *checkout_store;

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

void *testWorker(void *param) {
	sleep(3);

	//Schedule adding an item to the checkout list
	g_idle_add(addCheckoutItemValues, NULL);

	return NULL;
}

//Button callback function to switch to checkout page
void openCheckoutPage(GtkWidget *widget, gpointer stack) {
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "checkout");

	//Start test thread when the checkout page opens
	pthread_t test_thread;
	pthread_create(&test_thread, NULL, testWorker, NULL);
}

//Button callback function to switch to startup page
void openStartupPage(GtkWidget *widget, gpointer stack) {
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "startup");
}

/* ********************************************
 * Main function
 * ********************************************/

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);

	//Create main window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "TagSense");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

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

