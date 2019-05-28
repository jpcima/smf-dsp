//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "config.h"
#include "utility/paths.h"
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

std::string get_configuration_dir()
{
#if defined(_WIN32)
    std::string path = get_executable_path();
    while (!path.empty() && path.back() != '/') path.pop_back();
    path.append("config/");
    mkdir(path.c_str(), 0755);
#elif defined(__HAIKU__)
    std::string path = get_home_directory();
    if (path.empty())
        return std::string();
    path.append("config/");
    mkdir(path.c_str(), 0755);
    path.append("settings/");
    mkdir(path.c_str(), 0755);
    path.append(PROGRAM_DISPLAY_NAME "/");
    mkdir(path.c_str(), 0755);
#else
    std::string path = get_home_directory();
    if (path.empty())
        return std::string();
    path.append(".config/");
    mkdir(path.c_str(), 0755);
    path.append(PROGRAM_DISPLAY_NAME "/");
    mkdir(path.c_str(), 0755);
#endif
    return path;
}

std::string get_configuration_file(gsl::cstring_span name)
{
    std::string path = get_configuration_dir();
    path.append(name.data(), name.size());
    path.append(".ini");
    return path;
}

std::unique_ptr<CSimpleIniA> create_configuration()
{
    bool iniIsUtf8 = true, iniMultiKey = true, iniMultiLine = true;
    return std::unique_ptr<CSimpleIniA>(
        new CSimpleIniA(iniIsUtf8, iniMultiKey, iniMultiLine));
}

std::unique_ptr<CSimpleIniA> load_configuration(gsl::cstring_span name)
{
    std::unique_ptr<CSimpleIniA> ini = create_configuration();
    if (ini->LoadFile(get_configuration_file(name).c_str()) != SI_OK)
        return nullptr;

    return ini;
}

bool save_configuration(gsl::cstring_span name, const CSimpleIniA &ini)
{
    return ini.SaveFile(get_configuration_file(name).c_str()) == SI_OK;
}

std::unique_ptr<CSimpleIniA> load_global_configuration()
{
    return load_configuration(PROGRAM_DISPLAY_NAME);
}

bool save_global_configuration(const CSimpleIniA &ini)
{
    return save_configuration(PROGRAM_DISPLAY_NAME, ini);
}
