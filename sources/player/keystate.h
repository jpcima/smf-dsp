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
