#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <mutex>
#include <memory>
class File_Browser;
struct File_Entry;
class Metadata_Display;
class Player;
struct Player_State;
class Main_Layout;

class Application
{
public:
    static const char title_[];
    static const Point size_;

    Application();
    ~Application();

    void set_scale_factor(SDL_Window *win, unsigned sf);
    void paint(SDL_Renderer *rr, int paint);
    void paint_cached_background(SDL_Renderer *rr);
    bool handle_key_pressed(const SDL_KeyboardEvent &event);
    bool handle_key_released(const SDL_KeyboardEvent &event);

    void play_file(const std::string &dir, const File_Entry *entries, size_t index, size_t count);
    void play_random(const std::string &dir, const File_Entry &entry);
    void set_current_file(const std::string &path);
    static bool filter_file_name(const std::string &name);
    static bool filter_file_entry(const File_Entry &ent);

    void request_update();

    void engage_shutdown();
    void advance_shutdown();
    bool should_quit() const;

private:
    void receive_state_in_other_thread(const Player_State &ps);

private:
    SDL_TimerID update_timer_ = 0;

    std::unique_ptr<Main_Layout> layout_;
    SDL_Texture *cached_background_ = nullptr;

    bool fadeout_engaged_ = false;
    int fadeout_time_ = 0;

    unsigned scale_factor_ = 1;
    std::unique_ptr<File_Browser> file_browser_;
    std::unique_ptr<Metadata_Display> metadata_display_;
    std::unique_ptr<Player> player_;

    std::unique_ptr<Player_State> ps_;
    std::mutex ps_mutex_;

    enum Info_Mode {
        Info_File,
        Info_Metadata,
        Info_Mode_Count,
    };
    Info_Mode info_mode_ = Info_File;
};
