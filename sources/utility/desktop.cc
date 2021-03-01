//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "utility/desktop.h"
#include "utility/logs.h"
#include <gsl/gsl>
#if defined(_WIN32)
#include "utility/charset.h"
#include <windows.h>
#elif defined(__APPLE__)
#else
#include <glib.h>
#include <gio/gio.h>
#endif

#if defined(_WIN32)
bool open_file_externally(const std::string &path)
{
    std::wstring wpath;
    convert_utf<char, wchar_t>(path, wpath, true);

    ShellExecuteW(nullptr, nullptr, wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    return true;
}
#elif defined(__APPLE__)
bool open_file_externally(const std::string &path)
{
    #pragma message("TODO: implement open_file_externally")
}
#else
bool open_file_externally(const std::string &path)
{
    char *uri = g_filename_to_uri(path.c_str(), nullptr, nullptr);
    if (!uri)
        return false;
    auto cleanup = gsl::finally([uri]() { g_free(uri); });
    return g_app_info_launch_default_for_uri(uri, nullptr, nullptr);
}
#endif
