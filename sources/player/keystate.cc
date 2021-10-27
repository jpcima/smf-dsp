//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "keystate.h"
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
    Keyboard_Midi_Spec resetspec;

    if (len >= 2 && (data[0] >> 4) != 15)
        this->channel[data[0] & 15].handle_message(data, len);
    else if (len >= 1 && data[0] == 0xff)
        clear();
    else if (identify_reset_message(data, len, &resetspec)) {
        clear();
        midispec = resetspec;
    }
}

bool identify_reset_message(const uint8_t *msg, unsigned len, Keyboard_Midi_Spec *spec)
{
    if (len >= 1 && msg[0] == 0xff) { // GM system reset
        if (spec)
            *spec = KMS_GeneralMidi;
        return true;
    }

    if (len >= 4 && msg[0] == 0xf0 && msg[len - 1] == 0xf7) { // sysex resets
        uint8_t manufacturer = msg[1];
        uint8_t device_id = msg[2];
        const uint8_t *payload = msg + 3;
        uint8_t paysize = len - 4;

        switch (manufacturer) {
        case 0x7e: { // GM system messages
            const uint8_t gm1_on[] = {0x09, 0x01};
            const uint8_t gm2_on[] = {0x09, 0x03};
            if (paysize >= 2 && !memcmp(gm1_on, payload, 2)) {
                if (spec)
                    *spec = KMS_GeneralMidi;
                return true;
            }
            if (paysize >= 2 && !memcmp(gm2_on, payload, 2)) {
                if (spec)
                    *spec = KMS_GeneralMidi2;
                return true;
            }
            break;
        }
        case 0x43: { // Yamaha XG
            const uint8_t xg_on[] = {0x4c, 0x00, 0x00, 0x7e};
            const uint8_t all_reset[] = {0x4c, 0x00, 0x00, 0x7f};
            if ((device_id & 0xf0) == 0x10 && paysize >= 4 &&
                (!memcmp(xg_on, payload, 4) || !memcmp(all_reset, payload, 4)))
            {
                if (spec)
                    *spec = KMS_YamahaXG;
                return true;
            }
            break;
        }
        case 0x41: { // Roland GS / Roland Sound Canvas / Roland MT-32
            const uint8_t gs_on[] = {0x42, 0x12, 0x40, 0x00, 0x7f, 0x00};
            const uint8_t sc_mode1_set[] = {0x42, 0x12, 0x00, 0x00, 0x7f, 0x00};
            const uint8_t sc_mode2_set[] = {0x42, 0x12, 0x00, 0x00, 0x7f, 0x01};
            const uint8_t mt32_reset[] = {0x16, 0x12, 0x7f};
            if ((device_id & 0xf0) == 0x10 &&
                ((paysize >= 6 && !memcmp(gs_on, payload, 6)) ||
                 (paysize >= 6 && !memcmp(sc_mode1_set, payload, 6)) ||
                 (paysize >= 6 && !memcmp(sc_mode2_set, payload, 6)) ||
                 (paysize >= 3 && !memcmp(mt32_reset, payload, 3))))
            {
                if (spec)
                    *spec = KMS_RolandGS;
                return true;
            }
            break;
        }
        }
    }

    return false;
}
