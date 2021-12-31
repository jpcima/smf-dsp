//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <memory>

typedef void *Dl_Handle;

Dl_Handle Dl_open(const char *name);
void Dl_close(Dl_Handle handle);
void *Dl_sym(Dl_Handle handle, const char *name);

struct Dl_Handle_Deleter {
    void operator()(Dl_Handle x) const noexcept {
        Dl_close(x);
    }
};

typedef std::unique_ptr<
    std::remove_pointer<Dl_Handle>::type, Dl_Handle_Deleter> Dl_Handle_U;
