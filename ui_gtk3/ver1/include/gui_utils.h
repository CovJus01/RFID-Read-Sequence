#include <gtk/gtk.h>

//Definitions for GUI
#define ADMIN_USER "admin"
#define ADMIN_PASS "1234"
#define IDSTRS_SIZE 100

//Struct to gather admin login information for button callback function
typedef struct {
    GtkEntry *username;
    GtkEntry *password;
} LoginInfo;

//Function prototypes
void load_css(void);

void openStartupPage(GtkWidget *widget, gpointer stack);

void openAdminLoginPage(GtkWidget *widget, gpointer stack);

void attemptAdminLogin(GtkButton *button, gpointer data);

void openCheckoutPage(GtkWidget *widget, gpointer stack);

void addCheckoutItem(GtkListStore *checkout_store, int quantity, const gchar *name, int price);

void clearCheckout(GtkListStore *store);

void updateCheckoutItemCount(int count);

void updatePriceDetails(double subtotal, double tax, double total);

void refreshCheckout(GtkWidget *widget, gpointer data);

void checkout(GtkWidget *widget, gpointer data);

void readRFIDTagsInCheckout();

void readRFIDTagsInAdmin();

void addAdminTableItem(GtkListStore *store, const gchar *item_id, const gchar *name, int price);

void refreshAdminTable(GtkWidget *widget, gpointer data);

void clearAdminTable(GtkListStore *store);

void addTags(GtkButton *button, gpointer data);

void updateTags(GtkButton *button, gpointer data);

void clearMemoryOnClose(GtkWidget *widget, gpointer data);
