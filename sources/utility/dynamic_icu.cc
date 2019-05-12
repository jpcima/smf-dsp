#include "dynamic_icu.h"
#include <mutex>
#include <string>
#include <cstring>

Icuuc_Lib::Icuuc_Lib()
{
#if defined (_WIN32)
    const char libname[] = "icuuc.dll";
#elif defined (__APPLE__)
    const char libname[] = "libicucore.dylib"; // "libicuuc.dylib"
#else
    const char libname[] = "libicuuc.so";
#endif

    Dl_Handle handle = Dl_open(libname);
    if (!handle)
        return;

    handle_.reset(handle);
    version_ = extract_version(libname, handle);
}

Icuuc_Lib &Icuuc_Lib::instance()
{
    static std::mutex s_mutex;
    static std::unique_ptr<Icuuc_Lib> s_ptr;
    std::lock_guard<std::mutex> lock(s_mutex);
    Icuuc_Lib *ptr = s_ptr.get();
    if (!ptr) {
        ptr = new Icuuc_Lib;
        s_ptr.reset(ptr);
    }
    return *ptr;
}

void *Icuuc_Lib::find_symbol(const char *name)
{
    Dl_Handle handle = handle_.get();
    if (!handle)
        return nullptr;

    void *sym = Dl_sym(handle, name);
    if (sym)
        return sym;

    unsigned version = version_;
    if (version == 0)
        return nullptr;

    std::string name2 = std::string(name) + '_' + std::to_string(version);
    return Dl_sym(handle, name2.c_str());
}

#ifdef __gnu_linux__
#include <link.h>

unsigned Icuuc_Lib::extract_version(const char *libname, Dl_Handle handle)
{
    const link_map *link_map;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &link_map) == -1)
        return 0;

    const char *strtab = nullptr;
    ElfW(Xword) offsoname = ~(ElfW(Xword))0;
    for (ElfW(Dyn) *dyn = link_map->l_ld; dyn->d_tag != 0; ++dyn) {
        switch (dyn->d_tag) {
        case 5: // DT_STRTAB
            strtab = reinterpret_cast<const char *>(dyn->d_un.d_ptr);
            break;
        case 14: // DT_SONAME
            offsoname = dyn->d_un.d_val;
            break;
        }
    }

    if (!strtab || offsoname == ~(ElfW(Xword))0)
        return 0;

    const char *soname = strtab + offsoname;
    size_t sonamelen = strlen(soname);
    size_t libnamelen = strlen(libname);
    if (sonamelen < libnamelen || memcmp(libname, soname, libnamelen) != 0)
        return 0;

    unsigned version;
    if (sscanf(soname + libnamelen, ".%u", &version) != 1)
        return 0;

    return version;
}
#else
unsigned Icuuc_Lib::extract_version(const char *libname, Dl_Handle handle)
{
    // not implemented
    return 0;
}
#endif
