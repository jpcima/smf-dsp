#include "../synth.h"
#include "utility/paths.h"
#define MT32EMU_API_TYPE 1
#include <mt32emu/mt32emu.h>
#include <string>
#include <memory>
#include <cstring>

static constexpr bool mt32emu_debug_enabled = false;

///
struct mt32emu_context_deleter { void operator()(mt32emu_context x) const noexcept { mt32emu_free_context(x); } };
typedef std::unique_ptr<mt32emu_data, mt32emu_context_deleter> mt32emu_context_u;

///
struct mt32emu_synth_object {
    double srate = 0;
    std::string control_rom;
    std::string pcm_rom;
    bool gm_emulation = false;
    int partial_count = 0;
    mt32emu_context_u context;
};

static std::string mt32emu_synth_base_dir;

static void mt32emu_plugin_init(const char *base_dir)
{
    mt32emu_synth_base_dir.assign(base_dir);
}

static void mt32emu_plugin_shutdown()
{
}

static const synth_option the_synth_options[] = {
    {"control-rom", "MT-32 control ROM file", 's', {.s = "MT32_CONTROL.ROM"}},
    {"pcm-rom", "MT-32 PCM ROM file", 's', {.s = "MT32_PCM.ROM"}},
    {"gm-emulation", "Use General MIDI emulation", 'b', {.b = true}},
    {"partial-count", "Number of partials", 'i', {.i = 32}},
};

static const synth_option *mt32emu_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *mt32emu_synth_instantiate(double srate)
{
    std::unique_ptr<mt32emu_synth_object> obj(new mt32emu_synth_object);
    if (!obj)
        return nullptr;

    obj->srate = srate;

    return (synth_object *)obj.release();
}

static void mt32emu_synth_cleanup(synth_object *obj)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;
    delete sy;
}

static const mt32emu_report_handler_i_v0 the_report_handler = {
    /** Returns the actual interface version ID */
    +[](mt32emu_report_handler_i i) -> mt32emu_report_handler_version
          { return MT32EMU_REPORT_HANDLER_VERSION_0; },
    /** Callback for debug messages, in vprintf() format */
    +[](void *instance_data, const char *fmt, va_list list)
    {
        if (!mt32emu_debug_enabled) return;
        fputs("mt32emu: ", stderr); vfprintf(stderr, fmt, list); fputc('\n', stderr);
    },
    /** Callbacks for reporting errors */
    +[](void *instance_data) {},
    +[](void *instance_data) {},
    /** Callback for reporting about displaying a new custom message on LCD */
    +[](void *instance_data, const char *message) {},
    /** Callback for reporting actual processing of a MIDI message */
    +[](void *instance_data) {},
    /**
     * Callback for reporting an overflow of the input MIDI queue.
     * Returns MT32EMU_BOOL_TRUE if a recovery action was taken
     * and yet another attempt to enqueue the MIDI event is desired.
     */
    +[](void *instance_data) -> mt32emu_boolean { return MT32EMU_BOOL_FALSE; },
    /**
     * Callback invoked when a System Realtime MIDI message is detected in functions
     * mt32emu_parse_stream and mt32emu_play_short_message and the likes.
     */
    +[](void *instance_data, mt32emu_bit8u system_realtime) {},
    /** Callbacks for reporting system events */
    +[](void *instance_data) {},
    +[](void *instance_data) {},
    /** Callbacks for reporting changes of reverb settings */
    +[](void *instance_data, mt32emu_bit8u mode) {},
    +[](void *instance_data, mt32emu_bit8u time) {},
    +[](void *instance_data, mt32emu_bit8u level) {},
    /** Callbacks for reporting various information */
    +[](void *instance_data, mt32emu_bit8u part_num) {},
    +[](void *instance_data, mt32emu_bit8u part_num, const char *sound_group_name, const char *patch_name) {},
};

static int mt32emu_synth_activate(synth_object *obj)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;

    mt32emu_report_handler_i report_handler;
    report_handler.v0 = &the_report_handler;

    mt32emu_context_u context(mt32emu_create_context(report_handler, obj));
    if (!context) {
        fprintf(stderr, "mt32emu: error creating context\n");
        return -1;
    }

    mt32emu_set_stereo_output_samplerate(context.get(), sy->srate);
    mt32emu_select_renderer_type(context.get(), MT32EMU_RT_FLOAT);

    std::string control_rom = sy->control_rom;
    std::string pcm_rom = sy->pcm_rom;
    if (!is_path_absolute(control_rom))
        control_rom = mt32emu_synth_base_dir + control_rom;
    if (!is_path_absolute(pcm_rom))
        pcm_rom = mt32emu_synth_base_dir + pcm_rom;

    if (mt32emu_add_rom_file(context.get(), control_rom.c_str()) < 0) {
        fprintf(stderr, "mt32emu: cannot add control ROM \"%s\"\n", control_rom.c_str());
        return -1;
    }
    if (mt32emu_add_rom_file(context.get(), pcm_rom.c_str()) < 0) {
        fprintf(stderr, "mt32emu: cannot add PCM ROM \"%s\"\n", pcm_rom.c_str());
        return -1;
    }

    mt32emu_rom_info rom_info;
    mt32emu_get_rom_info(context.get(), &rom_info);

    fprintf(stderr, "mt32emu: using control ROM \"%s\"\n", rom_info.control_rom_description);
    fprintf(stderr, "mt32emu: using PCM ROM \"%s\"\n", rom_info.pcm_rom_description);

    mt32emu_set_partial_count(context.get(), sy->partial_count);

    if (mt32emu_open_synth(context.get()) != MT32EMU_RC_OK) {
        fprintf(stderr, "mt32emu: cannot open synth\n");
        return -1;
    }

    fprintf(stderr, "mt32emu: number of partials %u\n", mt32emu_get_partial_count(context.get()));

    sy->context = std::move(context);
    return 0;
}

static void mt32emu_synth_deactivate(synth_object *obj)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;

    sy->context.reset();
}

static const uint8_t patch_gm_to_mt32[128] = {
    0, // Acoustic Grand Piano -> Acou Piano 1
    1, // Bright Acoustic Piano -> Acou Piano 2
    3, // Electric Grand Piano -> Elec Piano 1
    7, // Honky-tonk Piano -> Honkytonk
    5, // Rhodes Piano -> Elec Piano 3
    6, // Chorused Piano -> Elec Piano 4
    17, // Harpsichord -> Harpsi 2
    21, // Clavinet -> Clavi 3
    22, // Celesta -> Celesta 1
    101, // Glockenspiel -> Glock
    101, // Music Box -> Glock
    98, // Vibraphone -> Vibe 2
    104, // Marimba -> Marimba
    103, // Xylophone -> Xylophone
    102, // Tubular Bells -> Tube Bell
    105, // Dulcimer -> Koto
    12, // Hammond Organ -> Pipe Org 1
    9, // Percussive Organ -> Elec Org 2
    10, // Rock Organ -> Elec Org 3
    13, // Church Organ -> Pipe Org 2
    14, // Reed Organ -> Pipe Org 3
    15, // Accordion -> Accordion
    87, // Harmonica -> Harmonica
    15, // Tango Accordion -> Accordion
    59, // Acoustic Guitar (nylon) -> Guitar 1
    60, // Acoustic Guitar (steel) -> Guitar 2
    59, // Electric Guitar (jazz) -> Guitar 1
    62, // Electric Guitar (clean) -> Elec Gtr 2
    61, // Electric Guitar (muted) -> Elec Gtr 1
    59, // Overdriven Guitar -> Guitar 1
    62, // Distortion Guitar -> Elec Gtr 2
    62, // Guitar Harmonics -> Elec Gtr 2
    64, // Acoustic Bass -> Acou Bass 1
    67, // Electric Bass (finger) -> Elec Bass 2
    66, // Electric Bass (pick) -> Elec Bass 1
    71, // Fretless Bass -> Fretless 2
    68, // Slap Bass 1 -> Slap Bass 1
    69, // Slap Bass 2 -> Slap Bass 2
    66, // Synth Bass 1 -> Elec Bass 1
    70, // Synth Bass 2 -> Fretless 1
    53, // Violin -> Violin 2
    52, // Viola -> Violin 1
    54, // Cello -> Cello 1
    56, // Contrabass -> Contrabass
    53, // Tremolo Strings -> Violin 2
    51, // Pizzicato Strings -> Pizzicato
    57, // Orchestral Harp -> Harp 1
    112, // Timpani -> Timpani
    48, // String Ensemble 1 -> Str Sect 1
    50, // String Ensemble 2 -> Str Sect 3
    48, // SynthStrings 1 -> Str Sect 1
    50, // SynthStrings 2 -> Str Sect 3
    34, // Choir Aahs -> Chorale
    42, // Voice Oohs -> Oboe 2001
    33, // Synth Voice -> Harmo Pan
    122, // Orchestra Hit -> Orche Hit
    88, // Trumpet -> Trumpet 1
    90, // Trombone -> Trombone 1
    94, // Tuba -> Tuba
    89, // Muted Trumpet -> Trumpet 2
    92, // French Horn -> Fr Horn 1
    95, // Brass Section -> Brs Sect 1
    89, // Synth Brass 1 -> Trumpet 2
    91, // Synth Brass 2 -> Trombone 2
    78, // Soprano Sax -> Sax 1
    79, // Alto Sax -> Sax 2
    80, // Tenor Sax -> Sax 3
    81, // Baritone Sax -> Sax 4
    84, // Oboe -> Oboe
    85, // English Horn -> Engl Horn
    86, // Bassoon -> Bassoon
    83, // Clarinet -> Clarinet 2
    75, // Piccolo -> Piccolo 2
    73, // Flute -> Flute 2
    76, // Recorder -> Recorder
    77, // Pan Flute -> Pan Pipes
    110, // Bottle Blow -> Bottleblow
    107, // Shakuhachi -> Shakuhachi
    108, // Whistle -> Whistle 1
    72, // Ocarina -> Flute 1
    47, // Lead 1 (square) -> Square Wave
    67, // Lead 2 (sawtooth) -> Elec Bass 2
    75, // Lead 3 (calliope lead) -> Piccolo 2
    51, // Lead 4 (chiff lead) -> Pizzicato
    61, // Lead 5 (charang) -> Elec Gtr 1
    72, // Lead 6 (voice) -> Flute 1
    52, // Lead 7 (fifths) -> Violin 1
    67, // Lead 8 (bass + lead) -> Elec Bass 2
    32, // Pad 1 (new age) -> Fantasy
    33, // Pad 2 (warm) -> Harmo Pan
    67, // Pad 3 (polysynth) -> Elec Bass 2
    34, // Pad 4 (choir) -> Chorale
    32, // Pad 5 (bowed) -> Fantasy
    32, // Pad 6 (metallic) -> Fantasy
    33, // Pad 7 (halo) -> Harmo Pan
    33, // Pad 8 (sweep) -> Harmo Pan
    41, // FX 1 (rain) -> Ice Rain
    36, // FX 2 (soundtrack) -> Soundtrack
    35, // FX 3 (crystal) -> Glasses
    37, // FX 4 (atmosphere) -> Atmosphere
    45, // FX 5 (brightness) -> School Daze
    33, // FX 6 (goblins) -> Harmo Pan
    43, // FX 7 (echoes) -> Echo Pan
    32, // FX 8 (sci-fi) -> Fantasy
    63, // Sitar -> Sitar
    105, // Banjo -> Koto
    105, // Shamisen -> Koto
    105, // Koto -> Koto
    51, // Kalimba -> Pizzicato
    81, // Bagpipe -> Sax 4
    52, // Fiddle -> Violin 1
    81, // Shanai -> Sax 4
    23, // Tinkle Bell -> Celesta 2
    103, // Agogo -> Xylophone
    103, // Steel Drums -> Xylophone
    113, // Woodblock -> Melodic Tom
    117, // Taiko Drum -> Taiko
    113, // Melodic Tom -> Melodic Tom
    116, // Synth Drum -> Elec Perc 2
    119, // Reverse Cymbal -> Cymbal
    124, // Guitar Fret Noise -> Bird Tweet
    120, // Breath Noise -> Castanets
    119, // Seashore -> Cymbal
    124, // Bird Tweet -> Bird Tweet
    123, // Telephone Ring -> Telephone
    120, // Helicopter -> Castanets
    119, // Applause -> Cymbal
    114, // Gunshot -> Deep Snare
};

static uint32_t convert_gm_to_mt32(uint32_t msg)
{
    unsigned status = msg & 0xff;
    unsigned channel = status & 0x0f;
    unsigned data1 = (msg >> 8) & 0x7f;
    //unsigned data2 = (msg >> 16) & 0x7f;

    switch (status & 0xf0) {
    case 0xb0: // controller change
        if (data1 == 0) // bank select
            msg = 0;
        break;
    case 0xc0: // program change
        if (channel == 9)
            msg = 0;
        else
            msg = status | (patch_gm_to_mt32[data1] << 8);
        break;
    }

    return msg;
}

static void mt32emu_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;
    mt32emu_context context = sy->context.get();

    uint32_t short_msg = 0;
    switch (size) {
    case 4:
        short_msg |= msg[3] << 24;
        /* fall through */
    case 3:
        short_msg |= msg[2] << 16;
        /* fall through */
    case 2:
        short_msg |= msg[1] << 8;
        /* fall through */
    case 1:
        short_msg |= msg[0];
        if (sy->gm_emulation)
            short_msg = convert_gm_to_mt32(short_msg);
        if (short_msg)
            mt32emu_play_msg(context, short_msg);
        break;
    case 0:
        break;
    default:
        mt32emu_play_sysex(context, msg, size);
        break;
    }
}

static void mt32emu_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;
    mt32emu_context context = sy->context.get();

    mt32emu_render_float(context, frames, nframes);
}

static void mt32emu_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    mt32emu_synth_object *sy = (mt32emu_synth_object *)obj;

    if (!strcmp(name, "control-rom"))
        sy->control_rom.assign(value.s);
    else if (!strcmp(name, "pcm-rom"))
        sy->pcm_rom.assign(value.s);
    else if (!strcmp(name, "gm-emulation"))
        sy->gm_emulation = value.b;
    else if (!strcmp(name, "partial-count"))
        sy->partial_count = value.i;
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "MT32EMU",
    &mt32emu_plugin_init,
    &mt32emu_plugin_shutdown,
    &mt32emu_plugin_option,
    &mt32emu_synth_instantiate,
    &mt32emu_synth_cleanup,
    &mt32emu_synth_activate,
    &mt32emu_synth_deactivate,
    &mt32emu_synth_write,
    &mt32emu_synth_generate,
    &mt32emu_synth_set_option,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
