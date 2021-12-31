//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "synth_utility.h"
#include <cstring>

void string_list_delete::operator()(char **p) const
{
    for (char **q = p; *q; ++q)
        delete[] *q;
    delete[] p;
}

string_list_ptr string_list_dup(const char *const *list)
{
    size_t n = 0;
    while (list[n]) ++n;
    string_list_ptr copy(new char *[n + 1]());
    for (size_t i = 0; i < n; ++i) {
        size_t len = strlen(list[i]);
        copy[i] = new char[len + 1];
        memcpy(copy[i], list[i], len + 1);
    }
    copy[n] = nullptr;
    return copy;
}
