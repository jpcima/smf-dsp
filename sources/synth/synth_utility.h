//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <memory>

struct string_list_delete { void operator()(char **p) const; };
typedef std::unique_ptr<char *[], string_list_delete> string_list_ptr;

string_list_ptr string_list_dup(const char *const *list);
