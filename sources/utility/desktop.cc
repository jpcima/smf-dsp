//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "utility/desktop.h"
#include "utility/logs.h"
#if defined(_WIN32)
#include "utility/charset.h"
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <system_error>
#include <cerrno>
extern "C" { extern char **environ; }
#endif

#if defined(_WIN32)
bool open_file_externally(const std::string &path)
{
    std::wstring wpath;
    convert_utf<char, wchar_t>(path, wpath, true);

    ShellExecuteW(nullptr, nullptr, wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    return true;
}
#else
bool open_file_externally(const std::string &path)
{
    #if defined(__APPLE__)
    static const char argv0[] = "open";
    #else
    static const char argv0[] = "xdg-open";
    #endif
    const char *const argv[] = {argv0, path.c_str(), nullptr};

    struct SpawnData {
        SpawnData() = default;
        ~SpawnData()
        {
            if (attr)
                posix_spawnattr_destroy(attr);
            if (file)
                posix_spawn_file_actions_destroy(file);
        }

        SpawnData(const SpawnData &) = delete;
        SpawnData &operator=(const SpawnData &) = delete;

        posix_spawn_file_actions_t *file = nullptr;
        posix_spawnattr_t *attr = nullptr;
    };

    SpawnData s;
    int err;

    posix_spawn_file_actions_t file;
    err = posix_spawn_file_actions_init(&file);
    if (err != 0)
        throw std::system_error(err, std::generic_category());
    s.file = &file;

    posix_spawnattr_t attr;
    err = posix_spawnattr_init(&attr);
    if (err != 0)
        throw std::system_error(err, std::generic_category());
    s.attr = &attr;

    pid_t pid;
    err = posix_spawnp(&pid, argv0, &file, &attr, const_cast<char **>(argv), environ);
    if (err != 0) {
        std::error_code ec(err, std::generic_category());
        Log::e("Cannot run \"%s\": %s", argv0, ec.message().c_str());
        return false;
    }

    while (waitpid(pid, nullptr, 0) == -1 && errno == EINTR);

    return true;
}
#endif
