//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "file_browser_model.h"
#include "utility/charset.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
static const char initwd[] = "/";
#else
static const char initwd[] = "C:/";
#endif

File_Browser_Model::File_Browser_Model()
    : File_Browser_Model(initwd)
{
}

File_Browser_Model::File_Browser_Model(gsl::cstring_span cwd)
    : cwd_(gsl::to_string(cwd))
{
    entries_.reserve(256);
    refresh();
}

std::string File_Browser_Model::current_file() const
{
    const File_Entry *ent = current_entry();
    if (!ent)
        return std::string{};

    return (ent->type != 'D') ? ent->name : (ent->name + '/');
}

void File_Browser_Model::set_current_file(gsl::cstring_span file)
{
    const std::vector<File_Entry> &entries = entries_;
    size_t index = ~size_t{0};

    for (size_t i = 0, n = entries.size(); i < n && index == ~size_t{0}; ++i) {
        if (entries[i].name == file)
            index = i;
    }

    if (index != ~size_t{0})
        sel_ = index;
}

std::string File_Browser_Model::current_path() const
{
    return cwd_ + current_file();
}

void File_Browser_Model::set_current_path(gsl::cstring_span path)
{
    auto it = std::find(path.rbegin(), path.rend(), '/').base();
    if (it == path.end())
        return;
    size_t pos = std::distance(path.begin(), it);
    gsl::cstring_span filename = gsl::cstring_span(path).subspan(pos + 1);

    cwd_ = gsl::to_string(path.subspan(0, pos + 1));
    sel_ = 0;
    refresh();

    size_t sel = ~size_t{0};
    for (size_t i = 0, n = entries_.size(); i < n && sel == ~size_t{0}; ++i) {
        const File_Entry &cur = entries_[i];
        if (cur.name == filename)
            sel = i;
    }

    if (sel != ~size_t{0})
        sel_ = sel;
}

void File_Browser_Model::set_cwd(gsl::cstring_span dir)
{
    assert(!dir.empty() && dir[dir.size() - 1] == '/');

    cwd_ = gsl::to_string(dir);
    sel_ = 0;
    refresh();
}

void File_Browser_Model::set_selection(size_t sel)
{
    if (sel >= entries_.size())
        return;

    sel_ = sel;
}

size_t File_Browser_Model::find_entry(gsl::cstring_span name) const
{
    const std::vector<File_Entry> &entries = entries_;
    for (size_t i = 0, n = entries.size(); i < n; ++i) {
        if (name == entries[i].name)
            return i;
    }
    return ~size_t{0};
}

const File_Entry *File_Browser_Model::current_entry() const noexcept
{
    size_t sel = sel_;
    if (sel >= entries_.size())
        return nullptr;

    return &entries_[sel];
}

void File_Browser_Model::refresh()
{
    std::vector<File_Entry> &entries = entries_;

    File_Entry old_sel;
    bool have_old_sel = sel_ < entries.size();
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

void File_Browser_Model::trigger_entry(size_t index)
{
    if (index >= entries_.size())
        return;

    const File_Entry &ent = entries_[index];
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
        refresh();
        if (!curfile.empty()) {
            std::vector<File_Entry> &entries = entries_;
            auto it = std::find_if(
                entries.begin(), entries.end(),
                [&curfile](const File_Entry &x) -> bool { return x.type == 'D' && x.name == curfile; });
            if (it != entries.end())
                sel_ = std::distance(entries.begin(), it);
        }
    }
}

void File_Browser_Model::trigger_selected_entry()
{
    return trigger_entry(selection());
}
