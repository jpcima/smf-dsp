//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <SimpleIni.h>
#include <nonstd/string_view.hpp>
#include <string>
#include <memory>

const std::string &get_configuration_dir();
std::string get_configuration_file(nonstd::string_view name);

std::unique_ptr<CSimpleIniA> create_configuration();
std::unique_ptr<CSimpleIniA> load_configuration(nonstd::string_view name);
bool save_configuration(nonstd::string_view name, const CSimpleIniA &ini);

std::unique_ptr<CSimpleIniA> load_global_configuration();
bool save_global_configuration(const CSimpleIniA &ini);
