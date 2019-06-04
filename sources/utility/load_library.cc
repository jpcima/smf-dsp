//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "load_library.h"
#ifndef _WIN32
#include <dlfcn.h>
#else
#include "charset.h"
#include <windows.h>
#endif

#ifndef _WIN32
Dl_Handle Dl_open(const char *name)
{
    return dlopen(name, RTLD_LAZY);
}

void Dl_close(Dl_Handle handle)
{
    dlclose(handle);
}

void *Dl_sym(Dl_Handle handle, const char *name)
{
    return dlsym(handle, name);
}
#else
Dl_Handle Dl_open(const char *name)
{
    std::wstring wname;
    if (!convert_utf<char, wchar_t>(name, wname, false))
        return nullptr;
    return (Dl_Handle)LoadLibraryW(wname.c_str());
}

void Dl_close(Dl_Handle handle)
{
    FreeLibrary((HMODULE)handle);
}

void *Dl_sym(Dl_Handle handle, const char *name)
{
    return reinterpret_cast<void *>(GetProcAddress((HMODULE)handle, name));
}
#endif
