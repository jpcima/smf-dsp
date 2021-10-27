//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "file_scan.h"
#include "paths.h"
#include <nonstd/scope.hpp>

File_Scan::File_Scan(
    std::string path,
    std::function<bool (const std::string &)> file_filter)
    : path_(std::move(path)), file_filter_(std::move(file_filter))
{
    thread_ = std::thread([this] { scan_in_thread(); });
}

File_Scan::~File_Scan()
{
    cancel_.store(true);
    thread_.join();
}

size_t File_Scan::file_count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return file_count_;
}

std::string File_Scan::file_name(size_t index) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    const Entry &file = files_[index];
    const std::vector<Entry> &dirs = dirs_;
    std::list<const std::string *> path;

    size_t parent_index = file.parent;
    while (parent_index != ~size_t(0)) {
        const Entry &dir = dirs[parent_index];
        parent_index = dir.parent;
        path.push_front(&dir.name);
    }

    std::string full;
    auto it = path.begin(), end = path.end();
    if (it != end) {
        full.append(**it++);
        while (it != end) {
            append_path_separator(full);
            full.append(**it++);
        }
    }
    append_path_separator(full);
    full.append(file.name);
    return full;
}

bool File_Scan::is_complete() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<volatile const bool &>(complete_);
}

void File_Scan::wait_for_completion()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!static_cast<volatile bool &>(complete_))
        cond_.wait(lock);
}

bool File_Scan::wait_for_file_count(size_t count, std::chrono::milliseconds timeout)
{
    auto predicate =
        [this, count]() -> bool {
            return static_cast<volatile size_t &>(file_count_) >= count ||
                static_cast<volatile bool &>(complete_);
        };
    std::unique_lock<std::mutex> lock(mutex_);
    return cond_.wait_for(lock, timeout, predicate);
}

void File_Scan::scan_in_thread()
{
#if PORTFTS_HAVE_FTS
    int fts_flags = pFTS_LOGICAL;
#else
    int fts_flags = pFTS_PHYSICAL;
#endif

    pFTS *fts = portfts_open(path_.c_str(), fts_flags);
    if (!fts)
        return;
    auto fts_cleanup = nonstd::make_scope_exit([fts] { portfts_close(fts); });

    std::vector<Entry> &files = files_;
    std::vector<Entry> &dirs = dirs_;

    pFTSENT *ent = portfts_read(fts);
    if (!ent)
        return;
    if (ent->fts_info == pFTS_F) {
        std::lock_guard<std::mutex> lock(mutex_);
        files.emplace_back(~size_t(0), path_);
        file_count_ = files.size();
        return;
    }
    else if (ent->fts_info == pFTS_D) {
        std::lock_guard<std::mutex> lock(mutex_);
        dirs.emplace_back(~size_t(0), ent->fts_path);
    }
    else
        return;

    std::list<size_t> dir_stack = {0};
    while (pFTSENT *ent = portfts_read(fts)) {
        if (cancel_.load())
            break;
        if (ent->fts_level < 0)
            continue;
        dir_stack.resize(ent->fts_level);
        if (ent->fts_info == pFTS_D) {
            std::lock_guard<std::mutex> lock(mutex_);
            dirs.emplace_back(dir_stack.back(), ent->fts_name);
            dir_stack.push_back(dirs.size() - 1);
        }
        else if (ent->fts_info == pFTS_F) {
            std::string name = ent->fts_name;
            if (!file_filter_ || file_filter_(name)) {
                std::lock_guard<std::mutex> lock(mutex_);
                files.emplace_back(dir_stack.back(), std::move(name));
                file_count_ = files.size();
                cond_.notify_one();
            }
        }
    }

    std::unique_lock<std::mutex> lock(mutex_);
    complete_ = true;
    cond_.notify_one();
}
