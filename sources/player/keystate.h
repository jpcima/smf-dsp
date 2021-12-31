//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>

struct Channel_State {
    uint8_t key[128] = {};
    uint8_t ctl[128] = {};
    uint16_t bend = 0;
    uint8_t pgm = 0;
    union {
        unsigned flags = 0;
        struct {
            unsigned percussion : 1;
        };
    };
    void clear();
    void reset_controllers();
    void handle_message(const uint8_t *data, unsigned len);
};

enum Keyboard_Midi_Spec {
    KMS_GeneralMidi,
    KMS_GeneralMidi2,
    KMS_YamahaXG,
    KMS_RolandGS,
};

struct Keyboard_State {
    Keyboard_Midi_Spec midispec = KMS_GeneralMidi;
    Channel_State channel[16];
    Keyboard_State() noexcept { channel[9].percussion = true; }
    void clear();
    void handle_message(const uint8_t *data, unsigned len);
};

bool identify_reset_message(const uint8_t *msg, unsigned len, Keyboard_Midi_Spec *spec = nullptr);
