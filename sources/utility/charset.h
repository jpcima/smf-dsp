//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <gsl/gsl>
#include <string>
#include <memory>
#include <cstdio>

// conversion
bool to_utf8(gsl::cstring_span src, std::string &dst, const char *src_encoding, bool permissive);
bool has_valid_encoding(gsl::cstring_span src, const char *src_encoding);
template <class CharSrc, class CharDst> bool convert_utf(gsl::basic_string_span<const CharSrc> src, std::basic_string<CharDst> &dst, bool permissive);
uint32_t unicode_tolower(uint32_t ch);
uint32_t unicode_toupper(uint32_t ch);
uint32_t unicode_nfd_base(uint32_t ch);

// comparison
int utf8_strcoll(gsl::cstring_span a, gsl::cstring_span b);

// file I/O
FILE *fopen_utf8(const char *path, const char *mode);
int filemode_utf8(const char *path);

// directories
bool make_directory(gsl::cstring_span path);

class Dir {
public:
    explicit Dir(const gsl::cstring_span path);
    ~Dir();
    explicit operator bool() const noexcept { return dh_ != nullptr; }
    bool read_next(std::string &name);
#ifndef _WIN32
    int fd();
#endif

private:
    struct DirInfo;
    struct DirInfo_delete { void operator()(DirInfo *x) const noexcept; };
    std::unique_ptr<DirInfo, DirInfo_delete> dh_;
};

#if 0
// detection
class Encoding_Detector
{
public:
    Encoding_Detector();
    ~Encoding_Detector();
    void start();
    void finish();
    void feed(gsl::cstring_span text);
    const char *detected_encoding();

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};
#endif
