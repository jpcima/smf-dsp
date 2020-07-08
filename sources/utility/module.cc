//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "module.h"
#include "paths.h"
#include "charset.h"
#include <memory>
#include <stdexcept>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__HAIKU__)
#include <KernelKit.h>
#endif
#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(__HAIKU__)
static std::string get_native_executable_path()
{
    image_info info;
    int32 cookie = 0;
    if (get_next_image_info(0, &cookie, &info) != B_OK)
        throw std::runtime_error("get_next_image_info");
    return info.name;
}
#elif defined(_WIN32)
static std::string get_native_executable_path()
{
    size_t bufsize = 0x100;
    std::unique_ptr<wchar_t[]> buf;
    bool need_more_buffer;
    do {
        buf.reset(new wchar_t[bufsize]);
        //HMODULE mod = GetModuleHandle(nullptr);
        HMODULE mod = get_app_module();
        DWORD ret = GetModuleFileNameW(mod, buf.get(), bufsize);
        if (ret == 0)
            throw std::runtime_error("GetModuleFileNameW");
        need_more_buffer = (ret == bufsize);
        bufsize <<= 1;
    } while (need_more_buffer);

    std::string path;
    if (!convert_utf<wchar_t, char>(buf.get(), path, false))
        throw std::runtime_error("convert_utf");
    return path;
}
#elif defined(__APPLE__)
static std::string get_native_executable_path()
{
    uint32_t bufsize = 0x100;
    std::unique_ptr<char[]> buf;
    bool need_more_buffer;
    do {
        buf.reset(new char[bufsize]);
        need_more_buffer = (_NSGetExecutablePath(buf.get(), &bufsize) == -1);
        bufsize <<= 1;
    } while (need_more_buffer);
    return buf.get();
}
#else
static std::string get_native_executable_path()
{
    size_t bufsize = 0x100;
    std::unique_ptr<char[]> buf;
    bool need_more_buffer;
    do {
        buf.reset(new char[bufsize]);
        size_t count = readlink("/proc/self/exe", buf.get(), bufsize);
        if ((ssize_t)count == -1)
            throw std::runtime_error("readlink");
        need_more_buffer = (count == bufsize);
        if (!need_more_buffer)
            buf[count] = '\0';
        bufsize <<= 1;
    } while (need_more_buffer);
    return buf.get();
}
#endif

std::string get_executable_path()
{
    return normalize_path_separators(get_native_executable_path());
}

#if defined(_WIN32)
static HINSTANCE module_instance = nullptr;

void initialize_app_module(HINSTANCE instance)
{
    module_instance = instance;
}

HINSTANCE get_app_module()
{
    return module_instance;
}
#endif
