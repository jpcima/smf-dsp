#pragma once
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif
#include <memory>

#ifndef _WIN32
typedef void *Dl_Handle;

inline Dl_Handle Dl_open(const char *name)
{
    return dlopen(name, RTLD_LAZY);
}

inline void Dl_close(Dl_Handle handle)
{
    dlclose(handle);
}

inline void *Dl_sym(Dl_Handle handle, const char *name)
{
    return dlsym(handle, name);
}
#else
typedef HMODULE Dl_Handle;

inline Dl_Handle Dl_open(const char *name)
{
    return LoadLibraryA(name);
}

inline void Dl_close(Dl_Handle handle)
{
    FreeLibrary(handle);
}

inline void *Dl_sym(Dl_Handle handle, const char *name)
{
    return reinterpret_cast<void *>(GetProcAddress(handle, name));
}
#endif

struct Dl_Handle_Deleter {
    void operator()(Dl_Handle x) const noexcept {
        Dl_close(x);
    }
};

typedef std::unique_ptr<
    std::remove_pointer<Dl_Handle>::type, Dl_Handle_Deleter> Dl_Handle_U;
