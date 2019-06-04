//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ui/file_browser.h"
#include "ui/file_entry.h"
#include "ui/color_palette.h"
#include "ui/fonts.h"
#include "ui/text.h"
#include "ui/paint.h"
#include "utility/charset.h"
#include "utility/SDL++.h"
#include <gsl.hpp>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
static const char initwd[] = "/";
#else
static const char initwd[] = "C:/";
#endif

static constexpr unsigned fn_name_chars = 24;
static constexpr unsigned fn_max_chars = fn_name_chars + 2;

File_Browser::File_Browser(const Rect &bounds)
    : bounds_(bounds),
      cwd_(initwd)
{
    set_font(&font_s14);
    entries_.reserve(256);
    refresh_file_list();
    focus_on_selection();
}

void File_Browser::set_current_file(const std::string &path)
{
    size_t pos = path.rfind('/');
    if (pos == path.npos)
        return;
    gsl::cstring_span filename = gsl::cstring_span(path).subspan(pos + 1);

    cwd_ = path.substr(0, pos + 1);
    sel_ = 0;
    refresh_file_list();

    size_t sel = ~size_t(0);
    for (size_t i = 0, n = entries_.size(); i < n && sel == ~size_t(0); ++i) {
        const File_Entry &cur = entries_[i];
        if (cur.name == filename)
            sel = i;
    }

    if (sel != ~size_t(0))
        sel_ = sel;
    focus_on_selection();
}

void File_Browser::set_font(Font *font)
{
    font_ = font;
    focus_on_selection();
}

void File_Browser::set_cwd(const std::string &dir)
{
    cwd_ = dir;
    sel_ = 0;
    refresh_file_list();
    focus_on_selection();
}

void File_Browser::paint(SDL_Renderer *rr)
{
    Rect bounds = bounds_;

    size_t rows = this->rows();
    size_t cols = this->cols();
    size_t iw = item_width();
    size_t ih = item_height();

    const File_Entry *ents = entries_.data();
    size_t nent = entries_.size();

    Text_Painter tp;
    tp.rr = rr;
    tp.font = font_;
    int fw = tp.font->width();

    size_t colindex = colindex_;

    for (size_t i = 0, n = rows * cols; i < n; ++i) {
        size_t entno = i + rows * colindex;
        if (entno >= nent)
            break;

        const std::string &file_name = ents[entno].name;

        std::string name_drawn;
        if (ents[entno].type == 'D')
            name_drawn = '/' + file_name;
        else
            name_drawn = file_name;

        long padchars = ((long)(fn_name_chars * fw) - (long)tp.measure_utf8(name_drawn)) / fw;
        if (padchars > 0)
            name_drawn.insert(name_drawn.end(), padchars, ' ');

        size_t r = i % rows;
        size_t c = i / rows;

        Rect ib(bounds.x + c * iw, bounds.y + r * ih, fn_name_chars * fw, ih);
        tp.pos.x = ib.x;
        tp.pos.y = ib.y;

        tp.fg = Color_Palette::text_browser_foreground;
        if (entno == sel_) {
            tp.fg = Color_Palette::info_box_background;
            SDLpp_SetRenderDrawColor(rr, Color_Palette::text_browser_foreground);
            SDL_RenderFillRect(rr, &ib);
        }

        SDLpp_ClipState clip;
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &ib);
        tp.draw_utf8(name_drawn);
        SDLpp_RestoreClipState(rr, clip);
    }
}

bool File_Browser::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);
    if (keymod == KMOD_NONE) {
        switch (event.keysym.scancode) {
        case SDL_SCANCODE_UP:
            move_selection_by(-1);
            return true;
        case SDL_SCANCODE_DOWN:
            move_selection_by(+1);
            return true;
        case SDL_SCANCODE_LEFT:
            move_selection_by(-rows());
            return true;
        case SDL_SCANCODE_RIGHT:
            move_selection_by(+rows());
            return true;
        case SDL_SCANCODE_RETURN:
            trigger_entry(entries_[sel_]);
            return true;
        case SDL_SCANCODE_BACKSPACE:
            trigger_entry(entries_[0]);
            return true;
        default:
            break;
        }
    }

    return false;
}

bool File_Browser::handle_key_released(const SDL_KeyboardEvent &event)
{
    return false;
}

void File_Browser::refresh_file_list()
{
    std::vector<File_Entry> &entries = entries_;

    File_Entry old_sel;
    bool have_old_sel = sel_ < entries_.size();
    if (have_old_sel)
        old_sel = std::move(entries_[sel_]);

    entries.clear();
    entries.push_back(File_Entry{'D', ".."});

    const std::string &cwd = cwd_;

#ifdef _WIN32
    if (cwd == "/") {
        uint32_t drives = GetLogicalDrives();
        for (unsigned i = 0; i < 26; ++i) {
            if (drives & (1 << i)) {
                char name[] = {char('A' + i), ':'};
                entries.push_back(File_Entry{'D', std::string(name, sizeof(name))});
            }
        }
    }
    else
#endif
    {
        Dir dir(cwd);
        if (!dir)
            return;

        for (std::string name; dir.read_next(name);) {
            if (name.empty() || name[0] == '.')
                continue;

            unsigned mode = filemode_utf8((cwd + '/' + name).c_str());
            if ((int)mode == -1)
                continue;

            char type = S_ISDIR(mode) ? 'D' : 'F';
            File_Entry entry{type, name};

            if (!FileFilterCallback || FileFilterCallback(entry))
                entries.push_back(std::move(entry));
        }
    }

    std::sort(entries.begin(), entries.end());

    if (!have_old_sel)
        sel_ = 0;
    else {
        auto it = std::find(entries.begin(), entries.end(), old_sel);
        sel_ = (it == entries.end()) ? 0 : std::distance(entries.begin(), it);
    }
}

void File_Browser::move_selection_by(long offset)
{
    size_t sel = (long)sel_ + offset;
    if (sel < entries_.size()) {
        sel_ = sel;
        focus_on_selection();
    }
}

void File_Browser::focus_on_selection()
{
    size_t sel = sel_;
    size_t count = entries_.size();
    size_t rows = this->rows();
    size_t cols = this->cols();
    size_t totalcols = (count + rows - 1) / rows;
    size_t selcol = sel / rows;
    size_t maxcolindex = std::max((long)totalcols - (long)cols, 0l);
    colindex_ = std::min(selcol - std::min(cols / 2, selcol), maxcolindex);
}

void File_Browser::trigger_entry(const File_Entry &ent)
{
    std::string path = cwd_;
    std::string curfile;

#ifdef _WIN32
    if (ent.name == ".." && path.size() == 3 && path[1] == ':' && path[2] == '/') {
        char name[] = {path[0], path[1]};
        curfile.assign(name, sizeof(name));
        path = "/";
    }
    else if (path == "/") {
        if (ent.name == "..")
            path.clear();
        else {
            char name[] = {ent.name[0], ':', '/'};
            path.assign(name, sizeof(name));
        }
    }
    else
#endif
    if (ent.name == "..") {
        path.pop_back(); // remove last '/'

        size_t pos = path.rfind('/');
        if (pos == path.npos)
            path.clear();
        else {
            curfile = path.substr(pos + 1);
            path = path.substr(0, pos + 1);
        }
    }
    else if (ent.type == 'D') {
        path.append(ent.name);
        path.push_back('/');
    }
    else if (FileOpenCallback) {
        FileOpenCallback(path, &entries_[0], &ent - &entries_[0], entries_.size());
    }

    if (!path.empty() && ent.type == 'D') {
        cwd_ = path;
        sel_ = 0;
        refresh_file_list();
        if (!curfile.empty()) {
            std::vector<File_Entry> &entries = entries_;
            auto it = std::find_if(
                entries.begin(), entries.end(),
                [&curfile](const File_Entry &x) -> bool { return x.type == 'D' && x.name == curfile; });
            if (it != entries.end())
                sel_ = std::distance(entries.begin(), it);
        }
        focus_on_selection();
    }
}

size_t File_Browser::item_width() const
{
    return fn_max_chars * font_->width();
}

size_t File_Browser::item_height() const
{
    return font_->height();
}

size_t File_Browser::rows() const
{
    return std::max<size_t>(1, bounds_.h / item_height());
}

size_t File_Browser::cols() const
{
    return std::max<size_t>(1, bounds_.w / item_width());
}
