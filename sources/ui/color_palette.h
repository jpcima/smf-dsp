//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "color_palette.dat"
#include <SDL_pixels.h>
#include <gsl.hpp>

struct Color_Palette
{
    Color_Palette() noexcept : color_by_index{} {}

    static Color_Palette create_default();
    static Color_Palette &get_current();

    static constexpr size_t color_count = COLOR_PALETTE_ITEM_COUNT;
    static const gsl::cstring_span color_name[color_count];

    union {
        SDL_Color color_by_index[color_count];
        struct { COLOR_PALETTE_DECLARE_ALL(SDL_Color) };
    };

    static constexpr SDL_Color transparent() noexcept
        { return SDL_Color{0, 0, 0, 0}; }
};
