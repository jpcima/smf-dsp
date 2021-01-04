//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "player/instrument.h"
#include "synth/synth.h"
#include <fmidi/fmidi.h>
#include <gsl/gsl>
#include <memory>

class Midi_Synth_Instrument : public Midi_Instrument {
public:
    Midi_Synth_Instrument();
    ~Midi_Synth_Instrument();

    void flush_events() override;

    void open_midi_output(gsl::cstring_span id) override;
    void close_midi_output() override;

    bool is_synth() const override { return true; }

    void configure_audio(double audio_rate, double audio_latency);
    void generate_audio(float *output, unsigned nframes);

    void preload(const fmidi_smf_t &smf);

protected:
    void handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
