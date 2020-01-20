#pragma once
#include <uv.h>

#if (UV_VERSION_MAJOR < 1) || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR < 33)
void uv_sleep(unsigned msec);
#endif
