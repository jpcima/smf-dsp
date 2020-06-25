//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "instrument.h"
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
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // reset all controllers
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 121, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // volume
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 7, 100 };
            send_message(msg, sizeof(msg), 0, 0); }
        // pan
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 10, 64 };
            send_message(msg, sizeof(msg), 0, 0); }
        // bank select
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 0, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 32, 0 };
            send_message(msg, sizeof(msg), 0, 0); }
        // program change
        { uint8_t msg[] { (uint8_t)((0b1100 << 4) | c), 0 };
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
