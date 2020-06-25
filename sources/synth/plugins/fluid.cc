//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "../synth_utility.h"
#include "utility/paths.h"
#include "utility/logs.h"
#include <fluidsynth.h>
#include <memory>
#include <cstdlib>
#include <cstring>

///
struct fluid_synth_delete { void operator()(fluid_synth_t *x) const noexcept { delete_fluid_synth(x); } };
struct fluid_settings_delete { void operator()(fluid_settings_t *x) const noexcept { delete_fluid_settings(x); } };

typedef std::unique_ptr<fluid_synth_t, fluid_synth_delete> fluid_synth_u;
typedef std::unique_ptr<fluid_settings_t, fluid_settings_delete> fluid_settings_u;
///

struct fluid_synth_object {
    string_list_ptr soundfonts;
    fluid_settings_u settings;
    fluid_synth_u synth;

    bool chorus_enable = false;
    int chorus_voices = 0;
    double chorus_level = 0;
    double chorus_speed = 0;
    double chorus_depth = 0;
    int chorus_type = 0;

    bool reverb_enable = false;
    double reverb_room_size = 0;
    double reverb_damping = 0;
    double reverb_width = 0;
    double reverb_level = 0;
};

static std::string fluid_synth_base_dir;

///

static void fluid_log_error(int, const char *message, void *)
{
    Log::e("[fluid] %s", message);
}

static void fluid_log_warning(int, const char *message, void *)
{
    Log::w("[fluid] %s", message);
}

static void fluid_log_info(int, const char *message, void *)
{
    Log::i("[fluid] %s", message);
}

///

static void fluid_plugin_init(const char *base_dir)
{
    fluid_synth_base_dir.assign(base_dir);

    fluid_set_log_function(FLUID_PANIC, &fluid_log_error, nullptr);
    fluid_set_log_function(FLUID_ERR, &fluid_log_error, nullptr);
    fluid_set_log_function(FLUID_WARN, &fluid_log_warning, nullptr);
    fluid_set_log_function(FLUID_INFO, &fluid_log_info, nullptr);
    fluid_set_log_function(FLUID_PANIC, nullptr, nullptr);
}

static void fluid_plugin_shutdown()
{
    fluid_set_log_function(FLUID_PANIC, nullptr, nullptr);
    fluid_set_log_function(FLUID_ERR, nullptr, nullptr);
    fluid_set_log_function(FLUID_WARN, nullptr, nullptr);
    fluid_set_log_function(FLUID_INFO, nullptr, nullptr);
    fluid_set_log_function(FLUID_PANIC, nullptr, nullptr);
}

static const char *default_soundfont_value[] = {"A320U.sf2", nullptr};

static const synth_option the_synth_options[] = {
    {"soundfont", "List of SoundFont files to load", 'm', {.m = default_soundfont_value}},
    {"chorus-enable", "Enable chorus effect", 'b', {.b = true}},
    {"chorus-voices", "Chorus voice count [0:99]", 'i', {.i = 3}},
    {"chorus-level", "Chorus level [0:10]", 'f', {.f = 2.0}},
    {"chorus-speed", "Chorus speed (Hz) [0.29:5]", 'f', {.f = 0.3}},
    {"chorus-depth", "Chorus depth (ms) [0:21]", 'f', {.f = 8.0}},
    {"chorus-type", "Chorus type [0=sine;1=triangle]", 'i', {.i = 0}},
    {"reverb-enable", "Enable reverb effect", 'b', {.b = true}},
    {"reverb-room-size", "Reverb room size [0:1.2]", 'f', {.f = 0.2f}},
    {"reverb-damping", "Reverb damping [0:1]", 'f', {.f = 0.0f}},
    {"reverb-width", "Reverb width [0:100]", 'f', {.f = 0.5f}},
    {"reverb-level", "Reverb level [0:1]", 'f', {.f = 0.9f}},
};

static const synth_option *fluid_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *fluid_synth_instantiate(double srate)
{
    std::unique_ptr<fluid_synth_object> obj(new fluid_synth_object);
    if (!obj)
        return nullptr;

    obj->soundfonts.reset(new char *[1]());

    fluid_settings_t *settings = new_fluid_settings();
    if (!settings)
        return nullptr;
    obj->settings.reset(settings);

    fluid_settings_setnum(settings, "synth.sample-rate", srate);

    return (synth_object *)obj.release();
}

static void fluid_synth_cleanup(synth_object *obj)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;
    delete sy;
}

static int fluid_synth_activate(synth_object *obj)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;

    fluid_synth_t *synth = new_fluid_synth(sy->settings.get());
    if (!synth)
        return -1;
    sy->synth.reset(synth);

    for (char **p = sy->soundfonts.get(), *sf; (sf = *p); ++p) {
        std::string sf_absolute;
        if (!is_path_absolute(sf)) {
            sf_absolute = fluid_synth_base_dir + sf;
            sf = (char *)sf_absolute.c_str();
        }
        Log::i("[fluid] load soundfont: %s", sf);
        int sfid = fluid_synth_sfload(synth, sf, true);
        if (sfid == FLUID_FAILED)
            /**/;
    }

    fluid_synth_set_chorus_on(synth, sy->chorus_enable);
    fluid_synth_set_chorus(synth, sy->chorus_voices, sy->chorus_level, sy->chorus_speed, sy->chorus_depth, sy->chorus_type);
    fluid_synth_set_reverb_on(synth, sy->reverb_enable);
    fluid_synth_set_reverb(synth, sy->reverb_room_size, sy->reverb_damping, sy->reverb_width, sy->reverb_level);

    return 0;
}

static void fluid_synth_deactivate(synth_object *obj)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;

    sy->synth.reset();
}

static void fluid_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;
    fluid_synth_t *synth = sy->synth.get();

    if (size < 1)
        return;

    unsigned status = msg[0];

    if (status == 0xf0)
        fluid_synth_sysex(synth, (const char *)msg, size, nullptr, nullptr, nullptr, false);
    else if (status == 0xff)
        fluid_synth_system_reset(synth);
    else {
        unsigned data1 = (size > 1) ? (msg[1] & 127) : 0;
        unsigned data2 = (size > 2) ? (msg[2] & 127) : 0;
        unsigned channel = status & 0x0f;

        switch (status & 0xf0) {
        case 0x80:
            fluid_synth_noteoff(synth, channel, data1);
            break;
        case 0x90:
            fluid_synth_noteon(synth, channel, data1, data2);
            break;
        case 0xa0:
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            fluid_synth_key_pressure(synth, channel, data1, data2);
#endif
            break;
        case 0xb0:
            fluid_synth_cc(synth, channel, data1, data2);
            break;
        case 0xc0:
            fluid_synth_program_change(synth, channel, data1);
            break;
        case 0xd0:
            fluid_synth_channel_pressure(synth, channel, data1);
            break;
        case 0xe0:
            fluid_synth_pitch_bend(synth, channel, 128 * data2 + data1);
            break;
        }
    }
}

static void fluid_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;
    fluid_synth_t *synth = sy->synth.get();

    fluid_synth_write_float(synth, nframes, frames, 0, 2, frames, 1, 2);
}

static void fluid_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    fluid_synth_object *sy = (fluid_synth_object *)obj;

    if (!strcmp(name, "soundfont"))
        sy->soundfonts = string_list_dup(value.m);
    else if (!strcmp(name, "chorus-enable"))
        sy->chorus_enable = value.b;
    else if (!strcmp(name, "chorus-voices"))
        sy->chorus_voices = value.i;
    else if (!strcmp(name, "chorus-level"))
        sy->chorus_level = value.f;
    else if (!strcmp(name, "chorus-speed"))
        sy->chorus_speed = value.f;
    else if (!strcmp(name, "chorus-depth"))
        sy->chorus_depth = value.f;
    else if (!strcmp(name, "chorus-type"))
        sy->chorus_type = value.i;
    else if (!strcmp(name, "reverb-enable"))
        sy->reverb_enable = value.b;
    else if (!strcmp(name, "reverb-room-size"))
        sy->reverb_room_size = value.f;
    else if (!strcmp(name, "reverb-damping"))
        sy->reverb_damping = value.f;
    else if (!strcmp(name, "reverb-width"))
        sy->reverb_width = value.f;
    else if (!strcmp(name, "reverb-level"))
        sy->reverb_level = value.f;
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "Fluidsynth",
    &fluid_plugin_init,
    &fluid_plugin_shutdown,
    &fluid_plugin_option,
    &fluid_synth_instantiate,
    &fluid_synth_cleanup,
    &fluid_synth_activate,
    &fluid_synth_deactivate,
    &fluid_synth_write,
    &fluid_synth_generate,
    &fluid_synth_set_option,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
