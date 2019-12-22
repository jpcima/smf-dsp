//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "modal_box.h"
#include "color_palette.h"
#include "fonts.h"
#include "text.h"
#include "file_browser_model.h"
#include "utility/paths.h"
#include "utility/SDL++.h"
#include <algorithm>

void Modal_Box::paint(SDL_Renderer *rr)
{
    Rect bounds = bounds_;
    const Color_Palette &pal = Color_Palette::get_current();

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_frame]);
    SDLpp_RenderDrawRect(rr, bounds);

    bounds = bounds.reduced(1);

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_background]);
    SDLpp_RenderFillRect(rr, bounds);

    bounds.chop_from_top(4);

    Text_Painter tp;
    tp.rr = rr;
    tp.font = &font_fmdsp_medium;
    tp.pos = bounds.origin().off_by(Point(2, 0));
    tp.fg = pal[Colors::box_title];
    tp.draw_utf8(title_);

    bounds.chop_from_top(tp.font->height() + 3);

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_frame]);
    SDL_RenderDrawLine(rr, bounds.x, bounds.y, bounds.x + bounds.w - 1, bounds.y);

    bounds.chop_from_top(1);

    paint_contents(rr, bounds.reduced(2));
}

void Modal_Box::finish()
{
    if (!complete_) {
        complete_ = true;
        if (CompletionCallback)
            CompletionCallback();
    }
}

///
Modal_Selection_Box::Modal_Selection_Box(const Rect &bounds, std::string title, std::vector<std::string> items)
    : Modal_Box(bounds, std::move(title)), items_(std::move(items))
{
    if (items_.empty())
        sel_ = ~size_t(0);
}

void Modal_Selection_Box::set_selection_index(size_t index) noexcept
{
    if (index < items_.size())
        sel_ = index;
}

bool Modal_Selection_Box::get_completion_result(size_t index, void *dst)
{
    switch (index) {
    case 0:
        *static_cast<size_t *>(dst) = sel_;
        return true;
    case 1:
        *static_cast<gsl::cstring_span *>(dst) = (sel_ != ~size_t(0)) ?
            items_[sel_] : gsl::cstring_span();
        return true;
    default:
        return false;
    }
}

bool Modal_Selection_Box::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    if (has_completed())
        return false;

    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

    switch (event.keysym.scancode) {
    case SDL_SCANCODE_ESCAPE:
        if (keymod == KMOD_NONE) {
            sel_ = ~size_t(0);
            finish();
            return true;
        }
        break;
    case SDL_SCANCODE_RETURN:
        if (keymod == KMOD_NONE) {
            finish();
            return true;
        }
        break;
    case SDL_SCANCODE_UP: {
        size_t sel = sel_;
        if (sel != ~size_t(0) && sel > 0)
            sel_ = sel - 1;
        return true;
    }
    case SDL_SCANCODE_DOWN: {
        size_t sel = sel_;
        if (sel != ~size_t(0) && sel + 1 < items_.size())
            sel_ = sel + 1;
        return true;
    }
    case SDL_SCANCODE_PAGEUP: {
        size_t sel = sel_;
        if (sel != ~size_t(0))
            sel_ = std::max<ssize_t>(0, sel - 10);
        return true;
    }
    case SDL_SCANCODE_PAGEDOWN: {
        size_t sel = sel_;
        if (sel != ~size_t(0))
            sel_ = std::min(items_.size() - 1, sel + 10);
        return true;
    }
    case SDL_SCANCODE_HOME: {
        size_t sel = sel_;
        if (sel != ~size_t(0))
            sel_ = 0;
        return true;
    }
    case SDL_SCANCODE_END: {
        size_t sel = sel_;
        if (sel != ~size_t(0))
            sel_ = items_.size() - 1;
        return true;
    }
    default:
        break;
    }

    return false;
}

bool Modal_Selection_Box::handle_key_released(const SDL_KeyboardEvent &event)
{
    if (has_completed())
        return false;

    return false;
}

void Modal_Selection_Box::paint_contents(SDL_Renderer *rr, const Rect &bounds)
{
    const std::vector<std::string> &items = items_;
    size_t sel = sel_;

    if (sel == ~size_t(0))
        return;

    Rect r = bounds;
    const Color_Palette &pal = Color_Palette::get_current();

    Text_Painter tp;
    tp.rr = rr;
    tp.font = &font_s12;

    int fh = tp.font->height();
    size_t item_count = items.size();
    size_t items_shown = std::min<long>(item_count, std::max(0, r.h / fh));

    size_t show_offset = std::max<long>(0, sel - items_shown / 2);
    if (item_count - show_offset < items_shown)
        show_offset = std::max<long>(0, item_count - items_shown);

    for (size_t i = 0; i < items_shown; ++i) {
        size_t index = i + show_offset;

        Rect row = r.take_from_top(fh);
        tp.pos = row.origin();

        tp.fg = pal[Colors::box_foreground];
        if (index == sel) {
            tp.fg = pal[Colors::box_active_item_foreground];
            SDLpp_SetRenderDrawColor(rr, pal[Colors::box_active_item_background]);
            SDL_RenderFillRect(rr, &row);
        }

        SDLpp_ClipState clip;
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &row);
        tp.draw_utf8(items[index]);
        SDLpp_RestoreClipState(rr, clip);
    }
}

///
Modal_File_Selection_Box::Modal_File_Selection_Box(const Rect &bounds, std::string path)
    : Modal_Selection_Box(bounds, std::string{}, std::vector<std::string>{}),
      model_(new File_Browser_Model)
{
    File_Browser_Model &model = *model_;

    model.set_current_path(path);
    update_file_entries();
}

void Modal_File_Selection_Box::update_file_entries()
{
    File_Browser_Model &model = *model_;
    size_t count = model.count();

    if (count == 0)
        sel_ = ~size_t{0};
    else {
        sel_ = model.selection();
        items_.clear();
        for (size_t i = 0; i < count; ++i)
            items_.push_back(model.filename(i));
    }

    title_ = "File: " + get_display_path(model.cwd());
}

bool Modal_File_Selection_Box::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    if (has_completed())
        return false;

    File_Browser_Model &model = *model_;

    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

    switch (event.keysym.scancode) {
    case SDL_SCANCODE_RETURN:
        if (keymod == KMOD_NONE) {
            if (const File_Entry *ent = model.current_entry()) {
                if (ent->type == 'D') {
                    model.trigger_entry(model.selection());
                    update_file_entries();
                }
                else
                    finish();
            }
            return true;
        }
        break;
    case SDL_SCANCODE_BACKSPACE:
        if (keymod == KMOD_NONE) {
            model.trigger_entry(model.find_entry(".."));
            update_file_entries();
            return true;
        }
        break;
    default:
        break;
    }

    bool ret = Modal_Selection_Box::handle_key_pressed(event);
    model.set_selection(sel_);

    return ret;
}
