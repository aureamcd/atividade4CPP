#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>

#include "httplib.h"

// Estrutura para manter os ponteiros dos widgets
struct AppWidgets {
    GtkWidget *entry_host;
    GtkWidget *entry_port;
    GtkWidget *file_label;
    GtkWidget *result_view;
    gchar* selected_filepath = nullptr;
};

// Estrutura para passar o resultado da thread para a função de atualização da UI
struct ThreadResultData {
    GtkTextBuffer *buffer;
    std::string log_entry;
};

// Função auxiliar para obter o timestamp atual como string
std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

bool ler_conteudo_arquivo(const std::string& path, std::string& conteudo) {
    std::ifstream file(path);
    if (!file.is_open()) { return false; }
    std::stringstream buffer;
    buffer << file.rdbuf();
    conteudo = buffer.str();
    return true;
}

static gboolean append_log_and_scroll(gpointer user_data) {
    ThreadResultData* data = static_cast<ThreadResultData*>(user_data);
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(data->buffer, &end_iter);
    gtk_text_buffer_insert(data->buffer, &end_iter, data->log_entry.c_str(), -1);
    GtkTextMark* insert_mark = gtk_text_buffer_get_insert(data->buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(data->buffer), "result_view_widget")), insert_mark, 0.0, TRUE, 0.5, 0.5);
    delete data;
    return G_SOURCE_REMOVE;
}

void http_request_thread(AppWidgets *widgets) {
    const gchar *host_g = gtk_entry_get_text(GTK_ENTRY(widgets->entry_host));
    const gchar *port_g = gtk_entry_get_text(GTK_ENTRY(widgets->entry_port));
    std::string log_entry;
    std::string timestamp = get_current_timestamp();

    if (!widgets->selected_filepath || strlen(widgets->selected_filepath) == 0) {
        log_entry = "\n[" + timestamp + "] --------------------\nERRO: Nenhum arquivo selecionado.\n";
    } else {
        std::string host = host_g;
        int port = std::stoi(port_g);
        std::string path_arquivo = widgets->selected_filepath;
        std::string conteudo_arquivo;
        if (!ler_conteudo_arquivo(path_arquivo, conteudo_arquivo)) {
            log_entry = "\n[" + timestamp + "] --------------------\nERRO: Não foi possível abrir o arquivo '" + path_arquivo + "'.\n";
        } else {
            httplib::Client cli(host, port);
            cli.set_connection_timeout(5, 0);
            log_entry = "\n[" + timestamp + "] --------------------\n";
            log_entry += "Enviando para " + host + ":" + std::to_string(port) + " (Arquivo: " + path_arquivo.substr(path_arquivo.find_last_of("/\\") + 1) + ")...\n";
            auto res = cli.Post("/processar", conteudo_arquivo, "text/plain");
            if (res) {
                log_entry += "Status: " + std::to_string(res->status) + "\n";
                log_entry += "Resposta:\n" + res->body + "\n";
            } else {
                auto err = res.error();
                log_entry += "ERRO na requisição HTTP: " + httplib::to_string(err) + "\n";
            }
        }
    }
    
    ThreadResultData *data = new ThreadResultData();
    data->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->result_view));
    data->log_entry = log_entry;
    g_idle_add(append_log_and_scroll, data);
}

static void on_select_file_clicked(GtkButton *button, gpointer user_data) {
    (void)button; AppWidgets *widgets = static_cast<AppWidgets*>(user_data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Selecionar Arquivo", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancelar", GTK_RESPONSE_CANCEL, "_Abrir", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        if (widgets->selected_filepath) g_free(widgets->selected_filepath);
        widgets->selected_filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::string fullpath = widgets->selected_filepath;
        size_t last_slash = fullpath.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos) ? fullpath : fullpath.substr(last_slash + 1);
        gtk_label_set_markup(GTK_LABEL(widgets->file_label), ("<b>Arquivo:</b> " + filename).c_str());
    }
    gtk_widget_destroy(dialog);
}

static void on_send_clicked(GtkButton *button, gpointer user_data) {
    (void)button; AppWidgets *widgets = static_cast<AppWidgets*>(user_data);
    std::thread(http_request_thread, widgets).detach();
}

static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = static_cast<AppWidgets*>(user_data);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Cliente Contador");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 550);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_class(context, "main-window");

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font_desc='Sans Bold 24' foreground='#3A3A3A'>Contador</span>");
    gtk_box_pack_start(GTK_BOX(main_vbox), title_label, FALSE, FALSE, 0);

    GtkWidget *title_separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), title_separator, FALSE, FALSE, 10);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), grid, FALSE, FALSE, 5);

    GtkWidget *host_label = gtk_label_new("Endereço IP (Host):");
    widgets->entry_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_host), "localhost");
    gtk_widget_set_halign(host_label, GTK_ALIGN_START);

    GtkWidget *port_label = gtk_label_new("Porta:");
    widgets->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_port), "8080");
    gtk_widget_set_halign(port_label, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grid), host_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_host, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_port, 1, 1, 1, 1);

    GtkWidget *buttons_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttons_hbox, GTK_ALIGN_CENTER);
    
    GtkWidget *select_button = gtk_button_new_with_label("Selecionar Arquivo");
    GtkWidget *select_icon = gtk_image_new_from_icon_name("document-open-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(select_button), select_icon);
    gtk_button_set_always_show_image(GTK_BUTTON(select_button), TRUE);

    GtkWidget *send_button = gtk_button_new_with_label("Enviar Requisição");
    GtkWidget *send_icon = gtk_image_new_from_icon_name("mail-send-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(send_button), send_icon);
    gtk_button_set_always_show_image(GTK_BUTTON(send_button), TRUE);
    GtkStyleContext *send_btn_ctx = gtk_widget_get_style_context(send_button);
    gtk_style_context_add_class(send_btn_ctx, "send-button");

    gtk_box_pack_start(GTK_BOX(buttons_hbox), select_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttons_hbox), send_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), buttons_hbox, FALSE, FALSE, 5);

    widgets->file_label = gtk_label_new("Nenhum arquivo selecionado.");
    gtk_label_set_markup(GTK_LABEL(widgets->file_label), "<i>Nenhum arquivo selecionado</i>");
    gtk_widget_set_halign(widgets->file_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), widgets->file_label, FALSE, FALSE, 5);

    GtkWidget *results_frame = gtk_frame_new("Log de Atividades");
    gtk_widget_set_vexpand(results_frame, TRUE);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    widgets->result_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->result_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widgets->result_view), GTK_WRAP_WORD_CHAR);
    
    // *** ALTERAÇÃO AQUI: Linha de centralização REMOVIDA ***
    // gtk_text_view_set_justification(GTK_TEXT_VIEW(widgets->result_view), GTK_JUSTIFY_CENTER);

    // *** ALTERAÇÃO AQUI: Adicionando margens internas ao log ***
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(widgets->result_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(widgets->result_view), 10);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(widgets->result_view), 5);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(widgets->result_view), 5);

    g_object_set_data(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->result_view))), "result_view_widget", widgets->result_view);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->result_view)), "Aguardando ação do usuário...", -1);

    gtk_container_add(GTK_CONTAINER(scrolled_window), widgets->result_view);
    gtk_container_add(GTK_CONTAINER(results_frame), scrolled_window);
    gtk_box_pack_start(GTK_BOX(main_vbox), results_frame, TRUE, TRUE, 0);

    g_signal_connect(select_button, "clicked", G_CALLBACK(on_select_file_clicked), widgets);
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_clicked), widgets);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    AppWidgets widgets;
    GtkApplication *app = gtk_application_new("com.example.clientehttp", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &widgets);
    
    // *** ALTERAÇÃO AQUI: CSS Atualizado para o log claro e botão verde ***
    g_signal_connect(app, "startup", G_CALLBACK(+[](GApplication *application) {
        (void)application;
        GtkCssProvider *provider = gtk_css_provider_new();
        const char *css =
            ".main-window { background-color: #f5f5f5; }"
            "label { color: #4a4a4a; }"
            "entry { padding: 6px; border: 1px solid #dbdbdb; border-radius: 4px; }"
            "entry:focus { border-color: #3273dc; box-shadow: 0 0 0 0.125em rgba(50, 115, 220, 0.25); }"
            "button { "
                "font-weight: bold; border-radius: 5px; padding: 10px 18px; border: 1px solid #c0c0c0; "
                "background-image: linear-gradient(to bottom, #ffffff, #f0f0f0); "
                "box-shadow: 0 1px 1px rgba(0,0,0,0.05); transition: all 0.2s ease-in-out;"
            "}"
            "button:hover { background-image: linear-gradient(to bottom, #fafafa, #e0e0e0); }"
            "button:active { background-image: linear-gradient(to top, #fafafa, #e0e0e0); box-shadow: inset 0 1px 2px rgba(0,0,0,0.1); }"
            
            /* Botão de Enviar em VERDE */
            ".send-button { "
                "background-image: linear-gradient(to bottom, #48c774, #3ec46d); color: white; border-color: #3abb67;"
            "}"
            ".send-button:hover { background-image: linear-gradient(to bottom, #42b86a, #38ad60); }"
            ".send-button:active { background-image: linear-gradient(to top, #42b86a, #38ad60); }"
            
            /* Log com fundo BRANCO e texto ESCURO */
            "textview { "
                "font-family: Monospace; background-color: #ffffff; color: #363636; border-radius: 5px; "
            "}"
            "frame { padding: 5px; border-radius: 6px; border: 1px solid #dbdbdb; }";
        
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);
    }), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    if (widgets.selected_filepath) g_free(widgets.selected_filepath);
    g_object_unref(app);

    return status;
}