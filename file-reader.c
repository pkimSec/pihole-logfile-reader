/**    ____  __  _  _   __   __    ____    __     __    ___  ____  ____   __   ____  ____  ____ 
 *    (  _ \(  )/ )( \ /  \ (  )  (  __)  (  )   /  \  / __)(  _ \(  __) / _\ (    \(  __)(  _ \
 *     ) __/ )( ) __ ((  O )/ (_/\ ) _)   / (_/\(  O )( (_ \ )   / ) _) /    \ ) D ( ) _)  )   /
 *    (__)  (__)\_)(_/ \__/ \____/(____)  \____/ \__/  \___/(__\_)(____)\_/\_/(____/(____)(__\_)
 *
 *  Programm zur Suche von Strings in PiHole-Logdateien.
 *  Das Programm erlaubt die Verwendung von verschiedenen Filtern, wie Datum oder Ereignistyp.
 *
 *  Autor P. Kimmig
*/


// Imports GTK für GUI, stdio für Input/Output, string für String-Manipulationen, Magic für Textdateierkennung, time und sys/stat für Logs
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <magic.h>
#include <time.h>
#include <sys/stat.h>

// ---------- VARIABLES -----------
// Globale Variablen für GUI Elemente 
GString *log_buffer;
GtkWidget *window;
GtkWidget *file_chooser;
GtkWidget *search_entry;
GtkWidget *search_button;
GtkWidget *show_lines_button;
GtkWidget *result_label;
GtkWidget *vbox;
GtkWidget *filter_combo;
GtkWidget *start_date_button;
GtkWidget *end_date_button;


// Arrays für gefundene Zeilen/deren Inhalt
GArray *found_line_numbers = NULL;
GArray *found_line_contents = NULL;
GString *line_numbers = NULL;

// Boolean-Variablen für Such-Logik
gboolean search_performed = FALSE;
gboolean lines_displayed = FALSE;
gboolean lines_full_displayed = FALSE;
gboolean displaying_full = FALSE;

gboolean check_file(const char *filename);
gboolean valid_text_file_selected = FALSE;

// ---------- INIT ----------
// Methode zur Anzeige der Help Funktion in der MenuBar
void help_info(GtkWidget *widget, gpointer data) {
    GString *result = g_string_new("");
    g_string_append_printf(result, "PiHole LogReader\n");
        g_string_append_printf(result, "Software to open text files and search for Strings with filter options in the textfiles. \n");
    g_string_append_printf(result, "\n");
    g_string_append_printf(result, "Menu Bar\n");
    g_string_append_printf(result, "File -> Open          | (Clicking the top button) Opens the File Selector.  \n");
    g_string_append_printf(result, "File -> Search        | (Clicking the 'Search in File' button) Searches in the Logfile with given Filters. \n");
    g_string_append_printf(result, "File -> Quit           | Closes the software. \n");
    g_string_append_printf(result, "Log -> Create Log  | Creates a logfile in the subfolder 'Logs' (logs are also created on exit). \n");
    g_string_append_printf(result, "Log -> Open Log   | Displays the contents of the selected logfile. \n");
    g_string_append_printf(result, "Filters -> Reset     | Resets the filters to default value for a new search. \n");
    g_string_append_printf(result, "\n");
    g_string_append_printf(result, "GUI \n");
    g_string_append_printf(result, "Top Button                   | Opens the File Selector.\n");
    g_string_append_printf(result, "Enter Search Text          | Search for specific String in LogFile. \n");
    g_string_append_printf(result, "Filter Dropdown Menu  | Only show results from specified type. \n");
    g_string_append_printf(result, "Select Start Date           | Only show results on or after specified date. \n");
    g_string_append_printf(result, "Select End Date             | Only show results on or before specified date. \n");
    g_string_append_printf(result, "Search in File                | Searches in the Logfile with given Filters. \n");
    g_string_append_printf(result, "Show Lines                   | Clicking 1x shows first 10 Lines of result, Clicking 2x shows all results. \n");
    gtk_label_set_text(GTK_LABEL(result_label), result->str);
    g_string_free(result, TRUE); 
}

// Methode um Inhalte der Suche nach der Durchführung zu reinigen
void cleanup() {
    if (found_line_numbers) {
        g_array_free(found_line_numbers, TRUE);
    }
    if (found_line_contents) {
        for (int i = 0; i < found_line_contents->len; i++) {
            g_free(g_array_index(found_line_contents, char*, i));
        }
        g_array_free(found_line_contents, TRUE);
    }
}

// ---------- LOG OPERATIONS ----------
// Methode um Logdatei zu initialisieren
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

// Methode um einen Logeintrag in das Log anzuhängen
void add_log_entry(const char *action) {
    time_t now;
    struct tm *local;
    char timestamp[20];

    time(&now);
    local = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", local);

    g_string_append_printf(log_buffer, "[%s] %s\n", timestamp, action);
}

// Methode um Logdatei zu erstellen (ggf. auch Unterordner Logs)
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

// Methode um Inhalt einer gewählten Logdatei anzuzeigen
void open_log(GtkWidget *widget, gpointer data) {
    // Menü zur Dateiauswahl
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
        
        // Datei öffnen und lesen
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

// ---------- FUNCTIONS ----------
// Methode um Datum aus String zu extrahieren
int parse_date(const char *date_str, int *year, int *month, int *day) {
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char month_str[4];
    int i;

    // Monat als String und Tag aus date_str extrahieren
    if (sscanf(date_str, "%3s %d", month_str, day) != 2) {
        return 0;
    }

    for (i = 0; i < 12; i++) {
        if (strcmp(month_str, months[i]) == 0) {
            *month = i + 1;
            break;
        }
    }

    // Nicht gefunden
    if (i == 12) {
        return 0;
    }

    // Aktuelles Jahr
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    *year = tm_now->tm_year + 1900;

    return 1;
}

// Methode um zwei Daten zu vergleichen
int compare_dates(int year1, int month1, int day1, int year2, int month2, int day2) {
    if (year1 < year2) return -1;
    if (year1 > year2) return 1;
    if (month1 < month2) return -1;
    if (month1 > month2) return 1;
    if (day1 < day2) return -1;
    if (day1 > day2) return 1;
    return 0;
}

// Methode um Kalender zur Datumsauswahl anzuzeigen
void show_calendar(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkWidget *calendar;
    int is_end_date = GPOINTER_TO_INT(data);

    dialog = gtk_dialog_new_with_buttons("Select Date",
                                         GTK_WINDOW(window),
                                         GTK_DIALOG_MODAL,
                                         "_OK", GTK_RESPONSE_OK,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);

    calendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), calendar, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);

    // Wenn erfolgreich ausgewählt
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        guint year, month, day;
        gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);
        char date_str[11];
        sprintf(date_str, "%04d-%02d-%02d", year, month + 1, day);
        if (is_end_date) {
            gtk_button_set_label(GTK_BUTTON(end_date_button), date_str);
        } else {
            gtk_button_set_label(GTK_BUTTON(start_date_button), date_str);
        }
    }

    gtk_widget_destroy(dialog);
}

// ---------- FILE OPERATIONS ----------
// Methode um Datei auszuwählen
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

// Funktion zur Überprüfung der ausgewählten Datei
gboolean check_file(const char *filename) {
    // Überprüfen ob Datei geöffnet werden kann
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not open file.");
    add_log_entry(g_strdup_printf("Couldn't open file. Path: %s", filename));
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    // Überprüfen ob es sich um eine Textdatei handelt
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

    // Überprüfen des MIME-Typs
    const char *mime_type = magic_file(magic, filename);
    if (mime_type == NULL || strncmp(mime_type, "text/", 5) != 0) {
        magic_close(magic);
        fclose(file);
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Not a text file.");
    add_log_entry(g_strdup_printf("File is not a text file. Path: %s", filename));
        valid_text_file_selected = FALSE;
        return FALSE;
    }

    // Schließen der Datei und Rückgabe des Ergebnisses
    magic_close(magic);
    fclose(file);
    gtk_label_set_text(GTK_LABEL(result_label), "File selected successfully.");
    add_log_entry(g_strdup_printf("File selected. Path: %s", filename));
    valid_text_file_selected = TRUE;
    return TRUE;
}

// Wenn Datei ausgewählt wird, Überprüfung starten
void on_file_set(GtkFileChooserButton *chooser, gpointer user_data) {
    const char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    if (filename) {
        check_file(filename);
    }
}

// ---------- SEARCHING ----------
// Funktion zur Durchführung der Suche in der ausgewählten Datei
void search_file(GtkWidget *widget, gpointer data) {
    // Überprüfen ob gültige Texttdatei ausgewählt wurde
    if (!valid_text_file_selected) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please select a valid text file before searching.");
        return;
    }

    // Auslesen der Suchkriterien und Filter
    const gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry));
    const gchar *filter_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(filter_combo));

    const gchar *start_date_str = gtk_button_get_label(GTK_BUTTON(start_date_button));
    const gchar *end_date_str = gtk_button_get_label(GTK_BUTTON(end_date_button));

    // Initialisierung der Datumsvariablen und Überprüfung der Datumsfilter
    int start_year = 0, start_month = 0, start_day = 0;
    int end_year = 9999, end_month = 12, end_day = 31;  
    int use_date_filter = 0;

    // Logik zur Verarbeitung der Datumsfilter
    if (strcmp(start_date_str, "Select Start Date") != 0) {
        sscanf(start_date_str, "%d-%d-%d", &start_year, &start_month, &start_day);
        use_date_filter = 1;
    }

    if (strcmp(end_date_str, "Select End Date") != 0) {
        sscanf(end_date_str, "%d-%d-%d", &end_year, &end_month, &end_day);
        use_date_filter = 1;
    }

    // Erstellen eines Logeintrags für die Suche
    add_log_entry(g_strdup_printf("Searched in file for String \"%s\" with filter \"%s\" and date range %s to %s", 
                                  search_text, filter_text, start_date_str, end_date_str));

    if (filename == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please select a file.");
        return;
    }

    // Öffnen der ausgewählten Datei
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        gtk_label_set_text(GTK_LABEL(result_label), "Error: Could not open file.");
        return;
    }

    // Initialisierung der Variablen für die Suchschleife
    char line[1024];
    int line_number = 0;
    int found_count = 0;
    line_numbers = g_string_new("");
    int numbers_in_current_line = 0;
    found_line_numbers = g_array_new(FALSE, FALSE, sizeof(int));
    found_line_contents = g_array_new(FALSE, TRUE, sizeof(char*));

    // Überprüfung ob Textkriterien für die Suche vorhanden sind
    gboolean no_text_criteria = (strlen(search_text) == 0 && strcmp(filter_text, "No filter") == 0);

    // Hauptsuchschleife: Durchsucht jede Zeile der Datei
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        // Standardannahme das Zeile ein Treffer ist
        gboolean line_matches = TRUE; 

        // Überprüfung der Textkriterien 
        if (!no_text_criteria) {
            // Logik zur Überprüfung von Suchtext
            if (strlen(search_text) > 0) {
                line_matches = (strstr(line, search_text) != NULL);
            }
            
            // Logik zur Überprüfung von Filter
            if (strcmp(filter_text, "No filter") != 0) {
                line_matches = line_matches && (strstr(line, filter_text) != NULL);
            }
        }

        // Überprüfung des Datumsfilters (falls aktiviert)
        if (use_date_filter) {
            int year, month, day;
            if (parse_date(line, &year, &month, &day)) {
                // Datum ist in Zeitraum
                if (compare_dates(year, month, day, start_year, start_month, start_day) >= 0 &&
                    compare_dates(year, month, day, end_year, end_month, end_day) <= 0) {
                } else {
                    line_matches = FALSE;
                }
            } else {
                line_matches = FALSE;
            }
        }

        // Speichern der übereinstimmenden Zeilen
        if (line_matches) {
            found_count++;
            g_array_append_val(found_line_numbers, line_number);
            char *line_copy = g_strdup(line);
            g_array_append_val(found_line_contents, line_copy);
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

    search_performed = TRUE;
    fclose(file);

    // Anzeige der Suchergebnisse in einem String
    GString *result = g_string_new("");
    if (found_count > 0) {
        // Formatierung der Ergebnisausgabe basierend auf den Suchkriterien
        if (no_text_criteria && !use_date_filter) {
            g_string_append_printf(result, "Showing all %d lines in the file\n", found_count);
        } else if (no_text_criteria && use_date_filter) {
            g_string_append_printf(result, "Found %d entries within the specified date range\n", found_count);
            g_string_append_printf(result, "Matching entries in lines: %s", line_numbers->str);
        } else {
            g_string_append_printf(result, "Found %d matching entries\n", found_count);
            g_string_append_printf(result, "Matching entries in lines: %s", line_numbers->str);
        }
        gtk_label_set_text(GTK_LABEL(result_label), result->str);
        add_log_entry(g_strdup_printf("Found %d matching entries", found_count));
        if (!no_text_criteria || use_date_filter) {
            add_log_entry(g_strdup_printf("Matching entries in lines: %s", line_numbers->str));
        }
    } else {
        gtk_label_set_text(GTK_LABEL(result_label), "No matching entries found.");
    }

    // Aufräumen und zurücksetzen der Variablen
    g_string_free(result, TRUE);
    g_string_free(line_numbers, TRUE);

    lines_displayed = FALSE;
    displaying_full = FALSE;
    lines_full_displayed = FALSE;

    g_free((gchar*)filter_text);
}

// Methode um Inhalt der Ergebniszeilen anzuzeigen
void show_lines(GtkWidget *widget, gpointer data) {
    // Falls vor Suche geklickt
    if (!search_performed) {
        gtk_label_set_text(GTK_LABEL(result_label), "Please perform a search first.");
        return;
    }

    // Basic-Info oben im String anhängen
    GString *result = g_string_new("");
    g_string_append_printf(result, "Found in %d lines.\n", found_line_numbers->len);
    g_string_append(result, "Found in lines: \n");

    // Text aus Zeilen anzeigen
    for (int i = 0; i < found_line_numbers->len; i++) {
        int line_number = g_array_index(found_line_numbers, int, i);
        char *line_content = g_array_index(found_line_contents, char*, i);

        if (i < 10) {  //Erste 10 Zeilen
            g_string_append_printf(result, "L%d: %s", line_number, line_content);
        }

        if ((i >= 10) & (lines_displayed == TRUE)) {
            g_string_append_printf(result, "L%d: %s", line_number, line_content);
            displaying_full = TRUE;
        }

    }

    // Logik um erst 10, danach alle Zeilen anzuzeigen
    if (lines_displayed == FALSE) {
        lines_displayed = TRUE;
    }
    if ((lines_full_displayed == FALSE) & (displaying_full == TRUE)) {
        lines_full_displayed = TRUE;
    }

    if ((found_line_numbers->len > 10) & (lines_full_displayed == FALSE)) {
        g_string_append(result, "\n(Showing first 10 lines. Click again to see more.)");
    }

    gtk_label_set_text(GTK_LABEL(result_label), result->str);
    g_string_free(result, TRUE);
}

// Filter zurücksetzen
void reset_filters(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(search_entry), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo), 0);
    gtk_button_set_label(GTK_BUTTON(start_date_button), "Select Start Date");
    gtk_button_set_label(GTK_BUTTON(end_date_button), "Select End Date");
    gtk_label_set_text(GTK_LABEL(result_label), "Filters have been reset.");
}

// ---------- MAIN / GUI ----------
// Initialisierung der MenuBar
GtkWidget* create_menu_bar() {
    // Anlegen der Variablen für Objekte
    GtkWidget *menu_bar;
    GtkWidget *file_menu;
    GtkWidget *log_menu;
    GtkWidget *help_menu;
    GtkWidget *filters_menu;
    GtkWidget *file_mi;
    GtkWidget *log_mi;
    GtkWidget *help_mi;
    GtkWidget *filters_mi;
    GtkWidget *open_mi;
    GtkWidget *search_mi;
    GtkWidget *quit_mi;
    GtkWidget *create_log_mi;
    GtkWidget *open_log_mi;
    GtkWidget *help_help_mi;
    GtkWidget *reset_filters_mi;

    // Menu-Bar einrichten
    menu_bar = gtk_menu_bar_new();

    // Verschiedene Menüs anlegen
    file_menu = gtk_menu_new();
    log_menu = gtk_menu_new();
    filters_menu = gtk_menu_new();
    help_menu = gtk_menu_new();

    // Menüs mit Label versehen
    file_mi = gtk_menu_item_new_with_label("File");
    log_mi = gtk_menu_item_new_with_label("Log");
    help_mi = gtk_menu_item_new_with_label("Help");
    filters_mi = gtk_menu_item_new_with_label("Filters");

    // Menüs als Submenü der MenuBar setzen
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_mi), file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(log_mi), log_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_mi), help_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(filters_mi), filters_menu);

    // Unterpunkte für File anlegen
    open_mi = gtk_menu_item_new_with_label("Open");
    search_mi = gtk_menu_item_new_with_label("Search");
    quit_mi = gtk_menu_item_new_with_label("Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), search_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_mi);

    // Unterpunkte für Log anlegen
    create_log_mi = gtk_menu_item_new_with_label("Create Log");
    open_log_mi = gtk_menu_item_new_with_label("Open Log");

    gtk_menu_shell_append(GTK_MENU_SHELL(log_menu), create_log_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(log_menu), open_log_mi);

    // Unterpunkte für Filter anlegen
    reset_filters_mi = gtk_menu_item_new_with_label("Reset");
    gtk_menu_shell_append(GTK_MENU_SHELL(filters_menu), reset_filters_mi);

    // Unterppunkte für Help anlegen
    help_help_mi = gtk_menu_item_new_with_label("Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_help_mi);

    // Die Untermenüs an die MenuBar anhängen
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), log_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), filters_mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_mi);

    // Verbinden der Menüobjekte mit entsprechenden Funktionen
    g_signal_connect(G_OBJECT(open_mi), "activate", G_CALLBACK(open_file), NULL);
    g_signal_connect(G_OBJECT(search_mi), "activate", G_CALLBACK(search_file), NULL);
    g_signal_connect(G_OBJECT(quit_mi), "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(create_log_mi), "activate", G_CALLBACK(create_log_file), NULL);
    g_signal_connect(G_OBJECT(open_log_mi), "activate", G_CALLBACK(open_log), NULL);
    g_signal_connect(G_OBJECT(help_help_mi), "activate", G_CALLBACK(help_info), NULL);
    g_signal_connect(G_OBJECT(reset_filters_mi), "activate", G_CALLBACK(reset_filters), NULL);

    return menu_bar;
}

// Hauptfunktion zur Initialisierung der GUI und Programmstart
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    init_log();

    // Erstellen des Hauptfensters und der GUI Elemente
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Search");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 400, 300);

    // Weitere GUI Elemente initialisieren
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    // File-Chooser zur Dateiauswahl
    file_chooser = gtk_file_chooser_button_new("Select a File", GTK_FILE_CHOOSER_ACTION_OPEN);
    g_signal_connect(G_OBJECT(file_chooser), "file-set", G_CALLBACK(on_file_set), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), file_chooser, FALSE, FALSE, 0);

    // Abstand zwischen File-Chooser und Textfeld
    GtkWidget *spacing1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_size_request(spacing1, -1, 20);
    gtk_box_pack_start(GTK_BOX(vbox), spacing1, FALSE, FALSE, 0);

    // Textfeld für Suche
    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Enter search text");
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);

    // Drop-Down-Menü zur Filterauswahl
    filter_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), "No filter");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), "gravity blocked");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(filter_combo), "reply");
    gtk_combo_box_set_active(GTK_COMBO_BOX(filter_combo), 0);
    gtk_box_pack_start(GTK_BOX(vbox), filter_combo, FALSE, FALSE, 0);

    // Datumsauswahl für Von-Zeitraum
    start_date_button = gtk_button_new_with_label("Select Start Date");
    g_signal_connect(start_date_button, "clicked", G_CALLBACK(show_calendar), GINT_TO_POINTER(0));
    gtk_box_pack_start(GTK_BOX(vbox), start_date_button, FALSE, FALSE, 0);

    // Datumsauswahl für Bis-Zeitraum
    end_date_button = gtk_button_new_with_label("Select End Date");
    g_signal_connect(end_date_button, "clicked", G_CALLBACK(show_calendar), GINT_TO_POINTER(1));
    gtk_box_pack_start(GTK_BOX(vbox), end_date_button, FALSE, FALSE, 0);

    // Abstand zwischen Datumsauswahl und Suchfunktionen
    GtkWidget *spacing2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_size_request(spacing2, -1, 20);
    gtk_box_pack_start(GTK_BOX(vbox), spacing2, FALSE, FALSE, 0);

    // Suchbutton
    search_button = gtk_button_new_with_label("Search in File");
    g_signal_connect(search_button, "clicked", G_CALLBACK(search_file), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), search_button, FALSE, FALSE, 0);

    // Zeilenanzeige
    show_lines_button = gtk_button_new_with_label("Show Lines");
    g_signal_connect(show_lines_button, "clicked", G_CALLBACK(show_lines), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), show_lines_button, FALSE, FALSE, 0);

    // Textfeld für die Darstellung von Ergebnissen oder Hife
    result_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), result_label, FALSE, FALSE, 0);

    // Verbinden der Signale mit den entsprechenden Funktionen
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(create_log_file), NULL);  // Create log file on exit
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_string_free), log_buffer);

    // Anzeigen aller Widgets
    gtk_widget_show_all(window);

    g_signal_connect(window, "destroy", G_CALLBACK(cleanup), NULL);

    // Initialisieren von Ergebnissen
    found_line_numbers = g_array_new(FALSE, FALSE, sizeof(int));
    found_line_contents = g_array_new(FALSE, TRUE, sizeof(char*));

    // Start der Hauptschleife
    gtk_main();

    return 0;
}
