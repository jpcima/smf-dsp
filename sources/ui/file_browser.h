//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "file_browser_model.h"
#include "utility/geometry.h"
#include <SDL.h>
#include <nonstd/span.hpp>
#include <vector>
#include <string>
struct Font;
class File_Entry;

class File_Browser
{
public:
    explicit File_Browser(const Rect &bounds);

    unsigned current_index() const;
    unsigned current_count() const;
    const File_Entry *current_entry() const noexcept;
    nonstd::span<const File_Entry> all_current_entries() const noexcept { return model_.all_entries(); }

    std::string current_path() const;
    void set_current_path(const std::string &path);

    const std::string &cwd() const;
    void set_cwd(const std::string &dir);

    void trigger_selected_entry() { model_.trigger_selected_entry(); };

    void set_font(Font *font);

    void paint(SDL_Renderer *rr);
    bool handle_key_pressed(const SDL_KeyboardEvent &event);
    bool handle_key_released(const SDL_KeyboardEvent &event);
    bool handle_text_input(const SDL_TextInputEvent &event);

    std::function<void (const std::string &, const File_Entry *, size_t, size_t)> &FileOpenCallback;
    std::function<bool (const File_Entry &)> &FileFilterCallback;

private:
    Rect bounds_;
    Font *font_ = nullptr;
    size_t colindex_ = 0;
    File_Browser_Model model_;

private:
    void move_selection_by(long offset);
    void move_to_character(uint32_t character);
    void focus_on_selection();
    void update_dims();
    size_t item_width() const;
    size_t item_height() const;
    size_t rows() const;
    size_t cols() const;
};
