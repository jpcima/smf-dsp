#include "instrument.h"
#include "synth/synth_host.h"
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>

void Midi_Instrument::send_message(const uint8_t *data, unsigned len)
{
    handle_send_message(data, len);
    kbs_.handle_message(data, len);
}

void Midi_Instrument::initialize()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg)); }
        // reset all controllers
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 121, 0 };
            send_message(msg, sizeof(msg)); }
        // bank select
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 0, 0 };
            send_message(msg, sizeof(msg)); }
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 32, 0 };
            send_message(msg, sizeof(msg)); }
        // program change
        { uint8_t msg[] { (uint8_t)((0b1100 << 4) | c), 0 };
            send_message(msg, sizeof(msg)); }
        // pitch bend change
        { uint8_t msg[] { (uint8_t)((0b1110 << 4) | c), 0, 0b1000000 };
            send_message(msg, sizeof(msg)); }
    }
}

void Midi_Instrument::all_sound_off()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg)); }
    }
}

//
void Dummy_Instrument::handle_send_message(const uint8_t *data, unsigned len)
{
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

void Midi_Port_Instrument::handle_send_message(const uint8_t *data, unsigned len)
{
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

    // export port
    if (id.size() > 7 && !memcmp(id.data(), "//port/", 7)) {
        gsl::cstring_span port_id(id.begin() + 7, id.end());
        bool port_found = false;
        for (size_t i = 0, n = out->getPortCount(); i < n && !port_found; ++i) {
            if ((port_found = out->getPortName(i) == port_id))
                out->openPort(i, PROGRAM_DISPLAY_NAME " out");
        }
    }
    else // if (id.size() == 7 && !memcmp(id.data(), "//vport", 7))
        out->openVirtualPort(PROGRAM_DISPLAY_NAME " out");
}

void Midi_Port_Instrument::close_midi_output()
{
    RtMidiOut &out = *out_;
    out.closePort();
}

//
Midi_Synth_Instrument::Midi_Synth_Instrument()
    : host_(new Synth_Host)
{
}

Midi_Synth_Instrument::~Midi_Synth_Instrument()
{
}

void Midi_Synth_Instrument::open_midi_output(gsl::cstring_span id)
{
    Synth_Host &host = *host_;

    close_midi_output();

    if (id.empty())
        return;

    std::unique_ptr<RtAudio> audio(new RtAudio);
    unsigned audio_device = audio->getDefaultOutputDevice();
    RtAudio::DeviceInfo audio_devinfo = audio->getDeviceInfo(audio_device);

    RtAudio::StreamParameters audio_param;
    audio_param.deviceId = audio_device;
    audio_param.nChannels = 2;

    RtAudio::StreamOptions audio_opt;
    audio_opt.streamName = PROGRAM_DISPLAY_NAME " synth";

    const double audio_latency = 20e-3;
    unsigned audio_buffer_size = (unsigned)std::ceil(audio_latency * audio_devinfo.preferredSampleRate);

    audio->openStream(&audio_param, nullptr, RTAUDIO_FLOAT32, audio_devinfo.preferredSampleRate, &audio_buffer_size, &audio_callback, this, &audio_opt);

    if (!host.load(id, audio_devinfo.preferredSampleRate))
        return;

    midibuf_.reset(new Ring_Buffer(midi_buffer_size));
    audio->startStream();
    audio_ = std::move(audio);
}

void Midi_Synth_Instrument::close_midi_output()
{
    audio_.reset();
    midibuf_.reset();
    host_->unload();
}

void Midi_Synth_Instrument::handle_send_message(const uint8_t *data, unsigned len)
{
    Ring_Buffer *midibuf = midibuf_.get();
    size_t size_need = sizeof(unsigned) + sizeof(len);

    if (!midibuf || size_need > midibuf->capacity())
        return;

    while (midibuf->size_free() < size_need)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    midibuf->put(len);
    midibuf->put(data, len);
}

int Midi_Synth_Instrument::audio_callback(void *output_buffer, void *, unsigned int nframes, double, RtAudioStreamStatus, void *user_data)
{
    Midi_Synth_Instrument *self = (Midi_Synth_Instrument *)user_data;
    Synth_Host &host = *self->host_;

    // maximum interval between midi processing cycles
    constexpr unsigned midi_interval_max = 64;

    float *frame_buffer = (float *)output_buffer;
    unsigned frame_index = 0;

    while (frame_index < nframes) {
        self->process_midi();
        unsigned nframes_current = std::min(nframes, midi_interval_max);
        host.generate(&frame_buffer[2 * frame_index], nframes_current);
        frame_index += nframes_current;
    }

    return 0;
}

void Midi_Synth_Instrument::process_midi()
{
    Synth_Host &host = *host_;
    Ring_Buffer *midibuf = midibuf_.get();

    if (!midibuf)
        return;

    unsigned len = 0;
    while (midibuf->peek(len) && midibuf->size_used() >= sizeof(len) + len) {
        midibuf->discard(sizeof(len));
        uint8_t msg[midi_message_max];
        if (len > sizeof(msg))
            midibuf->discard(len);
        else {
            midibuf->get(msg, len);
            host.send_midi(msg, len);
        }
    }
}
