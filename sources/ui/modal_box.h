//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <nonstd/any.hpp>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <cstdint>

class File_Browser_Model;
struct Font;

class Modal_Box {
public:
    Modal_Box(const Rect &bounds, std::string title);
    virtual ~Modal_Box() {}

    bool has_completed() const noexcept { return complete_; }
    void paint(SDL_Renderer *rr);

    template <class T> T completion_result(size_t index)
    {
        return nonstd::any_cast<T>(get_completion_result(index));
    }

    virtual bool handle_key_pressed(const SDL_KeyboardEvent &event) { return false; };
    virtual bool handle_key_released(const SDL_KeyboardEvent &event) { return false; };
    virtual bool handle_text_input(const SDL_TextInputEvent &event) { return false; };

    std::function<void ()> CompletionCallback;

protected:
    virtual nonstd::any get_completion_result(size_t index) { return nonstd::any{}; }

protected:
    void finish();
    virtual void paint_contents(SDL_Renderer *rr) = 0;

protected:
    const Rect bounds_;
    std::string title_;
    Font *font_title_ = nullptr;

    Rect get_content_bounds() const;

private:
    bool complete_ = false;
};

///
class Modal_Selection_Box : public Modal_Box {
public:
    Modal_Selection_Box(const Rect &bounds, std::string title, std::vector<std::string> items);
    virtual ~Modal_Selection_Box() {}

    size_t selection_index() const noexcept { return sel_; }
    void set_selection_index(size_t index) noexcept;

    virtual bool handle_key_pressed(const SDL_KeyboardEvent &event) override;
    virtual bool handle_key_released(const SDL_KeyboardEvent &event) override;

protected:
    /* 0: (size_t)       selection index
       1: (std::string)  selection text  */
    nonstd::any get_completion_result(size_t index) override;

protected:
    void paint_contents(SDL_Renderer *rr) override;

protected:
    size_t sel_ = 0;
    std::vector<std::string> items_;
};

///
class Modal_File_Selection_Box : public Modal_Selection_Box {
public:
    Modal_File_Selection_Box(const Rect &bounds, std::string path);

    bool handle_key_pressed(const SDL_KeyboardEvent &event) override;

private:
    void update_file_entries();

private:
    std::unique_ptr<File_Browser_Model> model_;
};


///
class Modal_Text_Input_Box : public Modal_Box {
public:
    Modal_Text_Input_Box(const Rect &bounds, std::string title);

    bool handle_key_pressed(const SDL_KeyboardEvent &event) override;
    bool handle_key_released(const SDL_KeyboardEvent &event) override;
    bool handle_text_input(const SDL_TextInputEvent &event) override;

protected:
    /* 0: (bool)         accepted
       1: (std::string)  input text  */
    nonstd::any get_completion_result(size_t index) override;

protected:
    void paint_contents(SDL_Renderer *rr) override;

private:
    unsigned get_max_display_chars() const;
    size_t char_display_size(size_t index) const;

    void offset_to_left();
    void offset_to_right();

private:
    size_t cursor_ = 0;
    size_t display_offset_ = 0;
    std::u32string input_text_;
    std::vector<uint8_t> input_gsize_;
    std::string label_;
    Font *font_text_input_ = nullptr;
    bool accepted_ = false;
};
