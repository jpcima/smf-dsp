#pragma once
#include <SimpleIni.h>
#include <gsl/gsl>
#include <string>
#include <memory>

std::string get_configuration_dir();
std::string get_configuration_file(gsl::cstring_span name);

std::unique_ptr<CSimpleIniA> create_configuration();
std::unique_ptr<CSimpleIniA> load_configuration(gsl::cstring_span name);
bool save_configuration(gsl::cstring_span name, const CSimpleIniA &ini);

std::unique_ptr<CSimpleIniA> load_global_configuration();
bool save_global_configuration(const CSimpleIniA &ini);
