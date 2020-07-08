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
#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#if defined(_WIN32)
static std::string known_folder_path(int csidl, std::error_code &ec)
{
    ec.clear();

    WCHAR wpath[PATH_MAX + 1];
    HRESULT result = SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, wpath);

    if (result != S_OK) {
        ec = std::error_code(result, std::system_category());
        return std::string();
    }

    wpath[PATH_MAX] = L'\0';

    std::string path;
    convert_utf<WCHAR, char>(wpath, path, true);
    return normalize_path_separators(path + '/');
}

static std::string known_folder_path(int csidl)
{
    std::error_code ec;
    std::string path = known_folder_path(csidl, ec);
    if (ec)
        throw std::system_error(ec);
    return path;
}
#endif

const std::string &get_configuration_dir()
{
    static const std::string path = []() -> std::string {
        std::string path;
#if defined(_WIN32)
        path = known_folder_path(CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE);
        path.append(PROGRAM_DISPLAY_NAME "/");
        mkdir(path.c_str(), 0755);
#elif defined(__HAIKU__)
        path = get_home_directory();
        if (path.empty())
            return std::string();
        path.append("config/");
        mkdir(path.c_str(), 0755);
        path.append("settings/");
        mkdir(path.c_str(), 0755);
        path.append(PROGRAM_DISPLAY_NAME "/");
        mkdir(path.c_str(), 0755);
#else
        path = get_home_directory();
        if (path.empty())
            return std::string();
        path.append(".config/");
        mkdir(path.c_str(), 0755);
        path.append(PROGRAM_DISPLAY_NAME "/");
        mkdir(path.c_str(), 0755);
#endif
        path.shrink_to_fit();
        return path;
    }();
    return path;
}

std::string get_configuration_file(gsl::cstring_span name)
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
    return load_configuration("config");
}

bool save_global_configuration(const CSimpleIniA &ini)
{
    return save_configuration("config", ini);
}
