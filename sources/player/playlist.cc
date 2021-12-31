//          Copyright Jean Pierre Cimalando 2018-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "playlist.h"
#include "utility/file_scan.h"
#include <memory>

//
void Linear_Play_List::add_file(const std::string &path)
{
    files_.emplace_back(path);
}

void Linear_Play_List::start()
{
    index_ = 0;
}

bool Linear_Play_List::at_end() const
{
    return index_ == files_.size();
}

std::string Linear_Play_List::current() const
{
    return files_[index_];
}

bool Linear_Play_List::go_next()
{
    if (index_ == files_.size())
        return false;
    ++index_;
    return true;
}

bool Linear_Play_List::go_previous()
{
    if (index_ == 0)
        return false;
    --index_;
    return true;
}

//
Random_Play_List::Random_Play_List(
    const std::string &root_path,
    std::function<bool (const std::string &)> file_filter)
    : file_scan_(new File_Scan(root_path, std::move(file_filter))),
      prng_(std::random_device{}())
{
}

Random_Play_List::~Random_Play_List()
{
}

void Random_Play_List::start()
{
    File_Scan &fs = *file_scan_;
    fs.wait_for_file_count(100, std::chrono::milliseconds(500));

    index_ = 0;
    history_.clear();

    if (!fs.files_empty())
        history_.push_back(random_file());
}

bool Random_Play_List::at_end() const
{
    return history_.empty();
}

std::string Random_Play_List::current() const
{
    File_Scan &fs = *file_scan_;
    return fs.file_name(history_[index_]);
}

bool Random_Play_List::go_next()
{
    File_Scan &fs = *file_scan_;
    if (index_ < history_.size() - 1)
        ++index_;
    else {
        if (history_.size() == history_max)
            history_.pop_front();
        else if (fs.files_empty())
            return false;
        history_.push_back(random_file());
        index_ = history_.size() - 1;
    }
    return true;
}

bool Random_Play_List::go_previous()
{
    if (index_ == 0)
        return false;
    --index_;
    return true;
}

size_t Random_Play_List::random_file() const
{
    File_Scan &fs = *file_scan_;
    std::uniform_int_distribution<size_t> dist(0, fs.file_count() - 1);
    return dist(prng_);
}
