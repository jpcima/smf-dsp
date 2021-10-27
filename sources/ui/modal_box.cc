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
#include "utility/charset.h"
#include "utility/SDL++.h"
#include <utf/utf.hpp>
#include <nonstd/string_view.hpp>
#include <algorithm>

static constexpr int title_top_padding = 4;
static constexpr int title_bottom_padding = 3;
static constexpr int title_left_padding = 3;
static constexpr int content_padding = 2;

Modal_Box::Modal_Box(const Rect &bounds, std::string title)
    : bounds_(bounds), title_(std::move(title)), font_title_(&font_fmdsp_medium)
{
}

void Modal_Box::paint(SDL_Renderer *rr)
{
    const Rect bounds = bounds_;
    const Color_Palette &pal = Color_Palette::get_current();

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_background]);
    SDLpp_RenderFillRect(rr, bounds);

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_frame]);
    SDLpp_RenderDrawRect(rr, bounds);

    Text_Painter tp;
    tp.rr = rr;
    tp.font = font_title_;
    tp.pos = bounds.origin().off_by(Point(title_left_padding, title_top_padding));
    tp.fg = pal[Colors::box_title];
    tp.draw_utf8(title_);

    int texth = tp.font->height();
    int ybottom = bounds.y + title_top_padding + texth + title_bottom_padding;

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_frame]);
    SDLpp_RenderDrawHLine(rr, bounds.x, bounds.right(), ybottom);

    paint_contents(rr);
}

void Modal_Box::finish()
{
    if (!complete_) {
        complete_ = true;
        if (CompletionCallback)
            CompletionCallback();
    }
}

Rect Modal_Box::get_content_bounds() const
{
    int texth = font_title_->height();
    return Rect(bounds_)
        .chop_from_top(title_top_padding + texth + title_bottom_padding)
        .reduced(content_padding);
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
    case SDL_SCANCODE_KP_ENTER:
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

nonstd::any Modal_Selection_Box::get_completion_result(size_t index)
{
    switch (index) {
    case 0:
        return sel_;
    case 1:
        return (sel_ != ~size_t(0)) ? items_[sel_] : std::string{};
    default:
        return nonstd::any{};
    }
}

void Modal_Selection_Box::paint_contents(SDL_Renderer *rr)
{
    const Rect bounds = get_content_bounds();

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
    case SDL_SCANCODE_KP_ENTER:
        if (keymod == KMOD_NONE) {
            if (const File_Entry *ent = model.current_entry()) {
                if (ent->type() == 'D') {
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

///
static constexpr int text_input_horiz_margin = 4;
static constexpr int text_input_vert_margin = 4;

Modal_Text_Input_Box::Modal_Text_Input_Box(const Rect &bounds, std::string title)
    : Modal_Box{bounds, std::move(title)}, font_text_input_(&font_s12)
{
}

bool Modal_Text_Input_Box::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    if (has_completed())
        return false;

    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

    switch (event.keysym.scancode) {
    case SDL_SCANCODE_ESCAPE:
        if (keymod == KMOD_NONE) {
            accepted_ = false;
            finish();
            return true;
        }
        break;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        if (keymod == KMOD_NONE) {
            accepted_ = true;
            finish();
            return true;
        }
        break;
    case SDL_SCANCODE_LEFT:
        if (keymod == KMOD_NONE) {
            if (cursor_ > 0) {
                --cursor_;
                offset_to_left();
            }
            return true;
        }
        break;
    case SDL_SCANCODE_RIGHT:
        if (keymod == KMOD_NONE) {
            if (cursor_ < input_text_.size()) {
                ++cursor_;
                offset_to_right();
            }
            return true;
        }
        break;
    case SDL_SCANCODE_HOME:
        if (keymod == KMOD_NONE) {
            if (cursor_ != 0) {
                cursor_ = 0;
                offset_to_left();
            }
            return true;
        }
        break;
    case SDL_SCANCODE_END:
        if (keymod == KMOD_NONE) {
            if (cursor_ != input_text_.size()) {
                cursor_ = input_text_.size();
                offset_to_right();
            }
            return true;
        }
        break;
    case SDL_SCANCODE_DELETE:
        if (keymod == KMOD_NONE) {
            if (cursor_ < input_text_.size()) {
                input_text_.erase(input_text_.begin() + cursor_);
                input_gsize_.erase(input_gsize_.begin() + cursor_);
            }
            return true;
        }
        break;
    case SDL_SCANCODE_BACKSPACE:
        if (keymod == KMOD_NONE) {
            if (cursor_ > 0) {
                --cursor_;
                input_text_.erase(input_text_.begin() + cursor_);
                input_gsize_.erase(input_gsize_.begin() + cursor_);
                offset_to_left();
            }
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

bool Modal_Text_Input_Box::handle_key_released(const SDL_KeyboardEvent &event)
{
    if (has_completed())
        return false;

    return false;
}

bool Modal_Text_Input_Box::handle_text_input(const SDL_TextInputEvent &event)
{
    if (has_completed())
        return false;

    nonstd::string_view str = event.text;
    uint32_t cursor = cursor_;

    for (const char *cur = str.begin(), *end = str.end(); cur != end;) {
        char32_t ch = utf::utf_traits<char>::decode(cur, end);
        if (ch == utf::incomplete) break;
        if (ch == utf::illegal) continue;
        input_text_.insert(input_text_.begin() + cursor, ch);
        const Font_Glyph *g = font_text_input_->glyph(ch);
        input_gsize_.insert(input_gsize_.begin() + cursor, g ? g->size : 1);
        ++cursor;
    }

    cursor_ = cursor;
    offset_to_right();

    return true;
}


nonstd::any Modal_Text_Input_Box::get_completion_result(size_t index)
{
    switch (index) {
    case 0:
        return accepted_;
    case 1: {
        std::string utf8;
        convert_utf(nonstd::u32string_view(input_text_), utf8, true);
        return std::move(utf8);
    }
    default:
        return nonstd::any{};
    }
}

void Modal_Text_Input_Box::paint_contents(SDL_Renderer *rr)
{
    unsigned maxchars = get_max_display_chars();
    if (maxchars == 0)
        return;

    const Rect bounds = get_content_bounds();
    const Color_Palette &pal = Color_Palette::get_current();

    Text_Painter tp;
    tp.rr = rr;
    tp.font = font_text_input_;

    Rect frame;
    frame.w = maxchars * tp.font->width() + 2 * text_input_horiz_margin;
    frame.h = tp.font->height() + 2 * text_input_vert_margin;

    frame.x = bounds.x + (bounds.w - frame.w) / 2;
    frame.y = bounds.y + (bounds.h - frame.h) / 2;

    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_frame]);
    SDL_RenderDrawRect(rr, &frame);

    Rect textbounds = frame.reduced({4, 4});
    tp.pos = textbounds.origin();

    ///
    const std::u32string &text = this->input_text_;
    size_t glyphs_drawn = 0;
    const uint32_t cursor = cursor_;

    for (size_t i = display_offset_, n = text.size();; ++i) {
        char32_t ch = (i < n) ? text[i] : U'_';
        const Font_Glyph *g = tp.font->glyph(ch);
        unsigned gsize = g ? g->size : 1;
        if (glyphs_drawn + gsize > maxchars)
            break;
        tp.fg = pal[Colors::box_foreground];
        if (i >= n)
            tp.fg = pal[Colors::box_foreground_secondary];
        if (i == cursor) {
            Rect cr{tp.pos.x, tp.pos.y, (int)(gsize * tp.font->width()), tp.font->height()};
            SDLpp_SetRenderDrawColor(rr, pal[Colors::box_foreground]);
            SDL_RenderFillRect(rr, &cr);
            tp.fg = pal[Colors::box_background];
        }
        tp.draw_char(ch);
        glyphs_drawn += gsize;
    }

    ///
    Rect labelbounds = textbounds
        .off_by({0, -2 * text_input_vert_margin - tp.font->height()});
    tp.font = &font_fmdsp_medium;
    tp.pos = labelbounds.origin();
    tp.fg = pal[Colors::box_foreground];
    nonstd::string_view label = label_;
    if (label.empty())
        label = "Please enter a value:";
    tp.draw_utf8(label);
}

unsigned Modal_Text_Input_Box::get_max_display_chars() const
{
    const Rect bounds = get_content_bounds();
    int charw = font_text_input_->width();
    int maxchars = (bounds.w - 16 - 2 * text_input_horiz_margin) / charw;
    return (maxchars > 0) ? maxchars : 0;
}

size_t Modal_Text_Input_Box::char_display_size(size_t index) const
{
    return (index < input_text_.size()) ? input_gsize_[index] : 1;
}

void Modal_Text_Input_Box::offset_to_left()
{
    display_offset_ = std::min(display_offset_, cursor_);
}

void Modal_Text_Input_Box::offset_to_right()
{
    size_t c_cur = cursor_;
    size_t c_off = display_offset_;

    const size_t d_max = get_max_display_chars();
    size_t d_size = 0;
    for (size_t i = c_off; i < c_cur; ++i)
        d_size += char_display_size(i);

    while (d_size >= d_max && c_off < c_cur) {
        d_size -= char_display_size(c_off);
        ++c_off;
    }

    display_offset_ = c_off;
}
