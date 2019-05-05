#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <vector>
#include <string>
#include <functional>
struct Font;
struct File_Entry;

class File_Browser
{
public:
    explicit File_Browser(const Rect &bounds);

    const std::string &cwd() const noexcept { return cwd_; }
    void set_current_file(const std::string &path);

    size_t current_index() const noexcept { return sel_; }
    size_t current_count() const noexcept { return entries_.size(); }
    const File_Entry &entry(size_t index) const noexcept { return entries_[index]; }

    void set_font(Font *font);
    void set_cwd(const std::string &dir);

    void paint(SDL_Renderer *rr);
    bool handle_key_pressed(const SDL_KeyboardEvent &event);
    bool handle_key_released(const SDL_KeyboardEvent &event);

    std::function<void (const std::string &, const File_Entry *, size_t, size_t)> FileOpenCallback;
    std::function<bool (const File_Entry &)> FileFilterCallback;

private:
    Rect bounds_;
    Font *font_ = nullptr;
    size_t sel_ = 0;
    size_t colindex_ = 0;
    std::string cwd_;
    std::vector<File_Entry> entries_;

private:
    void refresh_file_list();
    void move_selection_by(long offset);
    void focus_on_selection();
    void trigger_entry(const File_Entry &ent);
    void update_dims();
    size_t item_width() const;
    size_t item_height() const;
    size_t rows() const;
    size_t cols() const;
};
