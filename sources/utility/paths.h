//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <nonstd/string_view.hpp>
#include <string>

std::string get_home_directory();
std::string get_current_directory();
bool is_path_absolute(nonstd::string_view path);
bool is_path_separator(char32_t character);
void append_path_separator(std::string &path);
std::string normalize_path_separators(nonstd::string_view path);
std::string make_path_canonical(nonstd::string_view path);
std::string expand_path_tilde(nonstd::string_view path);
nonstd::string_view path_file_name(nonstd::string_view path);
nonstd::string_view path_directory(nonstd::string_view path);

std::string get_display_path(nonstd::string_view path);

#if defined(_WIN32)
std::string known_folder_path(int csidl, std::error_code &ec);
std::string known_folder_path(int csidl);
#endif
