//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidiplay.h"
#include "ui/paint.h"
#include "utility/paths.h"
#include <gsl.hpp>
#include <getopt.h>
#include <cstdio>

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
        initial_path = make_path_absolute(argv[optind]);
        break;
    default:
        fprintf(stderr, "Error: invalid number of positional arguments\n");
        return 1;
    }

    // Initialize SDL
    uint32_t subsys = SDL_INIT_VIDEO|SDL_INIT_TIMER;
    if (SDL_InitSubSystem(subsys) < 0) {
        fprintf(stderr, "Error initializing.\n");
        return 1;
    }
    auto sdl_cleanup = gsl::finally([subsys] { SDL_QuitSubSystem(subsys); });

    // Initialize App and Window
    Application app;
    if (!initial_path.empty())
        app.set_current_file(initial_path);
    SDL_Window *win = SDL_CreateWindow(
        PROGRAM_DISPLAY_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        Application::size_.x, Application::size_.y, 0);
    if (!win) {
        fprintf(stderr, "Error creating window.\n");
        return 1;
    }
    auto window_cleanup = gsl::finally([win] { SDL_DestroyWindow(win); });

    SDL_Renderer *rr = SDL_CreateRenderer(win, -1, 0);
    if (!rr) {
        fprintf(stderr, "Error creating window renderer.\n");
        return 1;
    }
    auto renderer_cleanup = gsl::finally([rr] { SDL_DestroyRenderer(rr); });

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
            // fprintf(stderr, "SDL event %X\n", event.type);
            break;
        }

        if (update) {
            app.paint_cached_background(rr);
            app.paint(rr, Pt_Foreground);
            SDL_RenderPresent(rr);
        }
    }

    return 0;
}
