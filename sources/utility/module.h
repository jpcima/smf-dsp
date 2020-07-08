//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#if defined(_WIN32)
#include <windows.h>
#endif

std::string get_executable_path();

#if defined(_WIN32)
void initialize_app_module(HINSTANCE instance);
HINSTANCE get_app_module();
#endif
