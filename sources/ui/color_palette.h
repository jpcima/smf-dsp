//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <SDL_pixels.h>

struct Color_Palette
{
    static const SDL_Color background;

    static const SDL_Color info_box_background;

    static const SDL_Color text_browser_foreground;

    static const SDL_Color metadata_label;
    static const SDL_Color metadata_value;

    static const SDL_Color text_min_brightness;
    static const SDL_Color text_low_brightness;
    static const SDL_Color text_high_brightness;

    static const SDL_Color piano_white_key;
    static const SDL_Color piano_white_shadow;
    static const SDL_Color piano_black_key;
    static const SDL_Color piano_pressed_key;

    static const SDL_Color digit_on;
    static const SDL_Color digit_off;

    static constexpr SDL_Color transparent() noexcept
        { return SDL_Color{0, 0, 0, 0}; }
};
