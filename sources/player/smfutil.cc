//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "smfutil.h"
#include <unordered_set>

class SMF_Instrument_Collector {
public:
    explicit SMF_Instrument_Collector(const fmidi_smf_t &smf);
    std::vector<synth_midi_ins> collect();

private:
    void update(unsigned channel);
    const fmidi_event_t *next_event();

private:
    fmidi_seq_u seq_;
    unsigned bank_lsb_[16] = {};
    unsigned bank_msb_[16] = {};
    unsigned program_[16] = {};
    unsigned note_[16] = {~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
                          ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u};
    std::unordered_set<unsigned> set_;
};

///
std::vector<synth_midi_ins> collect_file_instruments(const fmidi_smf_t &smf)
{
    SMF_Instrument_Collector col(smf);
    return col.collect();
}

///
SMF_Instrument_Collector::SMF_Instrument_Collector(const fmidi_smf_t &smf)
    : seq_(fmidi_seq_new(&smf))
{
}

std::vector<synth_midi_ins> SMF_Instrument_Collector::collect()
{
    for (unsigned channel = 0; channel < 16; ++channel)
        update(channel);

    while (const fmidi_event_t *event = next_event()) {
        unsigned channel = event->data[0] & 0x0f;

        switch (event->data[0] & 0xf0) {
        case 0x90: // note on
            if ((event->data[2] & 0x7f) > 0) {
                note_[channel] = event->data[1] & 0x7f;
                update(channel);
            }
            break;
        case 0xb0: // controller change
            switch (event->data[1] & 0x7f) {
            case 0: // bank select MSB
                bank_msb_[channel] = event->data[2] & 0x7f;
                update(channel);
                break;
            case 32: // bank select LSB
                bank_lsb_[channel] = event->data[2] & 0x7f;
                update(channel);
                break;
            }
            break;
        case 0xc0: // program change
            program_[channel] = event->data[1] & 0x7f;
            update(channel);
            break;
        }
    }

    std::vector<synth_midi_ins> list;
    list.reserve(set_.size());
    for (unsigned id : set_) {
        synth_midi_ins ins;
        ins.program = id & 0x7f;
        ins.bank_lsb = (id >> 8) & 0x7f;
        ins.bank_msb = (id >> 16) & 0x7f;
        ins.percussive = id >> 24;
        list.push_back(ins);
    }
    return list;
}

void SMF_Instrument_Collector::update(unsigned channel)
{
    unsigned note = note_[channel];

    // melodic
    for (unsigned msb : {0u, bank_msb_[channel]}) {
        for (unsigned lsb : {0u, bank_lsb_[channel]}) {
            for (unsigned program : {0u, program_[channel]})
                set_.insert((msb << 16) | (lsb << 8) | program);
        }
    }

    // percussive
    if (note != ~0u) {
        for (unsigned msb : {0u, bank_msb_[channel]}) {
            for (unsigned program : {0u, program_[channel]})
                set_.insert((1u << 24) | (program << 16) | (msb << 8) | note);
        }
    }
}

const fmidi_event_t *SMF_Instrument_Collector::next_event()
{
    const fmidi_event_t *evt = nullptr;
    fmidi_seq_event_t seq_evt;
    while (!evt && fmidi_seq_next_event(seq_.get(), &seq_evt)) {
        if (seq_evt.event->type == fmidi_event_message)
            evt = seq_evt.event;
    }
    return evt;
}
