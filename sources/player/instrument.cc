//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "instrument.h"
#include "sequences.h"
#include "utility/logs.h"

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
    play_initialization_sequence([this](const uint8_t *msg, unsigned len) {
        send_message(msg, len, 0, 0);
    });
}

void Midi_Instrument::all_sound_off()
{
    play_all_sound_off([this](const uint8_t *msg, unsigned len) {
        send_message(msg, len, 0, 0);
    });
}
