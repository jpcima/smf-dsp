//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "file_entry.h"
#include <nonstd/span.hpp>
#include <nonstd/string_view.hpp>
#include <vector>
#include <string>
#include <functional>

class File_Browser_Model
{
public:
    File_Browser_Model();
    explicit File_Browser_Model(nonstd::string_view cwd);

    std::string current_filename() const;
    void set_current_filename(nonstd::string_view file);
    std::string filename(size_t index) const;

    std::string current_path() const;
    void set_current_path(nonstd::string_view path);

    const std::string &cwd() const noexcept { return cwd_; }
    void set_cwd(nonstd::string_view dir);

    size_t selection() const noexcept { return sel_; }
    void set_selection(size_t sel);

    size_t count() const noexcept { return entries_.size(); }
    const File_Entry &entry(size_t index) const noexcept { return entries_[index]; }
    const File_Entry *current_entry() const noexcept;
    size_t find_entry(nonstd::string_view name) const;
    nonstd::span<const File_Entry> all_entries() const noexcept { return entries_; }

    void refresh();

    void trigger_entry(size_t index);
    void trigger_selected_entry();

    std::function<void (const std::string &, const File_Entry *, size_t, size_t)> FileOpenCallback;
    std::function<bool (const File_Entry &)> FileFilterCallback;

private:
    size_t sel_ = 0;
    std::string cwd_;
    std::vector<File_Entry> entries_;
};
