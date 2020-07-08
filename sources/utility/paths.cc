//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "paths.h"
#include "charset.h"
#include <list>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cerrno>
#include <cassert>
#include <unistd.h>
#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif
#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string get_home_directory()
{
#ifndef _WIN32
    if (const char *env = getenv("HOME")) {
        std::string dir = normalize_path_separators(env);
        if (is_path_absolute(dir)) {
            if (dir.back() != '/')
                dir.push_back('/');
            return dir;
        }
    }
    return std::string();
#else
    return known_folder_path(CSIDL_PROFILE|CSIDL_FLAG_CREATE);
#endif
}

std::string get_current_directory()
{
    char *cwd_ret = getcwd(nullptr, 0);
    if (!cwd_ret) {
        if (errno == ENOMEM)
            throw std::bad_alloc();
        return std::string();
    }
    auto cwd_cleanup = gsl::finally([cwd_ret] { free(cwd_ret); });

    std::string cwd = normalize_path_separators(cwd_ret);
    if (!is_path_absolute(cwd))
        return std::string();
    if (cwd.back() != '/')
        cwd.push_back('/');

    return cwd;
}

#ifdef _WIN32
static bool is_drive_letter(char32_t character)
{
    return (character >= 'A' && character <= 'Z') ||
        (character >= 'a' && character <= 'z');
}
#endif

bool is_path_absolute(gsl::cstring_span path)
{
#ifndef _WIN32
    return !path.empty() && path[0] == '/';
#else
    return path.size() >= 3 && is_drive_letter(path[0]) && path[1] == ':' &&
        is_path_separator(static_cast<unsigned char>(path[2]));
#endif
}

void append_path_separator(std::string &path)
{
    if (path.empty() || !is_path_separator(static_cast<unsigned char>(path.back())))
        path.push_back('/');
}

bool is_path_separator(char32_t character)
{
    if (character == '/')
        return true;
#ifdef _WIN32
    if (character == '\\')
        return true;
#endif
    return false;
}

std::string normalize_path_separators(gsl::cstring_span path)
{
    std::string result;
    result.reserve(path.size());

    bool has_sep_before = false;
    for (char c : path) {
        bool is_sep = is_path_separator(static_cast<unsigned char>(c));
        if (!is_sep)
            result.push_back(c);
        else if (!has_sep_before)
            result.push_back('/');
        has_sep_before = is_sep;
    }

    return result;
}

#ifndef _WIN32
std::string make_path_canonical(gsl::cstring_span path)
{
    char *buf = realpath(gsl::to_string(path).c_str(), nullptr);
    if (!buf) {
        if (errno == ENOMEM)
            throw std::bad_alloc();
        return std::string();
    }
    auto cleanup = gsl::finally([buf] { free(buf); });

    assert(buf[0] != '\0');

    struct stat st;
    if (stat(buf, &st) == -1)
        return std::string();

    std::string real;
    if (!S_ISDIR(st.st_mode))
        real.assign(buf);
    else {
        size_t size = strlen(buf);
        real.reserve(size + 1);
        real.append(buf, size);
        if (real.back() != '/')
            real.push_back('/');
    }

    return real;
}
#else
static std::list<gsl::cstring_span> split_path_components(gsl::cstring_span path)
{
    std::list<gsl::cstring_span> parts;
    gsl::cstring_span rest = path;
    if (!rest.empty()) {
        gsl::cstring_span::iterator it;
        while (it = std::find(rest.begin(), rest.end(), '/'), it != rest.end()) {
            parts.emplace_back(rest.begin(), it);
            rest = gsl::cstring_span(it + 1, rest.end());
        }
        parts.push_back(rest);
    }
    return parts;
}

static std::string join_path_components(const std::list<gsl::cstring_span> &parts)
{
    if (parts.empty())
        return std::string();

    size_t size = static_cast<size_t>(-1);
    for (gsl::cstring_span part : parts)
        size = size + 1 + part.size();

    std::string result;
    result.reserve(size);

    auto it = parts.begin();
    auto end = parts.end();
    result.append(it->data(), it->size());
    for (++it; it != end; ++it) {
        result.push_back('/');
        result.append(it->data(), it->size());
    }

    return result;
}

std::string make_path_canonical(gsl::cstring_span path)
{
#ifdef _WIN32
    while (!path.empty() && is_path_separator(path[0]))
        path = path.subspan(1);
#endif

    std::string rel = normalize_path_separators(path);
    std::string abs;

    if (is_path_absolute(rel))
        abs = rel;
    else {
        std::string cwd = get_current_directory();
        if (cwd.empty())
            return std::string();
        abs = cwd + rel;
    }

    // split path components and clean up
    std::list<gsl::cstring_span> parts;
    for (gsl::cstring_span part : split_path_components(abs)) {
        if (part == ".") {
        }
        else if (part == "..") {
            if (parts.size() > 1)
                parts.pop_back();
        }
        else
            parts.push_back(part);
    }

    return join_path_components(parts);
}
#endif

std::string expand_path_tilde(gsl::cstring_span path)
{
    if (path.empty())
        return std::string{};

    bool is_tilde_path = path[0] == '~' &&
        (path.size() == 1 || is_path_separator(path[1]));

    if (!is_tilde_path)
        return normalize_path_separators(path);

    std::string home = get_home_directory();
    if (home.empty())
        return std::string{};

    return normalize_path_separators(home + gsl::to_string(path.subspan(1)));
}

gsl::cstring_span path_file_name(gsl::cstring_span path)
{
    for (size_t i = path.size(); i-- > 0;) {
        if (path[i] == '/')
            return path.subspan(i + 1);
    }
    return path;
}

gsl::cstring_span path_directory(gsl::cstring_span path)
{
    for (size_t i = path.size(); i-- > 0;) {
        if (path[i] == '/')
            return path.subspan(0, i + 1);
    }
    return gsl::cstring_span();
}

std::string get_display_path(gsl::cstring_span path)
{
    std::string result(path.data(), path.size());

    std::string home = get_home_directory();
    if (!home.empty() && path.size() >= home.size() && !memcmp(path.data(), home.data(), home.size())) {
        result.erase(result.begin(), result.begin() + home.size());
        if (result.empty())
            result.assign("[Home]");
    }

    return result;
}

#if defined(_WIN32)
std::string known_folder_path(int csidl, std::error_code &ec)
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
    if (!convert_utf<WCHAR, char>(wpath, path, false)) {
        ec = std::error_code(EINVAL, std::generic_category());
        return std::string();
    }

    return normalize_path_separators(path + '/');
}

std::string known_folder_path(int csidl)
{
    std::error_code ec;
    std::string path = known_folder_path(csidl, ec);
    if (ec)
        throw std::system_error(ec);
    return path;
}
#endif
