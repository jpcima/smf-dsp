//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "load_library.h"

class Icuuc_Lib {
protected:
    Icuuc_Lib();

public:
    static Icuuc_Lib &instance();
    void *find_symbol(const char *name);

private:
    Dl_Handle_U handle_;
    unsigned version_ = 0;

private:
    static unsigned extract_version(const char *libname, Dl_Handle handle);
};
