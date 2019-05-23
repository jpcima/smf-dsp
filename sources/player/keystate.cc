#include "keystate.h"
#include <gsl.hpp>
#include <cstring>

void Channel_State::clear()
{
    memset(key, 0, sizeof(key));
    reset_controllers();
    pgm = 0;
    flags = 0;
}

void Channel_State::reset_controllers()
{
    memset(ctl, 0, sizeof(ctl));
    ctl[0x07] = 100;
    ctl[0x0a] = 64;
    ctl[0x0b] = 127;
    ctl[0x2b] = 127;
    bend = 0;
}

void Channel_State::handle_message(const uint8_t *data, unsigned len)
{
    if (len < 1)
        return;

    uint8_t op = data[0] >> 4;

    if (len == 2) {
        switch (op) {
        case 0b1100:
            pgm = data[1] & 127;
            break;
        }
    }
    else if (len == 3) {
        switch (op) {
        case 0b1000:
            key[data[1] & 127] = 0;
            break;
        case 0b1001:
            key[data[1] & 127] = data[2] & 127;
            break;
        case 0b1011: {
            uint8_t id = data[1] & 127;
            if (id < 120)
                ctl[id] = data[2] & 127;
            else if (id == 121)
                reset_controllers();
            else if (id == 120 || id >= 123)
                memset(key, 0, sizeof(key));
            break;
        }
        case 0b1110:
            bend = (data[1] & 127) | ((data[2] & 127) << 7);
            break;
        }
    }
}

void Keyboard_State::clear()
{
    midispec = KMS_GeneralMidi;
    for (unsigned ch = 0; ch < 16; ++ch)
        channel[ch].clear();
    channel[9].percussion = true;
}

void Keyboard_State::handle_message(const uint8_t *data, unsigned len)
{
    static constexpr uint8_t sys_gm_reset[] = {0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
    static constexpr uint8_t sys_gm2_reset[] = {0xf0, 0x7e, 0x7f, 0x09, 0x03, 0xf7};
    static constexpr uint8_t sys_gs_reset[] = {0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};
    static constexpr uint8_t sys_xg_reset[] = {0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7};

    if (len >= 2 && len <= 3 && (data[0] >> 4) != 15)
        this->channel[data[0] & 15].handle_message(data, len);
    else if (len == 1 && data[0] == 0xff)
        clear();
    else if (gsl::span<const uint8_t>(data, len) == gsl::make_span(sys_gm_reset)) {
        clear();
        midispec = KMS_GeneralMidi;
    }
    else if (gsl::span<const uint8_t>(data, len) == gsl::make_span(sys_gm2_reset)) {
        clear();
        midispec = KMS_GeneralMidi2;
    }
    else if (gsl::span<const uint8_t>(data, len) == gsl::make_span(sys_gs_reset)) {
        clear();
        midispec = KMS_RolandGS;
    }
    else if (gsl::span<const uint8_t>(data, len) == gsl::make_span(sys_xg_reset)) {
        clear();
        midispec = KMS_YamahaXG;
    }
}
