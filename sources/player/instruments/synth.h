//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "player/instrument.h"
#include "player/adev/adev.h"
#include <ring_buffer.h>
#include <atomic>
#include <memory>
class Synth_Host;

class Midi_Synth_Instrument : public Midi_Instrument {
public:
    Midi_Synth_Instrument();
    ~Midi_Synth_Instrument();

    void flush_events() override;

    void open_midi_output(gsl::cstring_span id) override;
    void close_midi_output() override;

    bool is_synth() const override { return true; }

protected:
    void handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags) override;

private:
    static void audio_callback(float *output, unsigned nframes, void *user_data);
    void process_midi(double time_incr);

    bool extract_next_message();

private:
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

private:
    static constexpr unsigned midi_buffer_size = 8192;
    static constexpr unsigned midi_message_max = 256;
    static constexpr unsigned midi_interval_max = 64;

private:
    bool have_next_message_ = false;
    Message_Header next_header_;
    uint8_t next_message_[midi_message_max];
    std::atomic_uint message_count_ = {0};
};
