//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "utility/paths.h"
#include "utility/logs.h"
#include <opnmidi.h>
#include <algorithm>
#include <memory>
#include <cstring>
#include <cctype>

///
struct OPN2_MIDIPlayer_delete { void operator()(OPN2_MIDIPlayer *x) const { opn2_close(x); } };
typedef std::unique_ptr<OPN2_MIDIPlayer, OPN2_MIDIPlayer_delete> OPN2_MIDIPlayer_u;

///
struct opnmidi_synth_object {
    double srate = 0;
    OPN2_MIDIPlayer_u player;

    int chip_count = 0;
    std::string instrument_bank;
    std::string emulator;
    std::string volume_model;
};

static std::string opnmidi_synth_base_dir;

static void opnmidi_plugin_init(const char *base_dir)
{
    opnmidi_synth_base_dir.assign(base_dir);
}

static void opnmidi_plugin_shutdown()
{
}

static const synth_option the_synth_options[] = {
    {"chip-count", "Number of emulated chips", 'i', {.i = 4}},
    {"instrument-bank", "Bank number, or WOPN file path", 's', {.s = "0"}},
    {"emulator", "Name of the chip emulator", 's', {.s = "mame"}},
    {"volume-model", "Name of the volume model", 's', {.s = "auto"}},
};

struct named_emulator {
    const char *name;
    int value;
};

static const named_emulator the_emulators[] = {
    {"mame", OPNMIDI_EMU_MAME},
    {"mame2612", OPNMIDI_EMU_MAME},
    {"nuked", OPNMIDI_EMU_NUKED},
    {"gens", OPNMIDI_EMU_GENS},
    {"gx", OPNMIDI_EMU_GX},
    {"neko", OPNMIDI_EMU_NP2},
    {"mame2608", OPNMIDI_EMU_MAME_2608},
    {"pmdwin", OPNMIDI_EMU_PMDWIN},
};

struct named_volume_model {
    const char *name;
    int value;
};

static const named_volume_model the_volume_models[] = {
    {"auto", OPNMIDI_VolumeModel_AUTO},
    {"generic", OPNMIDI_VolumeModel_Generic},
    {"native-opn2", OPNMIDI_VolumeModel_NativeOPN2},
    {"dmx", OPNMIDI_VolumeModel_DMX},
    {"apogee", OPNMIDI_VolumeModel_APOGEE},
    {"9x", OPNMIDI_VolumeModel_9X},
};

static const synth_option *opnmidi_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *opnmidi_synth_instantiate(double srate)
{
    std::unique_ptr<opnmidi_synth_object> obj(new opnmidi_synth_object);
    if (!obj)
        return nullptr;

    obj->srate = srate;

    return (synth_object *)obj.release();
}

static void opnmidi_synth_cleanup(synth_object *obj)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;
    delete sy;
}

static int opnmidi_synth_activate(synth_object *obj)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;

    OPN2_MIDIPlayer *player = opn2_init(sy->srate);
    if (!player)
        return -1;
    sy->player.reset(player);

    auto str_to_lower = [](std::string text) -> std::string {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) -> char { return std::tolower(c); });
        return text;
    };

    //
    int emulator = -1;
    const unsigned num_emulators = sizeof(the_emulators) / sizeof(the_emulators[0]);

    const std::string emu_id = str_to_lower(sy->emulator);
    for (unsigned i = 0; i < num_emulators && emulator == -1; ++i) {
        if (emu_id == the_emulators[i].name)
            emulator = the_emulators[i].value;
    }
    if (emulator == -1) {
        Log::e("opnmidi: cannot find an emulator named \"%s\"", sy->emulator.c_str());
        emulator = the_emulators[0].value;
    }

    //
    int volume_model = -1;
    const unsigned num_volume_models = sizeof(the_volume_models) / sizeof(the_volume_models[0]);

    const std::string volmodel_id = str_to_lower(sy->volume_model);
    for (unsigned i = 0; i < num_volume_models && volume_model == -1; ++i) {
        if (volmodel_id == the_volume_models[i].name)
            volume_model = the_volume_models[i].value;
    }
    if (volume_model == -1) {
        Log::e("opnmidi: cannot find a volume model named \"%s\"", sy->volume_model.c_str());
        volume_model = the_volume_models[0].value;
    }

    //
    if (opn2_switchEmulator(player, emulator) != 0)
        Log::e("opnmidi: cannot set emulator");

    if (opn2_setNumChips(player, sy->chip_count) != 0)
        Log::e("opnmidi: cannot set chip count %d", sy->chip_count);

    Log::i("opnmidi: use %d chips \"%s\"", opn2_getNumChips(player), opn2_chipEmulatorName(player));

    int bank_no = 0;
    unsigned scan_count = 0;
    if (sscanf(sy->instrument_bank.c_str(), "%d%n", &bank_no, &scan_count) == 1 && scan_count == sy->instrument_bank.size()) {
        Log::i("opnmidi: set bank number %d", bank_no);
        #pragma message("TODO opnmidi: no library support for embedded banks")
        bool loaded = false;
        if (bank_no == 0) {
            const uint8_t bank_data[] = {
                #include "opnmidi_bank.dat"
            };
            loaded = opn2_openBankData(player, bank_data, sizeof(bank_data)) == 0;
        }
        if (!loaded)
            Log::e("opnmidi: cannot set bank number %d", bank_no);
    }
    else {
        std::string path = sy->instrument_bank;
        if (!is_path_absolute(path))
            path = opnmidi_synth_base_dir + path;
        Log::i("opnmidi: set bank file %s", path.c_str());
        if (opn2_openBankFile(player, path.c_str()) != 0)
            Log::e("opnmidi: cannot set bank file \"%s\"", path.c_str());
    }

    return 0;
}

static void opnmidi_synth_deactivate(synth_object *obj)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;
    sy->player.reset();
}

static void opnmidi_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;
    OPN2_MIDIPlayer *player = sy->player.get();

    if (size <= 0)
        return;

    unsigned status = msg[0];
    if (status == 0xf0) {
        opn2_rt_systemExclusive(player, msg, size);
        return;
    }

    unsigned channel = status & 0x0f;
    switch (status >> 4) {
    case 0b1001: {
        if (size < 3) break;
        unsigned vel = msg[2] & 0x7f;
        if (vel != 0) {
            unsigned note = msg[1] & 0x7f;
            opn2_rt_noteOn(player, channel, note, vel);
            break;
        }
    }
    case 0b1000: {
        if (size < 3) break;
        unsigned note = msg[1] & 0x7f;
        opn2_rt_noteOff(player, channel, note);
        break;
    }
    case 0b1010:
        if (size < 3) break;
        opn2_rt_noteAfterTouch(player, channel, msg[1] & 0x7f, msg[2] & 0x7f);
        break;
    case 0b1101:
        if (size < 2) break;
        opn2_rt_channelAfterTouch(player, channel, msg[1] & 0x7f);
        break;
    case 0b1011: {
        if (size < 3) break;
        unsigned cc = msg[1] & 0x7f;
        unsigned val = msg[2] & 0x7f;
        opn2_rt_controllerChange(player, channel, cc, val);
        break;
    }
    case 0b1100: {
        if (size < 2) break;
        unsigned pgm = msg[1] & 0x7f;
        opn2_rt_patchChange(player, channel, pgm);
        break;
    }
    case 0b1110:
        if (size < 3) break;
        unsigned value = (msg[1] & 0x7f) | ((msg[2] & 0x7f) << 7);
        opn2_rt_pitchBend(player, channel, value);
        break;
    }
}

static void opnmidi_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;
    OPN2_MIDIPlayer *player = sy->player.get();

    OPNMIDI_AudioFormat format;
    format.type = OPNMIDI_SampleType_F32;
    format.containerSize = sizeof(float);
    format.sampleOffset = 2 * sizeof(float);
    opn2_generateFormat(player, 2 * nframes, (OPN2_UInt8 *)frames, (OPN2_UInt8 *)(frames + 1), &format);
}

static void opnmidi_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    opnmidi_synth_object *sy = (opnmidi_synth_object *)obj;

    if (!strcmp(name, "chip-count"))
        sy->chip_count = value.i;
    else if (!strcmp(name, "instrument-bank"))
        sy->instrument_bank.assign(value.s);
    else if (!strcmp(name, "emulator"))
        sy->emulator.assign(value.s);
    else if (!strcmp(name, "volume-model"))
        sy->volume_model.assign(value.s);
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "FM-OPN2 (OPNMIDI)",
    &opnmidi_plugin_init,
    &opnmidi_plugin_shutdown,
    &opnmidi_plugin_option,
    &opnmidi_synth_instantiate,
    &opnmidi_synth_cleanup,
    &opnmidi_synth_activate,
    &opnmidi_synth_deactivate,
    &opnmidi_synth_write,
    &opnmidi_synth_generate,
    &opnmidi_synth_set_option,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
