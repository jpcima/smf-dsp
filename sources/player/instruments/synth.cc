//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player/instruments/synth.h"
#include "player/adev/adev.h"
#include "synth/synth_host.h"
#include "configuration.h"
#include "utility/logs.h"
#include <ring_buffer.h>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>

static constexpr unsigned midi_buffer_size = 8192;
static constexpr unsigned midi_message_max = 256;
static constexpr unsigned midi_interval_max = 64;

struct Midi_Synth_Instrument::Impl {
    std::unique_ptr<Synth_Host> host_;
    std::unique_ptr<Ring_Buffer> midibuf_;
    std::unique_ptr<Audio_Device> audio_;
    double audio_rate_ = 0;
    double audio_latency_ = 0;
    double time_delta_ = 0;

    struct Message_Header {
        unsigned len;
        double timestamp;
        uint8_t flags;
    };

    bool have_next_message_ = false;
    Message_Header next_header_;
    uint8_t next_message_[midi_message_max];
    std::atomic_uint message_count_ = {0};

    static void audio_callback(float *output, unsigned nframes, void *user_data);
    void process_midi(double time_incr);

    bool extract_next_message();
};

Midi_Synth_Instrument::Midi_Synth_Instrument()
    : impl_(new Impl)
{
    impl_->host_.reset(new Synth_Host);
    impl_->midibuf_.reset(new Ring_Buffer(midi_buffer_size));
}

Midi_Synth_Instrument::~Midi_Synth_Instrument()
{
}

void Midi_Synth_Instrument::flush_events()
{
    Impl& impl = *impl_;

    while (impl.message_count_.load() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Midi_Synth_Instrument::open_midi_output(gsl::cstring_span id)
{
    Impl& impl = *impl_;
    Synth_Host &host = *impl.host_;

    close_midi_output();

    if (id.empty())
        return;

    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini) ini = create_configuration();

    double desired_latency = ini->GetDoubleValue("", "synth-audio-latency", 50);
    desired_latency = 1e-3 * std::max(1.0, std::min(500.0, desired_latency));

    std::unique_ptr<Audio_Device> audio(Audio_Device::create_best_for_system());
    if (!audio->init(desired_latency))
        return;

    audio->set_callback(&Impl::audio_callback, this);

    const double audio_rate = audio->sample_rate();
    const double audio_latency = audio->latency();

    Log::i("Audio latency: %f ms", 1e3 * audio_latency);

    if (!host.load(id, audio_rate))
        Log::e("Could not open synth: %s", gsl::to_string(id).c_str());

    impl.audio_rate_ = audio_rate;
    impl.audio_latency_ = audio_latency;
    impl.audio_ = std::move(audio);

    impl.time_delta_ = -audio_latency;

    impl.audio_->start();
}

void Midi_Synth_Instrument::close_midi_output()
{
    Impl& impl = *impl_;

    impl.audio_.reset();
    impl.host_->unload();

    Ring_Buffer &midibuf = *impl.midibuf_;
    midibuf.discard(midibuf.size_used());

    impl.have_next_message_ = false;
    impl.message_count_.store(0);
}

void Midi_Synth_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    Impl& impl = *impl_;
    Ring_Buffer &midibuf = *impl.midibuf_;

    if (len > midi_message_max)
        len = 0; // too large, only write header

    Impl::Message_Header hdr{len, ts, flags};
    size_t size_need = sizeof(hdr) + len;

    while (midibuf.size_free() < size_need)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    impl.message_count_.fetch_add(1);
    midibuf.put(hdr);
    midibuf.put(data, len);
}

void Midi_Synth_Instrument::Impl::audio_callback(float *output, unsigned nframes, void *user_data)
{
    Midi_Synth_Instrument *self = (Midi_Synth_Instrument *)user_data;
    Impl& impl = *self->impl_;
    Synth_Host &host = *impl.host_;
    double srate = impl.audio_rate_;

    unsigned frame_index = 0;
    while (frame_index < nframes) {
        unsigned nframes_current = std::min(nframes - frame_index, midi_interval_max);
        impl.process_midi(nframes_current * (1.0 / srate));
        host.generate(&output[2 * frame_index], nframes_current);
        frame_index += nframes_current;
    }
}

void Midi_Synth_Instrument::Impl::process_midi(double time_incr)
{
    Synth_Host &host = *host_;

    time_delta_ += time_incr;

    while (extract_next_message()) {
        Message_Header hdr = next_header_;

        if (hdr.flags & Midi_Message_Is_First) {
            time_delta_ = time_incr - audio_latency_;
            next_header_.flags = hdr.flags & ~Midi_Message_Is_First;
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

bool Midi_Synth_Instrument::Impl::extract_next_message()
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
