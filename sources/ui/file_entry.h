//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>

struct File_Entry { char type; std::string name; };

bool operator==(const File_Entry &a, const File_Entry &b);
bool operator<(const File_Entry &a, const File_Entry &b);
