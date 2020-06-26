//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player/instruments/synth.h"
#include "synth/synth_host.h"
#include "utility/logs.h"
#include <ring_buffer.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>

static constexpr unsigned midi_buffer_size = 8192;
static constexpr unsigned midi_message_max = 256;
static constexpr unsigned midi_interval_max = 64;

struct Midi_Synth_Instrument::Impl {
    std::unique_ptr<Synth_Host> host_;
    std::unique_ptr<Ring_Buffer> midibuf_;
    double eff_audio_rate_ = 0;
    double eff_audio_latency_ = 0;
    double time_delta_ = 0;

    struct Message_Header {
        unsigned len;
        double timestamp;
        uint8_t flags;
    };

    bool have_next_message_ = false;
    Message_Header next_header_;
    uint8_t next_message_[midi_message_max];
    std::atomic_uint message_count_{0};
    std::atomic_bool messages_initialized_{false};

    std::mutex host_mutex_;

    volatile unsigned cycle_counter_ = 0;

    struct AudioConfig {
        double rate = 0;
        double latency = 0;
    };
    AudioConfig config;

    void process_midi(double time_incr);

    bool extract_next_message();

    void wait_audio_cycle();
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

    unsigned long counter = 0;
    auto interval = std::chrono::milliseconds(1);
    bool warned = false;

    while (impl.message_count_.load() > 0) {
        if (counter > 1000 && !warned) {
            Log::w("Events are taking a long time to flush (%u left)", impl.message_count_.load());
            warned = true;
        }
        std::this_thread::sleep_for(interval);
        ++counter;
    }
}

void Midi_Synth_Instrument::open_midi_output(gsl::cstring_span id)
{
    Impl& impl = *impl_;
    Synth_Host &host = *impl.host_;

    close_midi_output();

    if (id.empty())
        return;

    std::lock_guard<std::mutex> lock(impl.host_mutex_);
    if (!host.load(id, impl.config.rate))
        Log::e("Could not open synth: %s", gsl::to_string(id).c_str());

    impl.messages_initialized_.store(false);
    impl.wait_audio_cycle();

    impl.eff_audio_rate_ = impl.config.rate;
    impl.eff_audio_latency_ = impl.config.latency;
}

void Midi_Synth_Instrument::close_midi_output()
{
    Impl& impl = *impl_;
    Synth_Host &host = *impl.host_;

    std::lock_guard<std::mutex> lock(impl.host_mutex_);
    host.unload();

    impl.messages_initialized_.store(false);
    impl.wait_audio_cycle();
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

void Midi_Synth_Instrument::configure_audio(double audio_rate, double audio_latency)
{
    Impl& impl = *impl_;

    impl.config.rate = audio_rate;
    impl.config.latency = audio_latency;
}

void Midi_Synth_Instrument::generate_audio(float *output, unsigned nframes)
{
    Impl& impl = *impl_;
    Synth_Host &host = *impl.host_;
    double srate = impl.eff_audio_rate_;

    if (!impl.messages_initialized_.exchange(true)) {
        impl.time_delta_ = -impl.eff_audio_latency_;

        Ring_Buffer &midibuf = *impl.midibuf_;
        midibuf.discard(midibuf.size_used());

        impl.have_next_message_ = false;
        impl.message_count_.store(0);
    }

    unsigned frame_index = 0;
    while (frame_index < nframes) {
        unsigned nframes_current = std::min(nframes - frame_index, midi_interval_max);
        impl.process_midi(nframes_current * (1.0 / srate));
        {
            std::unique_lock<std::mutex> lock(impl.host_mutex_, std::try_to_lock);
            if (lock.owns_lock())
                host.generate(&output[2 * frame_index], nframes_current);
            else
                std::memset(&output[2 * frame_index], 0, 2 * nframes_current * sizeof(float));
        }
        frame_index += nframes_current;
    }

    impl.cycle_counter_ += 1;
}

void Midi_Synth_Instrument::Impl::process_midi(double time_incr)
{
    Synth_Host &host = *host_;

    time_delta_ += time_incr;

    while (extract_next_message()) {
        Message_Header hdr = next_header_;

        if (hdr.flags & Midi_Message_Is_First) {
            time_delta_ = time_incr - eff_audio_latency_;
            next_header_.flags = hdr.flags & ~Midi_Message_Is_First;
        }

        if (time_delta_ < hdr.timestamp)
            break;
        time_delta_ -= hdr.timestamp;

        if (hdr.len > 0) {
            std::unique_lock<std::mutex> lock(host_mutex_, std::try_to_lock);
            if (lock.owns_lock())
                host.send_midi(next_message_, hdr.len);
        }

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

void Midi_Synth_Instrument::Impl::wait_audio_cycle()
{
    unsigned cyc1 = cycle_counter_;

    unsigned long counter = 0;
    auto interval = std::chrono::milliseconds(1);
    bool warned = false;

    while (cycle_counter_ - cyc1 < 2) {
        if (counter > 1000 && !warned) {
            Log::w("The audio cycle is taking a long time to complete");
            warned = true;
        }
        std::this_thread::sleep_for(interval);
        ++counter;
    }
}
