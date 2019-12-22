//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

class File_Browser_Model;

class Modal_Box {
public:
    Modal_Box(const Rect &bounds, std::string title)
        : bounds_(bounds), title_(std::move(title)) {}
    virtual ~Modal_Box() {}

    bool has_completed() const noexcept { return complete_; }
    void paint(SDL_Renderer *rr);

    virtual bool get_completion_result(size_t index, void *dst) { return false; }

    virtual bool handle_key_pressed(const SDL_KeyboardEvent &event) = 0;
    virtual bool handle_key_released(const SDL_KeyboardEvent &event) = 0;

    std::function<void ()> CompletionCallback;

protected:
    void finish();
    virtual void paint_contents(SDL_Renderer *rr, const Rect &bounds) = 0;

protected:
    const Rect bounds_;
    std::string title_;

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

    /* 0: (size_t)             selection index
       1: (gsl::cstring_span)  selection text  */
    bool get_completion_result(size_t index, void *dst) override;

    virtual bool handle_key_pressed(const SDL_KeyboardEvent &event) override;
    virtual bool handle_key_released(const SDL_KeyboardEvent &event) override;

protected:
    void paint_contents(SDL_Renderer *rr, const Rect &bounds) override;

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
