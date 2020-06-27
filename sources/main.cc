//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidiplay.h"
#include "ui/paint.h"
#include "utility/paths.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <getopt.h>
#if defined(__linux__)
#include <jack/jack.h>
#include <alsa/asoundlib.h>
#endif

#if defined(__linux__)
static void alsa_log_callback(const char *, int, const char *, int, const char *, ...)
{
    // just get rid of these verbose alsa logs
}
#endif

int main(int argc, char *argv[])
{
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
    SDL_Event event;
    bool shutting_down = false;
    while (!app.should_quit() && SDL_WaitEvent(&event)) {
        bool update = false;

        switch (event.type) {
        case SDL_KEYDOWN:
            if (!shutting_down)
                update = app.handle_key_pressed(event.key);
            break;
        case SDL_KEYUP:
            if (!shutting_down)
                update = app.handle_key_released(event.key);
            break;
        case SDL_TEXTINPUT:
            if (!shutting_down)
                update = app.handle_text_input(event.text);
            break;
        case SDL_WINDOWEVENT:
            update = true;
            break;
        case SDL_USEREVENT:
            update = true;
            if (!shutting_down) {
                app.request_update();
                app.update_modals();
            }
            app.advance_shutdown();
            break;
        case SDL_QUIT:
            shutting_down = true;
            app.engage_shutdown();
            break;
        default:
            //Log::i("SDL event %X", event.type);
            break;
        }

        if (update) {
            app.paint_cached_background(rr);
            app.paint(rr, Pt_Foreground);
            SDL_RenderPresent(rr);
        }
    }

    Log::i("Quit");

    return 0;
}
