/*
Proyecto 1 IO -PR01
Estudiantes:
Emily -
Viviana -
*/

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>

//Window 1
GtkWidget   *window1;
GtkWidget   *fixed1;
GtkWidget   *title;
GtkWidget   *description;
GtkWidget   *instruction;
GtkWidget   *spinNodes;
GtkWidget   *fileLoad;
GtkWidget   *loadLabel;
GtkWidget   *exitButton1;
GtkWidget   *scrollWindow;
/*//Window 2
GtkWidget   *window2;
GtkWidget   *fixed2;
GtkWidget   *title2;
GtkWidget   *instruction2;
GtkWidget   *instruction3;
GtkWidget   *createSolution;
GtkWidget   *fileName;
GtkWidget   *saveProblem;
GtkWidget   *matrixTable;
GtkWidget   *exitButton2;*/

static GPtrArray *col_headers = NULL; // índices 1..n (0 sin usar)
static GPtrArray *row_headers = NULL; // índices 1..n (0 sin usar)
static gint current_n = 0;
static gboolean syncing_headers = FALSE;

GtkWidget *current_grid = NULL;

GtkBuilder  *builder;
GtkCssProvider *cssProvider;
GtkWidget *pendingWindow;

/* 
Para ejecutar:
Abrir la terminal en el folder principal de Mini Proyecto.
Usar el comando: gcc main.c $(pkg-config --cflags --libs gtk+-3.0) -o main -export-dynamic
(Esto para que pueda correr con libgtk-3.0)
Ejecutar el main con el comando en terminal: ./main
También se puede hacer click en el archivo ejecutable 'Main' en la carpeta principal.
*/

//Función para que se utilice el archivo .css como proveedor de estilos.
void set_css (GtkCssProvider *cssProvider, GtkWidget *widget){
	GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
	gtk_style_context_add_provider(styleContext,GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

/*
Funciones para Window1 1
*/

static gchar* index_to_label(gint index) {
    GString *s = g_string_new(NULL);
    gint n = index;
    while (n > 0) {
        n--;
        gint rem = n % 26;
        g_string_prepend_c(s, 'A' + rem);
        n /= 26;
    }
    return g_string_free(s, FALSE);
}

// Normaliza a solo letras de la A a la Z (Mayúscula). 
static gchar* name_label(const gchar *in, gint index) {
    GString *s = g_string_new(NULL);
    for (const char *p = in; *p; ++p) {
        if (g_ascii_isalpha(*p)) g_string_append_c(s, g_ascii_toupper(*p));
    }
    if (s->len == 0) {
        g_string_free(s, TRUE);
        return index_to_label(index);
    }
    return g_string_free(s, FALSE);
}

static void on_header_changed(GtkEditable *editable, gpointer user_data) {
    if (syncing_headers) return;

    gboolean is_col = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(editable), "is_col"));
    gint index        = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(editable), "index"));
    if (index <= 0 || index > current_n) return;

    const gchar *raw = gtk_entry_get_text(GTK_ENTRY(editable));
    gchar *clean = name_label(raw, index);

    GtkEntry *peer = GTK_ENTRY(
        g_ptr_array_index(is_col ? row_headers : col_headers, index)
    );
    if (!peer) { g_free(clean); return; }

    syncing_headers = TRUE;
    gtk_entry_set_text(GTK_ENTRY(editable), clean);
    gtk_entry_set_text(peer, clean);                
    syncing_headers = FALSE;

    g_free(clean);
}

static void build_matrix_grid(GtkWidget *scrolled, gint n) {
    // Limpieza del grid anterior
    if (current_grid) {
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(scrolled));
        if (child) gtk_container_remove(GTK_CONTAINER(scrolled), child);
        current_grid = NULL;
    }
    if (col_headers) { g_ptr_array_free(col_headers, TRUE); col_headers = NULL; }
    if (row_headers) { g_ptr_array_free(row_headers, TRUE); row_headers = NULL; }

    current_n = n;
    col_headers = g_ptr_array_sized_new(n + 1);
    row_headers = g_ptr_array_sized_new(n + 1);
    g_ptr_array_set_size(col_headers, n + 1); 
    g_ptr_array_set_size(row_headers, n + 1);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);

    //(0,0)
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new(" "), 0, 0, 1, 1);

    // Hacer los headers de columnas editables
    for (gint c = 1; c <= n; c++) {
        GtkWidget *e = gtk_entry_new();
        gchar *txt = index_to_label(c);  
        gtk_entry_set_text(GTK_ENTRY(e), txt);
        g_free(txt);

        g_object_set_data(G_OBJECT(e), "is_col", GINT_TO_POINTER(TRUE));
        g_object_set_data(G_OBJECT(e), "index",    GINT_TO_POINTER(c));
        g_signal_connect(e, "changed", G_CALLBACK(on_header_changed), NULL);

        g_ptr_array_index(col_headers, c) = e;
        gtk_grid_attach(GTK_GRID(grid), e, c, 0, 1, 1);
    }

    //Headers de filas y celdas
    for (gint r = 1; r <= n; r++) {
        GtkWidget *e = gtk_entry_new();
        gchar *txt = index_to_label(r); 
        gtk_entry_set_text(GTK_ENTRY(e), txt);
        g_free(txt);

        g_object_set_data(G_OBJECT(e), "is_col", GINT_TO_POINTER(FALSE));
        g_object_set_data(G_OBJECT(e), "index",    GINT_TO_POINTER(r));
        g_signal_connect(e, "changed", G_CALLBACK(on_header_changed), NULL);

        g_ptr_array_index(row_headers, r) = e;
        gtk_grid_attach(GTK_GRID(grid), e, 0, r, 1, 1);

        for (gint c = 1; c <= n; c++) {
            GtkWidget *entry = gtk_entry_new();
            gtk_entry_set_width_chars(GTK_ENTRY(entry), 6);
            gtk_entry_set_alignment(GTK_ENTRY(entry), 0.5);

            if (r == c) {
                gtk_entry_set_text(GTK_ENTRY(entry), "0");
                gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
                gtk_widget_set_sensitive(entry, FALSE);
            }

            g_object_set_data(G_OBJECT(entry), "row", GINT_TO_POINTER(r));
            g_object_set_data(G_OBJECT(entry), "col", GINT_TO_POINTER(c));
            gtk_grid_attach(GTK_GRID(grid), entry, c, r, 1, 1);
        }
    }

    gtk_widget_show_all(grid);
    gtk_container_add(GTK_CONTAINER(scrolled), grid);
    current_grid = grid;

    //Asegurarse que las columnas y filas mantengan el mismo nombre
    syncing_headers = TRUE;
    for (gint k = 1; k <= n; k++) {
        const gchar *t = gtk_entry_get_text(GTK_ENTRY(g_ptr_array_index(col_headers, k)));
        gtk_entry_set_text(GTK_ENTRY(g_ptr_array_index(row_headers, k)), t);
    }
    syncing_headers = FALSE;
}

//Falta que lea la info ingresada

G_MODULE_EXPORT void on_spinNodes_value_changed(GtkSpinButton *spin, gpointer user_data) {
    gint n = gtk_spin_button_get_value_as_int(spin);
    if (n < 1) n = 1;
    build_matrix_grid(scrollWindow, n);
}



//Muestra la pantalla de pending y carga sus widgets.
void* pending(){
	char * command = "gcc pending.c $(pkg-config --cflags --libs gtk+-3.0) -o pending -export-dynamic";
	system(command);
	system("./pending");
}


/*
Funciones para Window1
*/


void on_loadProblem_clicked (GtkWidget *loadProblem, gpointer data){
	pthread_t thread;
	pthread_create(&thread, NULL, pending, NULL);
}


//Función de acción para el botón de 'Exit' que cierra todo el programa.
void on_exitButton_clicked (GtkButton *exitButton1, gpointer data){
	gtk_main_quit();
}


//Main
int main (int argc, char *argv[]){
	gtk_init(&argc, &argv);
	
	builder =  gtk_builder_new_from_file ("Floyd.glade");
	
	window1 = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
	
	g_signal_connect(window1, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	gtk_builder_connect_signals(builder, NULL);
	
	fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
	title = GTK_WIDGET(gtk_builder_get_object(builder, "title"));
	description = GTK_WIDGET(gtk_builder_get_object(builder, "description"));
	instruction = GTK_WIDGET(gtk_builder_get_object(builder, "instruction"));
	spinNodes = GTK_WIDGET(gtk_builder_get_object(builder, "spinNodes"));
	exitButton1 = GTK_WIDGET(gtk_builder_get_object(builder, "exitButton1"));
	fileLoad = GTK_WIDGET(gtk_builder_get_object(builder, "fileLoad"));
	loadLabel = GTK_WIDGET(gtk_builder_get_object(builder, "loadLabel"));
	scrollWindow = GTK_WIDGET(gtk_builder_get_object(builder, "scrollWindow"));

	cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(cssProvider, "theme.css", NULL);

	set_css(cssProvider, window1);
	set_css(cssProvider, fileLoad);
	set_css(cssProvider, exitButton1);
	set_css(cssProvider, scrollWindow);

	g_signal_connect(exitButton1, "clicked", G_CALLBACK(on_exitButton_clicked), NULL);
	g_signal_connect(fileLoad, "clicked", G_CALLBACK(on_loadProblem_clicked), NULL);
	
	gtk_widget_show(window1);
	
	gtk_main();

	return EXIT_SUCCESS;
	}


