//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "tray.h"
#include "utility/strings.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <string>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <cstdint>

#if defined(_WIN32) || defined(APPLE)

// TODO: no implementation on these OS

Tray_Icon::Tray_Icon()
{
}

Tray_Icon::~Tray_Icon()
{
}

#else

#include <whereami.h>
#define GDK_DISABLE_DEPRECATION_WARNINGS 1
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cerrno>

struct Tray_Icon::Impl {
    pid_t pid_ = -1;
    struct FD {
        ~FD() { if (fd != -1) close(fd); }
        explicit operator int() const { return fd; }
        int fd = -1;
    };
    FD reader_;
    FD writer_;
};

Tray_Icon::Tray_Icon()
    : impl_(new Impl)
{
    Impl &impl = *impl_;

    GError *error = nullptr;
    char *exe = g_file_read_link("/proc/self/exe", &error);
    if (error) {
        g_error_free(error);
        throw std::runtime_error("Cannot locate the executable");
    }
    auto exe_cleanup = gsl::finally([exe]() { g_free(exe); });

    pid_t pid = vfork();
    if (pid == -1)
        throw std::system_error(errno, std::generic_category());
    impl.pid_ = pid;
    if (pid == 0) {
        char *argv[] = { const_cast<char *>("smf-dsp-tray"), nullptr };
        execv(exe, argv);
        _exit(1);
    }
}

Tray_Icon::~Tray_Icon()
{
    Impl &impl = *impl_;
    kill(impl.pid_, SIGTERM);
    waitpid(impl.pid_, nullptr, 0);
}

///
class Gtk_Tray_Application {
public:
    Gtk_Tray_Application();
    ~Gtk_Tray_Application();
    int exec();
    static void activate(GtkApplication *app, gpointer user_data);

private:
    static void on_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data);

private:
    static void setup_icon_path(GtkIconTheme *theme);

private:
    GtkApplication *app_ = nullptr;
    GtkWidget *window_ = nullptr;
    GtkWidget *icon_menu_ = nullptr;
};

Gtk_Tray_Application::Gtk_Tray_Application()
{
    GtkApplication *app = app_ = gtk_application_new("io.github.jpcima.smf_dsp", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), this);
}

Gtk_Tray_Application::~Gtk_Tray_Application()
{
    g_object_unref(window_);
    g_object_unref(app_);
}

int Gtk_Tray_Application::exec()
{
    GtkApplication *app = app_;
    return g_application_run(G_APPLICATION(app), 0, nullptr);
}

void Gtk_Tray_Application::activate(GtkApplication *app, gpointer user_data)
{
    Gtk_Tray_Application *self = reinterpret_cast<Gtk_Tray_Application *>(user_data);

    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
    setup_icon_path(icon_theme);

    GtkWidget *window = self->window_ = gtk_application_window_new(app);
    (void)window;

    GtkStatusIcon *icon = gtk_status_icon_new_from_icon_name("smf-dsp");
    gtk_status_icon_set_visible(icon, true);

    g_signal_connect(icon, "popup-menu", (GCallback)&on_popup_menu, self);
}

void Gtk_Tray_Application::on_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
    Gtk_Tray_Application *self = reinterpret_cast<Gtk_Tray_Application *>(user_data);

    GtkWidget *icon_menu = self->icon_menu_;
    if (!icon_menu) {
        icon_menu = gtk_menu_new();

        GtkWidget *item_quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, nullptr);
        gtk_menu_shell_append(GTK_MENU_SHELL(icon_menu), item_quit);

        self->icon_menu_ = icon_menu;
    }

    gtk_widget_show_all(icon_menu);
    gtk_menu_popup(
        GTK_MENU(icon_menu), nullptr, nullptr,
        &gtk_status_icon_position_menu, status_icon, button, activate_time);
}

void Gtk_Tray_Application::setup_icon_path(GtkIconTheme *theme)
{
    std::unique_ptr<char[]> path;
    unsigned full_length = wai_getExecutablePath(nullptr, 0, nullptr);
    unsigned dir_length;

    if ((int)full_length != -1) {
        path.reset(new char[full_length + 1]);
        full_length = wai_getExecutablePath(path.get(), full_length, (int *)&dir_length);
    }
    if ((int)full_length == -1)
        throw std::runtime_error("cannot locate executable");

    std::string dir(path.get(), dir_length);
    while (!dir.empty() && dir.back() == '/')
        dir.pop_back();

    if (string_ends_with<char>(dir, "bin")) {
        dir.resize(dir.size() - 3);
        if (dir.empty() || dir.back() == '/') {
            dir.append("share/icons");
            gtk_icon_theme_append_search_path(theme, dir.c_str());
        }
    }
}

///
void Tray_Icon::run_gtk_tray(int argc, char *argv[])
{
    if (argv[0] == gsl::cstring_span("smf-dsp-tray")) {
        Gtk_Tray_Application app;
        int status = app.exec();
        exit(status);
    }
}

#endif
