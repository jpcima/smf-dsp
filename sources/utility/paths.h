//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <gsl.hpp>
#include <string>

std::string get_home_directory();
std::string get_current_directory();
std::string get_executable_path();
bool is_path_absolute(gsl::cstring_span path);
bool is_path_separator(char32_t character);
void append_path_separator(std::string &path);
std::string normalize_path_separators(gsl::cstring_span path);
std::string make_path_canonical(gsl::cstring_span path);
std::string expand_path_tilde(gsl::cstring_span path);
gsl::cstring_span path_file_name(gsl::cstring_span path);
gsl::cstring_span path_directory(gsl::cstring_span path);

std::string get_display_path(gsl::cstring_span path);
