//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <memory>

class Tray_Icon {
public:
    Tray_Icon();
    ~Tray_Icon();

#if !defined(_WIN32) && !defined(APPLE)
    static void run_gtk_tray(int argc, char *argv[]);
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
