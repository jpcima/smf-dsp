//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "utility/paths.h"
#include <emidi_alpha/CMIDIModule.hpp>
#include <emidi_alpha/CSccDevice.hpp>
#include <emidi_alpha/COpllDevice.hpp>
#include <algorithm>
#include <memory>
#include <cstring>

///
struct scc_synth_object {
    double srate = 0;
    unsigned module_count = 0;

    dsa::CMIDIModule module[16];
    std::unique_ptr<dsa::ISoundDevice> device[16];
};

static void scc_plugin_init(const char *base_dir)
{
}

static void scc_plugin_shutdown()
{
}

static const synth_option the_synth_options[] = {
    {"modules-count", "Number of emulated modules (2-16)", 'i', {.i = 8}},
};

static const synth_option *scc_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *scc_synth_instantiate(double srate)
{
    std::unique_ptr<scc_synth_object> obj(new scc_synth_object);
    if (!obj)
        return nullptr;

    obj->srate = srate;

    return (synth_object *)obj.release();
}

static void scc_synth_cleanup(synth_object *obj)
{
    scc_synth_object *sy = (scc_synth_object *)obj;
    delete sy;
}

static int scc_synth_activate(synth_object *obj)
{
    scc_synth_object *sy = (scc_synth_object *)obj;
    double srate = sy->srate;
    unsigned mods = sy->module_count;

    for (unsigned m = 0; m < mods; ++m) {
        dsa::ISoundDevice *dev;
        if (m & 1) dev = new dsa::CSccDevice(srate, 2);
        else dev = new dsa::COpllDevice(srate, 2);
        sy->device[m].reset(dev);
        sy->module[m].AttachDevice(dev);
        sy->module[m].Reset();
    }

    return 0;
}

static void scc_synth_deactivate(synth_object *obj)
{
    scc_synth_object *sy = (scc_synth_object *)obj;

    for (unsigned i = 0; i < 16; ++i)
        sy->device[i].reset();
}

static void scc_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    scc_synth_object *sy = (scc_synth_object *)obj;
    unsigned mods = sy->module_count;

    #pragma message("scc MIDI not realtime-safe")

    dsa::CMIDIMsgInterpreter mi;
    for (size_t i = 0; i < size; ++i)
        mi.Interpret(msg[i]);

    if (mi.GetMsgCount() < 1)
        return;

    const dsa::CMIDIMsg &mm = mi.GetMsg();
    //fprintf(stderr, "%s\n", mm.c_str());

    sy->module[(mm.m_ch * 2) % mods].SendMIDIMsg(mm);
    if (mm.m_ch != 9)
        sy->module[(mm.m_ch * 2 + 1) % mods].SendMIDIMsg(mm);
}

static void scc_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    scc_synth_object *sy = (scc_synth_object *)obj;
    unsigned mods = sy->module_count;

    std::fill(frames, frames + 2 * nframes, 0);

    for (unsigned m = 0; m < mods; ++m) {
        dsa::CMIDIModule &mod = sy->module[m];
        for (size_t i = 0; i < nframes; ++i) {
            int32_t b[2];
            mod.Render(b);
            frames[2 * i + 0] += b[0] * (1.0f / 32768);
            frames[2 * i + 1] += b[1] * (1.0f / 32768);
        }
    }
}

static void scc_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    scc_synth_object *sy = (scc_synth_object *)obj;

    if (!strcmp(name, "modules-count"))
        sy->module_count = std::max(16L, std::min(2L, value.i));
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "SCC (emidi)",
    &scc_plugin_init,
    &scc_plugin_shutdown,
    &scc_plugin_option,
    &scc_synth_instantiate,
    &scc_synth_cleanup,
    &scc_synth_activate,
    &scc_synth_deactivate,
    &scc_synth_write,
    &scc_synth_generate,
    &scc_synth_set_option,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
