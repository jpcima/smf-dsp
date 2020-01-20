#include "uv++.h"
#ifdef _WIN32
#   include <windows.h>
#else
#   include <time.h>
#   include <errno.h>
#endif

#if (UV_VERSION_MAJOR < 1) || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR < 33)
void uv_sleep(unsigned msec)
{
#ifdef _WIN32
    Sleep(msec);
#else
    struct timespec timeout;
    timeout.tv_sec = msec / 1000;
    timeout.tv_nsec = (msec % 1000) * 1000000;
    while (nanosleep(&timeout, &timeout) == -1 && errno == EINTR);
#endif
}
#endif
