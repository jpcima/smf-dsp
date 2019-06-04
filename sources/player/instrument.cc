//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "instrument.h"
#include "config.h"
#include "synth/synth_host.h"
#include <RtMidi.h>
#if !defined(__HAIKU__)
#include <RtAudio.h>
#else
#include <MediaKit.h>
#endif
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>

Midi_Instrument::Midi_Instrument()
{
    kbs_.clear();
}

void Midi_Instrument::send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    handle_send_message(data, len, ts, flags);
    kbs_.handle_message(data, len);
}

void Midi_Instrument::initialize()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // reset all controllers
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 121, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // bank select
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 0, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 32, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // program change
        { uint8_t msg[] { (uint8_t)((0b1100 << 4) | c), 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // pitch bend change
        { uint8_t msg[] { (uint8_t)((0b1110 << 4) | c), 0, 0b1000000 };
            send_message(msg, sizeof(msg), 0, 0); }
    }
}

void Midi_Instrument::all_sound_off()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
    }
}

//
void Dummy_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    (void)ts;
    (void)flags;

    if (false) {
        for (unsigned i = 0; i < len; ++i)
            fprintf(stderr, "%s%02X", i ? " " : "", data[i]);
        fputc('\n', stderr);
    }
}

//
Midi_Port_Instrument::Midi_Port_Instrument()
{
    open_midi_output(gsl::cstring_span());
}

Midi_Port_Instrument::~Midi_Port_Instrument()
{
}

void Midi_Port_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    (void)ts;
    (void)flags;

    RtMidiOut &out = *out_;
    out.sendMessage(data, len);
}

std::vector<Midi_Output> Midi_Port_Instrument::get_midi_outputs()
{
    std::vector<Midi_Output> ports;

    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);

    for (RtMidi::Api api : apis) {
        if (api == RtMidi::RTMIDI_DUMMY)
            continue;

        std::string api_name = RtMidi::getApiName(api);
        bool has_virtual_port = api != RtMidi::WINDOWS_MM;

        RtMidiOut out(api);

        if (has_virtual_port) {
            Midi_Output port;
            port.id = api_name + "://vport";
            port.name = api_name + ": Virtual port";
            ports.push_back(std::move(port));
        }

        for (size_t i = 0, n = out.getPortCount(); i < n; ++i) {
            std::string port_name = out.getPortName(i);
            if (port_name.empty())
                continue;
            Midi_Output port;
            port.id = api_name + "://port/" + port_name;
            port.name = api_name + ": " + port_name;
            ports.push_back(std::move(port));
        }
    }

    return ports;
}

void Midi_Port_Instrument::open_midi_output(gsl::cstring_span id)
{
    RtMidiOut *out = out_.get();
    if (out) {
        initialize();
        out_.reset();
    }

    // extract API name
    RtMidi::Api api = RtMidi::UNSPECIFIED;
    auto pos = std::find(id.begin(), id.end(), ':');
    if (pos != id.end()) {
        std::string api_name(id.begin(), pos);
        api = RtMidi::getCompiledApiByName(api_name);
        id = gsl::cstring_span(pos + 1, id.end());
    }

    // open client
    out = new RtMidiOut(api, PROGRAM_DISPLAY_NAME);
    out_.reset(out);
    api = out->getCurrentApi();

    if (id.empty())
        return;

    // export port
    if (id.size() > 7 && !memcmp(id.data(), "//port/", 7)) {
        gsl::cstring_span port_id(id.begin() + 7, id.end());
        bool port_found = false;
        for (size_t i = 0, n = out->getPortCount(); i < n && !port_found; ++i) {
            if ((port_found = out->getPortName(i) == port_id))
                out->openPort(i, PROGRAM_DISPLAY_NAME " out");
        }
    }
    else if (id.size() == 7 && !memcmp(id.data(), "//vport", 7))
        out->openVirtualPort(PROGRAM_DISPLAY_NAME " out");
}

void Midi_Port_Instrument::close_midi_output()
{
    RtMidiOut &out = *out_;
    out.closePort();
}

//
Midi_Synth_Instrument::Midi_Synth_Instrument()
    : host_(new Synth_Host),
      midibuf_(new Ring_Buffer(midi_buffer_size))
{
}

Midi_Synth_Instrument::~Midi_Synth_Instrument()
{
}

void Midi_Synth_Instrument::flush_events()
{
    while (message_count_.load() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Midi_Synth_Instrument::open_midi_output(gsl::cstring_span id)
{
    Synth_Host &host = *host_;

    close_midi_output();

    if (id.empty())
        return;

    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini) ini = create_configuration();

#if !defined(__HAIKU__)
    std::unique_ptr<RtAudio> audio(new RtAudio);
    unsigned audio_device = audio->getDefaultOutputDevice();
    RtAudio::DeviceInfo audio_devinfo = audio->getDeviceInfo(audio_device);

    fprintf(stderr, "Audio interface: %s\n", audio->getApiDisplayName(audio->getCurrentApi()).c_str());

    RtAudio::StreamParameters audio_param;
    audio_param.deviceId = audio_device;
    audio_param.nChannels = 2;

    RtAudio::StreamOptions audio_opt;
    audio_opt.streamName = PROGRAM_DISPLAY_NAME " synth";
    audio_opt.flags = RTAUDIO_ALSA_USE_DEFAULT;

    double audio_latency = ini->GetDoubleValue("", "synth-audio-latency", 50);
    audio_latency = 1e-3 * std::max(1.0, std::min(500.0, audio_latency));

    double audio_rate = audio_devinfo.preferredSampleRate;
    unsigned audio_buffer_size = (unsigned)std::ceil(audio_latency * audio_rate);

    audio->openStream(&audio_param, nullptr, RTAUDIO_FLOAT32, audio_rate, &audio_buffer_size, &audio_callback, this, &audio_opt);

    audio_latency = audio_buffer_size / audio_rate;
#else
    media_raw_audio_format format = media_raw_audio_format::wildcard;
    format.channel_count = 2;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_HOST_ENDIAN;
    //NOTE: latency setting not used

    std::unique_ptr<BSoundPlayer> audio(
        new BSoundPlayer(&format, PROGRAM_DISPLAY_NAME " synth", &audio_callback, nullptr, this));

    if (audio->InitCheck() != B_OK)
        return;

    format = audio->Format();
    double audio_rate = format.frame_rate;
    double audio_latency = format.buffer_size / audio_rate;
#endif
    fprintf(stderr, "Audio latency: %f ms\n", 1e3 * audio_latency);

    if (!host.load(id, audio_rate))
        return;

    audio_rate_ = audio_rate;
    audio_latency_ = audio_latency;

    audio_ = std::move(audio);

#if !defined(__HAIKU__)
    audio_->startStream();
#else
    if (audio_->Start() != B_OK) {
        audio_.reset();
        return;
    }
    audio_->SetHasData(true);
#endif
}

void Midi_Synth_Instrument::close_midi_output()
{
    audio_.reset();
    host_->unload();

    Ring_Buffer &midibuf = *midibuf_;
    midibuf.discard(midibuf.size_used());

    time_delta_ = -audio_latency_;
    have_next_message_ = false;
    message_count_.store(0);
}

void Midi_Synth_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    Ring_Buffer &midibuf = *midibuf_;

    if (len > midi_message_max)
        len = 0; // too large, only write header

    Message_Header hdr{len, ts, flags};
    size_t size_need = sizeof(hdr) + len;

    while (midibuf.size_free() < size_need)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    message_count_.fetch_add(1);
    midibuf.put(hdr);
    midibuf.put(data, len);
}

#if !defined(__HAIKU__)
int Midi_Synth_Instrument::audio_callback(void *output_buffer, void *, unsigned nframes, double, RtAudioStreamStatus, void *user_data)
#else
void Midi_Synth_Instrument::audio_callback(void *user_data, void *output_buffer, size_t size, const media_raw_audio_format &)
#endif
{
    Midi_Synth_Instrument *self = (Midi_Synth_Instrument *)user_data;
    Synth_Host &host = *self->host_;
    double srate = self->audio_rate_;

#if defined(__HAIKU__)
    unsigned nframes = size / (2 * sizeof(float));
#endif

    float *frame_buffer = (float *)output_buffer;
    unsigned frame_index = 0;

    while (frame_index < nframes) {
        unsigned nframes_current = std::min(nframes - frame_index, midi_interval_max);
        self->time_delta_ += nframes_current * (1.0 / srate);
        self->process_midi();
        host.generate(&frame_buffer[2 * frame_index], nframes_current);
        frame_index += nframes_current;
    }

#if !defined(__HAIKU__)
    return 0;
#endif
}

void Midi_Synth_Instrument::process_midi()
{
    Synth_Host &host = *host_;
    double audio_latency = audio_latency_;

    while (extract_next_message()) {
        Message_Header hdr = next_header_;

        if (hdr.flags & Midi_Message_Is_First) {
            time_delta_ = -audio_latency;
            next_header_.flags = hdr.flags & ~Midi_Message_Is_First;
            break;
        }

        if (time_delta_ < hdr.timestamp)
            break;
        time_delta_ -= hdr.timestamp;

        if (hdr.len > 0)
            host.send_midi(next_message_, hdr.len);

        have_next_message_ = false;
        message_count_.fetch_sub(1);
    }
}

bool Midi_Synth_Instrument::extract_next_message()
{
    if (have_next_message_)
        return true;

    Ring_Buffer &midibuf = *midibuf_;
    Message_Header hdr;

    bool have = midibuf.peek(hdr) && midibuf.size_used() >= sizeof(hdr) + hdr.len;
    if (!have)
        return false;

    have_next_message_ = true;
    next_header_ = hdr;
    midibuf.discard(sizeof(hdr));
    midibuf.get(next_message_, hdr.len);
    return true;
}

constexpr unsigned Midi_Synth_Instrument::midi_buffer_size;
constexpr unsigned Midi_Synth_Instrument::midi_message_max;
constexpr unsigned Midi_Synth_Instrument::midi_interval_max;
