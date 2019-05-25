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
    0, // Acou Piano 1 -> Acoustic Grand Piano
    1, // Acou Piano 2 -> Bright Acoustic Piano
    0, // Acou Piano 3 -> Acoustic Grand Piano
    2, // Elec Piano 1 -> Electric Grand Piano
    4, // Elec Piano 2 -> Rhodes Piano
    4, // Elec Piano 3 -> Rhodes Piano
    5, // Elec Piano 4 -> Chorused Piano
    3, // Honkytonk -> Honky-tonk Piano
    6, // Elec Org 1 -> Hammond Organ
    7, // Elec Org 2 -> Percussive Organ
    18, // Elec Org 3 -> Rock Organ
    16, // Elec Org 4 -> Hammond Organ
    16, // Pipe Org 1 -> Hammond Organ
    19, // Pipe Org 2 -> Church Organ
    20, // Pipe Org 3 -> Reed Organ
    21, // Accordion -> Accordion
    6, // Harpsi 1 -> Harpsichord
    6, // Harpsi 2 -> Harpsichord
    6, // Harpsi 3 -> Harpsichord
    7, // Clavi 1 -> Clavinet
    7, // Clavi 2 -> Clavinet
    7, // Clavi 3 -> Clavinet
    8, // Celesta 1 -> Celesta
    12, // Celesta 2 -> Tinkle Bell
    62, // Syn Brass 1 -> Synth Brass 1
    62, // Syn Brass 2 -> Synth Brass 1
    63, // Syn Brass 3 -> Synth Brass 2
    63, // Syn Brass 4 -> Synth Brass 2
    38, // Syn Bass 1 -> Synth Bass 1
    38, // Syn Bass 2 -> Synth Bass 1
    39, // Syn Bass 3 -> Synth Bass 2
    39, // Syn Bass 4 -> Synth Bass 2
    88, // Fantasy -> Pad 1 (new age)
    54, // Harmo Pan -> Synth Voice
    52, // Chorale -> Choir Aahs
    98, // Glasses -> FX 3 (crystal)
    97, // Soundtrack -> FX 2 (soundtrack)
    99, // Atmosphere -> FX 4 (atmosphere)
    14, // Warm Bell -> Tubular Bells
    54, // Funny Vox -> Synth Voice
    02, // Echo Bell -> FX 7 (echoes)
    96, // Ice Rain -> FX 1 (rain)
    53, // Oboe 2001 -> Voice Oohs
    02, // Echo Pan -> FX 7 (echoes)
    81, // Doctor Solo -> Lead 2 (sawtooth)
    00, // School Daze -> FX 5 (brightness)
    14, // Bellsinger -> Tubular Bells
    80, // Square Wave -> Lead 1 (square)
    48, // Str Sect 1 -> String Ensemble 1
    48, // Str Sect 2 -> String Ensemble 1
    49, // Str Sect 3 -> String Ensemble 2
    45, // Pizzicato -> Pizzicato Strings
    41, // Violin 1 -> Viola
    40, // Violin 2 -> Violin
    42, // Cello 1 -> Cello
    42, // Cello 2 -> Cello
    43, // Contrabass -> Contrabass
    46, // Harp 1 -> Orchestral Harp
    45, // Harp 2 -> Pizzicato Strings
    24, // Guitar 1 -> Acoustic Guitar (nylon)
    25, // Guitar 2 -> Acoustic Guitar (steel)
    28, // Elec Gtr 1 -> Electric Guitar (muted)
    27, // Elec Gtr 2 -> Electric Guitar (clean)
    04, // Sitar -> Sitar
    32, // Acou Bass 1 -> Acoustic Bass
    32, // Acou Bass 2 -> Acoustic Bass
    34, // Elec Bass 1 -> Electric Bass (pick)
    33, // Elec Bass 2 -> Electric Bass (finger)
    36, // Slap Bass 1 -> Slap Bass 1
    37, // Slap Bass 2 -> Slap Bass 2
    35, // Fretless 1 -> Fretless Bass
    35, // Fretless 2 -> Fretless Bass
    79, // Flute 1 -> Ocarina
    73, // Flute 2 -> Flute
    72, // Piccolo 1 -> Piccolo
    72, // Piccolo 2 -> Piccolo
    74, // Recorder -> Recorder
    75, // Pan Pipes -> Pan Flute
    64, // Sax 1 -> Soprano Sax
    65, // Sax 2 -> Alto Sax
    66, // Sax 3 -> Tenor Sax
    67, // Sax 4 -> Baritone Sax
    71, // Clarinet 1 -> Clarinet
    71, // Clarinet 2 -> Clarinet
    68, // Oboe -> Oboe
    69, // Engl Horn -> English Horn
    70, // Bassoon -> Bassoon
    22, // Harmonica -> Harmonica
    56, // Trumpet 1 -> Trumpet
    59, // Trumpet 2 -> Muted Trumpet
    57, // Trombone 1 -> Trombone
    57, // Trombone 2 -> Trombone
    60, // Fr Horn 1 -> French Horn
    60, // Fr Horn 2 -> French Horn
    58, // Tuba -> Tuba
    61, // Brs Sect 1 -> Brass Section
    61, // Brs Sect 2 -> Brass Section
    11, // Vibe 1 -> Vibraphone
    11, // Vibe 2 -> Vibraphone
    98, // Syn Mallet -> FX 3 (crystal)
    14, // Windbell -> Tubular Bells
    9, // Glock -> Glockenspiel
    14, // Tube Bell -> Tubular Bells
    13, // Xylophone -> Xylophone
    12, // Marimba -> Marimba
    107, // Koto -> Koto
    107, // Sho -> Koto
    77, // Shakuhachi -> Shakuhachi
    78, // Whistle 1 -> Whistle
    78, // Whistle 2 -> Whistle
    76, // Bottleblow -> Bottle Blow
    76, // Breathpipe -> Bottle Blow
    47, // Timpani -> Timpani
    117, // Melodic Tom -> Melodic Tom
    127, // Deep Snare -> Gunshot
    118, // Elec Perc 1 -> Synth Drum
    118, // Elec Perc 2 -> Synth Drum
    116, // Taiko -> Taiko Drum
    116, // Taiko Rim -> Taiko Drum
    119, // Cymbal -> Reverse Cymbal
    115, // Castanets -> Woodblock
    112, // Triangle -> Tinkle Bell
    55, // Orche Hit -> Orchestra Hit
    124, // Telephone -> Telephone Ring
    123, // Bird Tweet -> Bird Tweet
    0, // One Note Jam -> Acoustic Grand Piano
    14, // Water Bell -> Tubular Bells
    117, // Jungle Tune -> Melodic Tom
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
