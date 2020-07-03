//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "logs.h"
#include <sys/time.h>
#include <system_error>
#include <mutex>
#include <cstdio>
#include <ctime>

void Log::i(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    generic('-', "info", "\033[36m", format, ap);
    va_end(ap);
}

void Log::w(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    generic('!', "warn", "\033[33m", format, ap);
    va_end(ap);
}

void Log::e(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    generic('x', "error", "\033[31m", format, ap);
    va_end(ap);
}

void Log::s(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    generic('*', "success", "\033[32m", format, ap);
    va_end(ap);
}

void Log::generic(char symbol, const char *tag, const char *color, const char *format, va_list ap)
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    timeval tv;
    tm tm;
    if (gettimeofday(&tv, nullptr) == -1)
        throw std::system_error(errno, std::generic_category());

    time_t ts = tv.tv_sec;
#if !defined(_WIN32)
    if (!localtime_r(&ts, &tm))
        throw std::system_error(errno, std::generic_category());
#else
    errno_t localtime_errno = localtime_s(&tm, &ts);
    if (localtime_errno != 0)
        throw std::system_error(localtime_errno, std::generic_category());
#endif

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%X", &tm);

    fprintf(stderr, "%s.%03d [%c] %-8s %s", timebuf, (unsigned)tv.tv_usec / 1000 % 1000, symbol, tag, color);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "%s\n", "\033[0m");
}
