#pragma once
#include "load_library.h"

class Icuuc_Lib {
protected:
    Icuuc_Lib();

public:
    static Icuuc_Lib &instance();
    void *find_symbol(const char *name);

private:
    Dl_Handle_U handle_;
    unsigned version_ = 0;

private:
    static unsigned extract_version(const char *libname, Dl_Handle handle);
};
