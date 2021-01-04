//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define SYNTH_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define SYNTH_EXPORT __attribute__((visibility("default")))
#endif

enum {
    SYNTH_ABI_VERSION = 2
};

typedef struct _synth_object synth_object;

typedef union _synth_value {
    long i;
    double f;
    bool b;
    const char *s;
    const char *const *m;
} synth_value;

typedef struct _synth_option {
    const char *name;
    const char *description;
    unsigned type;
    synth_value initial;
} synth_option;

typedef struct _synth_midi_ins {
    unsigned char percussive : 1;
    unsigned char program : 7;
    unsigned char bank_msb : 7;
    unsigned char bank_lsb : 7;
} synth_midi_ins;

typedef struct _synth_interface {
    unsigned abi_version;
    const char *name;
    // ABI level 1
    void (*plugin_init)(const char *);
    void (*plugin_shutdown)();
    const synth_option *(*plugin_option)(size_t);
    synth_object *(*synth_instantiate)(double);
    void (*synth_cleanup)(synth_object *);
    int (*synth_activate)(synth_object *);
    void (*synth_deactivate)(synth_object *);
    void (*synth_write)(synth_object *, const unsigned char *, size_t);
    void (*synth_generate)(synth_object *, float *, size_t);
    void (*synth_set_option)(synth_object *, const char *, synth_value);
    // ABI level 2
    void (*synth_preload)(synth_object *, const synth_midi_ins *, size_t);
} synth_interface;

typedef const synth_interface *(synth_plugin_entry_fn)();

#ifdef __cplusplus
};
#endif
