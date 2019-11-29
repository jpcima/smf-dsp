//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <SDL_pixels.h>

struct Color_Palette
{
    static Color_Palette create_default();

    static Color_Palette &get_current();

    SDL_Color background;

    SDL_Color info_box_background;

    SDL_Color text_browser_foreground;

    SDL_Color metadata_label;
    SDL_Color metadata_value;

    SDL_Color text_min_brightness;
    SDL_Color text_low_brightness;
    SDL_Color text_high_brightness;

    SDL_Color piano_white_key;
    SDL_Color piano_white_shadow;
    SDL_Color piano_black_key;
    SDL_Color piano_pressed_key;

    SDL_Color digit_on;
    SDL_Color digit_off;

    static constexpr SDL_Color transparent() noexcept
        { return SDL_Color{0, 0, 0, 0}; }
};
