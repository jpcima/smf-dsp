//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "../synth.h"
#include "../synth_utility.h"
#include "utility/charset.h"
#include "utility/paths.h"
#include "utility/logs.h"
#include <timiditypp/timidity.h>
#include <timiditypp/instrum.h>
#include <timiditypp/playmidi.h>
#include <memory>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

void ZMusic_Print(int type, const char *msg, va_list args)
{
    if (type >= 100)
        Log::ve(msg, args);
    else if (type >= 50)
        Log::vw(msg, args);
    else if (type >= 10)
        Log::vi(msg, args);
}

static void timiditypp_print_message(int type, int verbosity_level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (verbosity_level >= TimidityPlus::VERB_NOISY)
        return;

    switch (type) {
    default:
    case TimidityPlus::CMSG_INFO:
        Log::vi(fmt, ap);
        break;
    case TimidityPlus::CMSG_WARNING:
        Log::vw(fmt, ap);
        break;
    case TimidityPlus::CMSG_ERROR:
        Log::ve(fmt, ap);
        break;
    }

    va_end(ap);
}

///
struct timiditypp_synth_object {
    string_list_ptr soundfonts;
    std::unique_ptr<MusicIO::FileSystemSoundFontReader> reader;
    std::unique_ptr<TimidityPlus::Instruments> instruments;
    std::unique_ptr<TimidityPlus::Player> player;
};

static std::string timiditypp_synth_base_dir;

static void timiditypp_plugin_init(const char *base_dir)
{
    timiditypp_synth_base_dir.assign(base_dir);
    TimidityPlus::printMessage = timiditypp_print_message;
}

static void timiditypp_plugin_shutdown()
{
}

static const char *default_soundfont_value[] = {"A320U.sf2", nullptr};

static const synth_option the_synth_options[] = {
    {"soundfont", "List of SoundFont files to load", 'm', {.m = default_soundfont_value}},
};

static const synth_option *timiditypp_plugin_option(size_t index)
{
    size_t count = sizeof(the_synth_options) / sizeof(the_synth_options[0]);
    return (index < count) ? &the_synth_options[index] : nullptr;
}

static synth_object *timiditypp_synth_instantiate(double srate)
{
    std::unique_ptr<timiditypp_synth_object> obj(new timiditypp_synth_object);
    if (!obj)
        return nullptr;

    obj->soundfonts.reset(new char *[1]());

    TimidityPlus::set_playback_rate(srate);

    return (synth_object *)obj.release();
}

static void timiditypp_synth_cleanup(synth_object *obj)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;
    delete sy;
}

static int timiditypp_synth_activate(synth_object *obj)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;

    TimidityPlus::Instruments *instruments = new TimidityPlus::Instruments;
    sy->instruments.reset(instruments);

    ///
    class Reader : public MusicIO::FileSystemSoundFontReader {
    public:
        explicit Reader(timiditypp_synth_object &sy)
            : FileSystemSoundFontReader("timidity.cfg")
        {
            std::string &config = config_;
            config.reserve(1024);

            for (char **p = sy.soundfonts.get(), *sf; (sf = *p); ++p) {
                std::string sf_absolute;
                if (!is_path_absolute(sf)) {
                    sf_absolute = timiditypp_synth_base_dir + sf;
                    sf = (char *)sf_absolute.c_str();
                }
                config.append("soundfont \"");
                config.append(sf);
                config.append("\"\n");
            }
        }

        MusicIO::FileInterface *open_file(const char *fn) override
        {
            if (!fn)
                return new MusicIO::MemoryReader(
                    (const uint8_t *)config_.c_str(), (long)config_.size());
            else
                return FileSystemSoundFontReader::open_file(fn);
        }

        void close() override
        {
        }

    private:
        std::string config_;
    };

    ///
    Reader *reader = new Reader(*sy);
    sy->reader.reset(reader);

    if (!instruments->load(reader))
        Log::e("[timidity++] cannot load the soundfont configuration");

    ///
    TimidityPlus::Player *player = new TimidityPlus::Player(instruments);
    sy->player.reset(player);

    ///
    player->playmidi_stream_init();

    return 0;
}

static void timiditypp_synth_deactivate(synth_object *obj)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;

    sy->player.reset();
    sy->instruments.reset();
    sy->reader.reset();
}

static void timiditypp_synth_write(synth_object *obj, const unsigned char *msg, size_t size)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;
    TimidityPlus::Player &player = *sy->player;

    uint8_t status = 0;
    uint8_t data1 = 0;
    uint8_t data2 = 0;

    switch (size) {
    case 3:
        data2 = msg[2]; // fall through
    case 2:
        data1 = msg[1]; // fall through
    case 1:
        status = msg[0];
        player.send_event(status, data1, data2);
        break;
    case 0:
        break;
    default:
        player.send_long_event(msg, size);
        break;
    }
}

static void timiditypp_synth_generate(synth_object *obj, float *frames, size_t nframes)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;
    TimidityPlus::Player &player = *sy->player;

    player.compute_data(frames, nframes);
}

static void timiditypp_synth_set_option(synth_object *obj, const char *name, synth_value value)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;

    if (!strcmp(name, "soundfont"))
        sy->soundfonts = string_list_dup(value.m);
}

static void timiditypp_synth_preload(synth_object *obj, const synth_midi_ins *ins, size_t count)
{
    timiditypp_synth_object *sy = (timiditypp_synth_object *)obj;
    TimidityPlus::Instruments *instruments = sy->instruments.get();

    if (!instruments)
        return;

    std::unique_ptr<uint16_t[]> ids(new uint16_t[count]);
    size_t ids_count = 0;

    for (size_t i = 0; i < count; ++i) {
        synth_midi_ins in = ins[i];
        uint16_t id = (bool(in.percussive) << 14) | ((in.bank_msb & 127) << 7) | (in.program & 127);
        ids[ids_count++] = id;
    }

    instruments->PrecacheInstruments(ids.get(), ids_count);
}

static const synth_interface the_synth_interface = {
    SYNTH_ABI_VERSION,
    "TiMidity++",
    &timiditypp_plugin_init,
    &timiditypp_plugin_shutdown,
    &timiditypp_plugin_option,
    &timiditypp_synth_instantiate,
    &timiditypp_synth_cleanup,
    &timiditypp_synth_activate,
    &timiditypp_synth_deactivate,
    &timiditypp_synth_write,
    &timiditypp_synth_generate,
    &timiditypp_synth_set_option,
    &timiditypp_synth_preload,
};

extern "C" SYNTH_EXPORT const synth_interface *synth_plugin_entry()
{
    return &the_synth_interface;
}
