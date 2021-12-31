//          Copyright Jean Pierre Cimalando 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>

template <class F>
void play_initialization_sequence(F &&play)
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            play(msg, sizeof(msg)); }
        // reset all controllers
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 121, 0 };
            play(msg, sizeof(msg)); }
        // volume
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 7, 100 };
            play(msg, sizeof(msg)); }
        // pan
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 10, 64 };
            play(msg, sizeof(msg)); }
        // bank select
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 0, 0 };
            play(msg, sizeof(msg)); }
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 32, 0 };
            play(msg, sizeof(msg)); }
        // program change
        { uint8_t msg[] { (uint8_t)((0b1100 << 4) | c), 0 };
            play(msg, sizeof(msg)); }
    }
}

template <class F>
void play_all_sound_off(F &&play)
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            play(msg, sizeof(msg)); }
    }
}
