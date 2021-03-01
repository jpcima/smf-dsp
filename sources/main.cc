//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "application.h"
#include "ui/paint.h"
#include "utility/paths.h"
#include "utility/module.h"
#include "utility/charset.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <getopt.h>
#include <memory>
#include <clocale>
#if defined(__linux__)
#include <jack/jack.h>
#include <alsa/asoundlib.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__linux__)
static void alsa_log_callback(const char *, int, const char *, int, const char *, ...)
{
    // just get rid of these verbose alsa logs
}
#endif

#if !defined(_WIN32)
int main(int argc, char *argv[])
#else
int real_main(int argc, char *argv[])
#endif
{
    // Initialize the locale
    setlocale(LC_ALL, "");

    // Initialize command line
    for (int c; (c = getopt(argc, argv, "")) != -1;) {
        switch (c) {
        default:
            return 1;
        }
    }

    std::string initial_path;
    switch (argc - optind) {
    case 0:
        break;
    case 1:
        initial_path = make_path_canonical(argv[optind]);
        break;
    default:
        Log::e("Invalid number of positional arguments");
        return 1;
    }

    Log::i("Start");

#if defined(__linux__)
    // Disable JACK messages
    jack_set_info_function([](const char *) {});
    jack_set_error_function([](const char *) {});
    // Disable ALSA messages
    snd_lib_error_set_handler(&alsa_log_callback);
#endif

    // Initialize SDL
    uint32_t subsys = SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER;
    if (SDL_InitSubSystem(subsys) < 0) {
        Log::e("Error initializing.");
        return 1;
    }
    auto sdl_cleanup = gsl::finally([subsys] { SDL_QuitSubSystem(subsys); });

    // Initialize App and Window
    Application app;
    if (!initial_path.empty())
        app.set_current_path(initial_path);

    Log::i("Creating window");
    SDL_Window *win = app.init_window();
    if (!win) {
        Log::e("Error creating window");
        return 1;
    }
    Log::s("New window: %p", win);

    Log::i("Creating renderer");
    SDL_Renderer *rr = app.init_renderer();
    if (!rr) {
        Log::e("Error creating window renderer");
        return 1;
    }
    Log::s("New renderer: %p", rr);

    // Initial painting
    app.paint_cached_background(rr);
    app.paint(rr, Pt_Foreground);
    SDL_RenderPresent(rr);

    // Event handling
    app.exec();

    Log::i("Quit");

    return 0;
}

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    initialize_app_module(instance);

    unsigned argc = __argc;
    LPWSTR *wargv = __wargv;

    std::unique_ptr<std::string[]> args{new std::string[argc]};
    for (unsigned i = 0; i < argc; ++i)
      convert_utf<WCHAR, char>(wargv[i], args[i], true);

    std::unique_ptr<char *[]> argv{new char *[argc + 1]};
    argv[argc] = nullptr;
    for (unsigned i = 0; i < argc; ++i)
      argv[i] = const_cast<char *>(args[i].c_str());

    return real_main(argc, argv.get());
}
#endif
