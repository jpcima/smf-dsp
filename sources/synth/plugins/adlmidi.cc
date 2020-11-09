//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "utility/paths.h"
#include "utility/logs.h"
#include <adlmidi.h>
#include <algorithm>
#include <memory>
#include <cstring>
#include <cctype>

///
struct ADL_MIDIPlayer_delete { void operator()(ADL_MIDIPlayer *x) const { adl_close(x); } };
typedef std::unique_ptr<ADL_MIDIPlayer, ADL_MIDIPlayer_delete> ADL_MIDIPlayer_u;

///
struct adlmidi_synth_object {
    double srate = 0;
    ADL_MIDIPlayer_u player;

    int chip_count = 0;
    std::string instrument_bank;
    std::string emulator;
    std::string volume_model;
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
    {"chip-count", "Number of emulated chips", 'i', {.i = 4}},
    {"instrument-bank", "Bank number, or WOPL file path", 's', {.s = "0"}},
    {"emulator", "Name of the chip emulator", 's', {.s = "dosbox"}},
    {"volume-model", "Name of the volume model", 's', {.s = "auto"}},
};

struct named_emulator {
    const char *name;
    int value;
};

static const named_emulator the_emulators[] = {
    {"dosbox", ADLMIDI_EMU_DOSBOX},
    {"nuked", ADLMIDI_EMU_NUKED},
    {"nuked 1.7", ADLMIDI_EMU_NUKED_174},
    {"opal", ADLMIDI_EMU_OPAL},
    {"java", ADLMIDI_EMU_JAVA},
};

struct named_volume_model {
    const char *name;
    int value;
};

static const named_volume_model the_volume_models[] = {
    {"auto", ADLMIDI_VolumeModel_AUTO},
    {"generic", ADLMIDI_VolumeModel_Generic},
    {"native-opl3", ADLMIDI_VolumeModel_NativeOPL3},
    {"cmf", ADLMIDI_VolumeModel_CMF},
    {"dmx", ADLMIDI_VolumeModel_DMX},
    {"apogee", ADLMIDI_VolumeModel_APOGEE},
    {"9x", ADLMIDI_VolumeModel_9X},
    {"dmx-fixed", ADLMIDI_VolumeModel_DMX_Fixed},
    {"apogee-fixed", ADLMIDI_VolumeModel_APOGEE_Fixed},
    {"ail", ADLMIDI_VolumeModel_AIL},
    {"9x-generic-fm", ADLMIDI_VolumeModel_9X_GENERIC_FM},
    {"hmi", ADLMIDI_VolumeModel_HMI},
    {"hmi-old", ADLMIDI_VolumeModel_HMI_OLD},
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
        Log::e("adlmidi: cannot find an emulator named \"%s\"", sy->emulator.c_str());
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
        Log::e("adlmidi: cannot find a volume model named \"%s\"", sy->volume_model.c_str());
        volume_model = the_volume_models[0].value;
    }

    //
    if (adl_switchEmulator(player, emulator) != 0)
        Log::e("adlmidi: cannot set emulator");

    adl_setVolumeRangeModel(player, volume_model);

    if (adl_setNumChips(player, sy->chip_count) != 0)
        Log::e("adlmidi: cannot set chip count %d", sy->chip_count);

    Log::i("adlmidi: use %d chips \"%s\"", adl_getNumChips(player), adl_chipEmulatorName(player));

    int bank_no = 0;
    unsigned scan_count = 0;
    if (sscanf(sy->instrument_bank.c_str(), "%d%n", &bank_no, &scan_count) == 1 && scan_count == sy->instrument_bank.size()) {
        Log::i("adlmidi: set bank number %d", bank_no);
        if (adl_setBank(player, bank_no) != 0)
            Log::e("adlmidi: cannot set bank number %d", bank_no);
    }
    else {
        std::string path = sy->instrument_bank;
        if (!is_path_absolute(path))
            path = adlmidi_synth_base_dir + path;
        Log::i("adlmidi: set bank file %s", path.c_str());
        if (adl_openBankFile(player, path.c_str()) != 0)
            Log::e("adlmidi: cannot set bank file \"%s\"", path.c_str());
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
    else if (!strcmp(name, "emulator"))
        sy->emulator.assign(value.s);
    else if (!strcmp(name, "volume-model"))
        sy->volume_model.assign(value.s);
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
