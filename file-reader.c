/* gtk/gtk für GUI, stdio für Dateioperationen, string für String-Manipulationen */
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <magic.h>
#include <time.h>
#include <sys/stat.h>

GString *log_buffer;

/* Globale Variablen für GUI Elemente */
GtkWidget *window;
GtkWidget *file_chooser;
GtkWidget *search_entry;
GtkWidget *search_button;
GtkWidget *result_label;
GtkWidget *vbox;

gboolean check_file(const char *filename);
gboolean valid_text_file_selected = FALSE;

void init_log() {
    log_buffer = g_string_new("");
    time_t now;
    struct tm *local;
    char timestamp[20];

    time(&now);
    local = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", local);

    g_string_append_printf(log_buffer, "[%s] Application started.\n", timestamp);
}

void add_log_entry(const char *action) {
    time_t now;
    struct tm *local;
    char timestamp[20];

    time(&now);
    local = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", local);

    g_string_append_printf(log_buffer, "[%s] %s\n", timestamp, action);
}

void create_log_file() {
    struct stat st = {0};
    if (stat("Logs", &st) == -1) {
        mkdir("Logs", 0700);
    }

    time_t now;
    struct tm *local;
    char filename[100];

    time(&now);
    local = localtime(&now);
    strftime(filename, sizeof(filename), "Logs/%Y-%m-%d--%H-%M-%S.log", local);

    FILE *log_file = fopen(filename, "w");
    if (log_file) {
        fprintf(log_file, "%s", log_buffer->str);
        fclose(log_file);
        gtk_label_set_text(GTK_LABEL(result_label), "Log file created successfully.");
    } else {
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not create log file.");
    }
}


void help_usage(GtkWidget *widget, gpointer data) {

    GString *result = g_string_new("");
    g_string_append_printf(result, "Software to open text files and search for Strings in the textfiles. \n");
    g_string_append_printf(result, "File -> Open (or clicking the top button) opens the File Selector.  \n");
    g_string_append_printf(result, "File -> Search (or clicking the search button) searches for the string in the text field. \n");
    g_string_append_printf(result, "File -> Quit closes the software. \n");
    g_string_append_printf(result, "Log -> Create Log creates a logfile in the subfolder Logs (Logs are also created on exit). \n");
    g_string_append_printf(result, "Log -> Open Log displays the contents of the selected Log file.");
    g_string_append_printf(result, "Help -> Usage displays this help menu.");

    gtk_label_set_text(GTK_LABEL(result_label), result->str);
    g_string_free(result, TRUE); 
}

/* Funktion die bei Verwenden des Search-Buttons aufgerufen wird */
void search_file(GtkWidget *widget, gpointer data) {
    if (!valid_text_file_selected) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please select a valid text file before searching.");
        return;
    }

    const gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));

    add_log_entry(g_strdup_printf("Searched in file for String \"%s\"", search_text));

    if (filename == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please select a file.");
        return;
    }

    if (strlen(search_text) == 0) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please enter search text.");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not open file.");
        return;
    }

    char line[1024];
    int line_number = 0;
    int found_count = 0;
    GString *line_numbers = g_string_new("");
    int numbers_in_current_line = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (strstr(line, search_text) != NULL) {
            found_count++;
            if (found_count > 1) {
                if (numbers_in_current_line == 20) {
                    g_string_append(line_numbers, "\n                : ");
                    numbers_in_current_line = 0;
                } else {
                    g_string_append(line_numbers, ", ");
                }
            }
            g_string_append_printf(line_numbers, "%d", line_number);
            numbers_in_current_line++;
        }
    }

    fclose(file);

    GString *result = g_string_new("");
    if (found_count > 0) {
        g_string_append_printf(result, "Found %d times\n", found_count);
        g_string_append_printf(result, "Found in lines: %s", line_numbers->str);
        gtk_label_set_text(GTK_LABEL(result_label), result->str);
        add_log_entry(g_strdup_printf("Found %d times", found_count));
        add_log_entry(g_strdup_printf("Found in lines: %s", line_numbers->str));
    } else {
        gtk_label_set_text(GTK_LABEL(result_label), "Text not found in the file.");
    }

    g_string_free(result, TRUE);
    g_string_free(line_numbers, TRUE);
}



void open_file(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Open File",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (check_file(filename)) {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser), filename);
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

gboolean check_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not open file.");
    add_log_entry(g_strdup_printf("Couldn't open file. Path: %s", filename));
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    // Check if it's a text file using libmagic
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (magic == NULL) {
        fclose(file);
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not initialize magic library.");
        add_log_entry("Couldn't initialize magic library.");
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    if (magic_load(magic, NULL) != 0) {
        magic_close(magic);
        fclose(file);
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not load magic database.");
        add_log_entry("Couldn't load magic database.");
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    const char *mime_type = magic_file(magic, filename);
    if (mime_type == NULL || strncmp(mime_type, "text/", 5) != 0) {
        magic_close(magic);
        fclose(file);
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Not a text file.");
    add_log_entry(g_strdup_printf("File is not a text file. Path: %s", filename));
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    magic_close(magic);
    fclose(file);
    gtk_label_set_text(GTK_LABEL(result_label), "File selected successfully.");
    add_log_entry(g_strdup_printf("File selected. Path: %s", filename));
    valid_text_file_selected = TRUE;
    return TRUE;
}

void on_file_set(GtkFileChooserButton *chooser, gpointer user_data) {
    const char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    if (filename) {
        check_file(filename);
    }
}

void open_log(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Open Log File",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "Logs");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), gtk_file_filter_new());
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.log");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        FILE *file = fopen(filename, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            long fsize = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *content = malloc(fsize + 1);
            fread(content, 1, fsize, file);
            fclose(file);

            content[fsize] = 0;

            gtk_label_set_text(GTK_LABEL(result_label), content);
            free(content);
        } else {
            gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not open log file.");
        }
        
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

GtkWidget* create_menu_bar() {
    GtkWidget *menu_bar;
    GtkWidget *file_menu;
    GtkWidget *log_menu;
    GtkWidget *help_menu;
    GtkWidget *file_mi;
    GtkWidget *log_mi;
    GtkWidget *help_mi;
    GtkWidget *open_mi;
    GtkWidget *search_mi;
    GtkWidget *quit_mi;
    GtkWidget *create_log_mi;
    GtkWidget *open_log_mi;
    GtkWidget *help_usage_mi;

    menu_bar = gtk_menu_bar_new();

    file_menu = gtk_menu_new();
    log_menu = gtk_menu_new();
    help_menu = gtk_menu_new();

    file_mi = gtk_menu_item_new_with_label("File");
    log_mi = gtk_menu_item_new_with_label("Log");
    help_mi = gtk_menu_item_new_with_label("Help");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_mi), file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(log_mi), log_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_mi), help_menu);

    open_mi = gtk_menu_item_new_with_label("Open");
    search_mi = gtk_menu_item_new_with_label("Search");
    quit_mi = gtk_menu_item_new_with_label("Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), search_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_mi);

    create_log_mi = gtk_menu_item_new_with_label("Create Log");
    open_log_mi = gtk_menu_item_new_with_label("Open Log");

    gtk_menu_shell_append(GTK_MENU_SHELL(log_menu), create_log_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(log_menu), open_log_mi);

    help_usage_mi = gtk_menu_item_new_with_label("Usage");

    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_usage_mi);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), log_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_mi);

    g_signal_connect(G_OBJECT(open_mi), "activate", G_CALLBACK(open_file), NULL);
    g_signal_connect(G_OBJECT(search_mi), "activate", G_CALLBACK(search_file), NULL);
    g_signal_connect(G_OBJECT(quit_mi), "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(create_log_mi), "activate", G_CALLBACK(create_log_file), NULL);
    g_signal_connect(G_OBJECT(open_log_mi), "activate", G_CALLBACK(open_log), NULL);
    g_signal_connect(G_OBJECT(help_usage_mi), "activate", G_CALLBACK(help_usage), NULL);

    return menu_bar;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    init_log();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Search");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 400, 300);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    file_chooser = gtk_file_chooser_button_new("Select a File", GTK_FILE_CHOOSER_ACTION_OPEN);
    g_signal_connect(G_OBJECT(file_chooser), "file-set", G_CALLBACK(on_file_set), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), file_chooser, FALSE, FALSE, 0);

    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Enter search text");
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);

    search_button = gtk_button_new_with_label("Search in File");
    g_signal_connect(search_button, "clicked", G_CALLBACK(search_file), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), search_button, FALSE, FALSE, 0);

    result_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), result_label, FALSE, FALSE, 0);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(create_log_file), NULL);  // Create log file on exit
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_string_free), log_buffer);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
