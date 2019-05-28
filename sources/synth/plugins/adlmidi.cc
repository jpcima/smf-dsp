//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "utility/paths.h"
#include <adlmidi.h>
#include <memory>
#include <cstring>

///
struct ADL_MIDIPlayer_delete { void operator()(ADL_MIDIPlayer *x) const { adl_close(x); } };
typedef std::unique_ptr<ADL_MIDIPlayer, ADL_MIDIPlayer_delete> ADL_MIDIPlayer_u;

///
struct adlmidi_synth_object {
    double srate = 0;
    ADL_MIDIPlayer_u player;

    int chip_count = 0;
    std::string instrument_bank;
};

static std::string adlmidi_synth_base_dir;

static void adlmidi_plugin_init(const char *base_dir)
{
    adlmidi_synth_base_dir.assign(base_dir);
}

static void adlmidi_plugin_shutdown()
{
}

static const synth_option the_synth_options[] = {
    {"chip-count", "Number of emulated chips", 'i', {.i = 6}},
    {"instrument-bank", "Bank number, or WOPL file path", 's', {.s = "0"}},
};

static const synth_option *adlmidi_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *adlmidi_synth_instantiate(double srate)
{
    std::unique_ptr<adlmidi_synth_object> obj(new adlmidi_synth_object);
    if (!obj)
        return nullptr;

    obj->srate = srate;

    return (synth_object *)obj.release();
}

static void adlmidi_synth_cleanup(synth_object *obj)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;
    delete sy;
}

static int adlmidi_synth_activate(synth_object *obj)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;

    ADL_MIDIPlayer *player = adl_init(sy->srate);
    if (!player)
        return -1;
    sy->player.reset(player);

    if (adl_setNumChips(player, sy->chip_count) != 0)
        fprintf(stderr, "adlmidi: cannot set chip count %d\n", sy->chip_count);

    int bank_no = 0;
    unsigned scan_count = 0;
    if (sscanf(sy->instrument_bank.c_str(), "%d%n", &bank_no, &scan_count) == 1 && scan_count == sy->instrument_bank.size()) {
        if (adl_setBank(player, bank_no) != 0)
            fprintf(stderr, "adlmidi: cannot set bank number %d\n", bank_no);
    }
    else {
        std::string path = sy->instrument_bank;
        if (!is_path_absolute(path))
            path = adlmidi_synth_base_dir + path;
        if (adl_openBankFile(player, path.c_str()) != 0)
            fprintf(stderr, "adlmidi: cannot set bank file \"%s\"\n", path.c_str());
    }

    return 0;
}

static void adlmidi_synth_deactivate(synth_object *obj)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;
    sy->player.reset();
}

static void adlmidi_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;
    ADL_MIDIPlayer *player = sy->player.get();

    if (size <= 0)
        return;

    unsigned status = msg[0];
    if (status == 0xf0) {
        adl_rt_systemExclusive(player, msg, size);
        return;
    }

    unsigned channel = status & 0x0f;
    switch (status >> 4) {
    case 0b1001: {
        if (size < 3) break;
        unsigned vel = msg[2] & 0x7f;
        if (vel != 0) {
            unsigned note = msg[1] & 0x7f;
            adl_rt_noteOn(player, channel, note, vel);
            break;
        }
    }
    case 0b1000: {
        if (size < 3) break;
        unsigned note = msg[1] & 0x7f;
        adl_rt_noteOff(player, channel, note);
        break;
    }
    case 0b1010:
        if (size < 3) break;
        adl_rt_noteAfterTouch(player, channel, msg[1] & 0x7f, msg[2] & 0x7f);
        break;
    case 0b1101:
        if (size < 2) break;
        adl_rt_channelAfterTouch(player, channel, msg[1] & 0x7f);
        break;
    case 0b1011: {
        if (size < 3) break;
        unsigned cc = msg[1] & 0x7f;
        unsigned val = msg[2] & 0x7f;
        adl_rt_controllerChange(player, channel, cc, val);
        break;
    }
    case 0b1100: {
        if (size < 2) break;
        unsigned pgm = msg[1] & 0x7f;
        adl_rt_patchChange(player, channel, pgm);
        break;
    }
    case 0b1110:
        if (size < 3) break;
        unsigned value = (msg[1] & 0x7f) | ((msg[2] & 0x7f) << 7);
        adl_rt_pitchBend(player, channel, value);
        break;
    }
}

static void adlmidi_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;
    ADL_MIDIPlayer *player = sy->player.get();

    ADLMIDI_AudioFormat format;
    format.type = ADLMIDI_SampleType_F32;
    format.containerSize = sizeof(float);
    format.sampleOffset = 2 * sizeof(float);
    adl_generateFormat(player, 2 * nframes, (ADL_UInt8 *)frames, (ADL_UInt8 *)(frames + 1), &format);
}

static void adlmidi_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    adlmidi_synth_object *sy = (adlmidi_synth_object *)obj;

    if (!strcmp(name, "chip-count"))
        sy->chip_count = value.i;
    else if (!strcmp(name, "instrument-bank"))
        sy->instrument_bank.assign(value.s);
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "FM-OPL3 (ADLMIDI)",
    &adlmidi_plugin_init,
    &adlmidi_plugin_shutdown,
    &adlmidi_plugin_option,
    &adlmidi_synth_instantiate,
    &adlmidi_synth_cleanup,
    &adlmidi_synth_activate,
    &adlmidi_synth_deactivate,
    &adlmidi_synth_write,
    &adlmidi_synth_generate,
    &adlmidi_synth_set_option,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
