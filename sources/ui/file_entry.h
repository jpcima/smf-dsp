//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>

class File_Entry {
public:
    File_Entry() noexcept {}
    File_Entry(char type, std::string name) noexcept : type_(type), name_(std::move(name)) {}

    char type() const noexcept { return type_; }
    const std::string &name() const noexcept { return name_; }

private:
    char type_;
    std::string name_;
};

bool operator==(const File_Entry &a, const File_Entry &b);
bool operator<(const File_Entry &a, const File_Entry &b);
