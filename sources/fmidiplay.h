//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "configuration.h"
#include "utility/geometry.h"
#include "utility/SDL++.h"
#include <SDL.h>
#include <gsl/gsl>
#include <mutex>
#include <vector>
#include <memory>
class File_Browser;
struct File_Entry;
class Metadata_Display;
class Modal_Box;
class Player;
struct Player_State;
class Main_Layout;
struct Midi_Output;

class Application
{
public:
    static const Point size_;

    Application();
    ~Application();

    SDL_Window *init_window();
    SDL_Renderer *init_renderer();

    void set_scale_factor(SDL_Window *win, unsigned sf);
    void paint(SDL_Renderer *rr, int paint);
    void paint_cached_background(SDL_Renderer *rr);
    bool handle_key_pressed(const SDL_KeyboardEvent &event);
    bool handle_key_released(const SDL_KeyboardEvent &event);
    bool handle_text_input(const SDL_TextInputEvent &event);

    void play_file(const std::string &dir, const File_Entry *entries, size_t index, size_t count);
    void play_random(const std::string &dir, const File_Entry &entry);
    void set_current_path(const std::string &path);
    static bool filter_file_name(const std::string &name);
    static bool filter_file_entry(const File_Entry &ent);

    void request_update();
    void update_modals();
    void choose_midi_output(bool ask, gsl::cstring_span choice);
    void get_midi_outputs(std::vector<Midi_Output> &outputs);

    void choose_theme(gsl::cstring_span choice);
    void get_themes(std::vector<std::string> &themes);

    void load_theme(gsl::cstring_span theme);
    void load_default_theme();
    void load_theme_configuration(const CSimpleIniA &ini);

    void engage_shutdown();
    void advance_shutdown();
    bool should_quit() const;

private:
    std::unique_ptr<CSimpleIniA> initialize_config();

private:
    void receive_state_in_other_thread(const Player_State &ps);

private:
    SDLpp_Window_u window_;
    SDLpp_Renderer_u renderer_;

    SDL_TimerID update_timer_ = 0;

    std::unique_ptr<Main_Layout> layout_;
    std::vector<std::unique_ptr<Modal_Box>> modal_;
    SDLpp_Texture_u cached_background_;

    SDLpp_Surface_u logo_image_;
    SDLpp_Surface_u wallpaper_image_;

    bool fadeout_engaged_ = false;
    int fadeout_time_ = 0;

    unsigned scale_factor_ = 1;
    std::unique_ptr<File_Browser> file_browser_;
    std::unique_ptr<Metadata_Display> metadata_display_;
    std::unique_ptr<Player> player_;

    std::string last_midi_output_choice_;
    std::string last_theme_choice_;

    std::unique_ptr<Player_State> ps_;
    std::mutex ps_mutex_;

    enum Info_Mode {
        Info_File,
        Info_Metadata,
        Info_Mode_Count,
    };
    Info_Mode info_mode_ = Info_File;
};
