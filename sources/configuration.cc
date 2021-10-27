//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "configuration.h"
#include "utility/paths.h"
#include "utility/charset.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <cerrno>
#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

const std::string &get_configuration_dir()
{
    static const std::string path = []() -> std::string {
        std::string path;
#if defined(_WIN32)
        path = known_folder_path(CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE);
        path.append(PROGRAM_DISPLAY_NAME "/");
        make_directory(path);
#elif defined(__HAIKU__)
        path = get_home_directory();
        if (path.empty())
            return std::string();
        path.append("config/");
        make_directory(path);
        path.append("settings/");
        make_directory(path);
        path.append(PROGRAM_DISPLAY_NAME "/");
        make_directory(path);
#else
        path = get_home_directory();
        if (path.empty())
            return std::string();
        path.append(".config/");
        make_directory(path);
        path.append(PROGRAM_DISPLAY_NAME "/");
        make_directory(path);
#endif
        path.shrink_to_fit();
        return path;
    }();
    return path;
}

std::string get_configuration_file(nonstd::string_view name)
{
    std::string path = get_configuration_dir();
    if (path.empty())
        return std::string{};
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

std::unique_ptr<CSimpleIniA> load_configuration(nonstd::string_view name)
{
    std::unique_ptr<CSimpleIniA> ini = create_configuration();
    if (ini->LoadFile(get_configuration_file(name).c_str()) != SI_OK)
        return nullptr;

    return ini;
}

bool save_configuration(nonstd::string_view name, const CSimpleIniA &ini)
{
    return ini.SaveFile(get_configuration_file(name).c_str()) == SI_OK;
}

std::unique_ptr<CSimpleIniA> load_global_configuration()
{
    return load_configuration("config");
}

bool save_global_configuration(const CSimpleIniA &ini)
{
    return save_configuration("config", ini);
}
