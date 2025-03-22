#include <gtk/gtk.h> 
#include "actuator_signal_send.h"

void append_text(GtkTextView* text_view, const char* new_text) {
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
	GtkTextIter end;

	//Get the last position in the buffer
    gtk_text_buffer_get_end_iter(buffer, &end);
    
    //Insert new text at the end
    gtk_text_buffer_insert(buffer, &end, new_text, -1);

	// Create a mark at the end (or move it if it exists)
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, "end", &end, FALSE);
    gtk_text_buffer_move_mark(buffer, mark, &end);

    // Scroll to the mark
    gtk_text_view_scroll_to_mark(text_view, mark, 0.0, FALSE, 0.0, 1.0);
}

void on_button_clicked(GtkButton* button, gpointer data) {
	//Complete transmission
	send_actuator_code_16bit(11981);

	//Output update to UI text view
	GtkTextView *text_view = GTK_TEXT_VIEW(data);
	append_text(text_view, "Transmission of code 11981 successful!\n");
}

int main(int argc, char *argv[]) {
	GtkBuilder* builder;
	GtkWidget* window;
	GtkWidget* button;
	GtkWidget* text_view;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	//Construct a GtkBuilder instance and load UI description
	builder = gtk_builder_new();
	if (gtk_builder_add_from_file(builder, "my_builder.ui", &error) == 0) {
		g_printerr("Error loading file: %s\n", error->message);
		g_clear_error(&error);
		return 1;
	}

	//Get objects from the UI file
	window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	button = GTK_WIDGET(gtk_builder_get_object(builder, "button"));
	text_view = GTK_WIDGET(gtk_builder_get_object(builder, "text_view"));

	//Check for errors in getting objects from UI file
	if (!window | !button | !text_view) {
		g_printerr("Error loading UI components\n");
	}
	
	//Connect button signal to callback function
	g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), text_view);

	//Connect window close event
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	g_object_unref(builder);
	return 0;
}

