//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "color_palette.h"

constexpr size_t Color_Palette::color_count;
const gsl::cstring_span Color_Palette::color_name[color_count] = { COLOR_PALETTE_NAME_ALL };

Color_Palette Color_Palette::create_default()
{
    Color_Palette pal;

    pal.background = {0x00, 0x00, 0x00, 0xff};

    pal.info_box_background = {0x11, 0x33, 0x55, 0xff};

    pal.text_browser_foreground = {0x77, 0xaa, 0xff, 0xff};

    pal.metadata_label = {0x33, 0x77, 0xbb, 0xff};
    pal.metadata_value = {0x77, 0xaa, 0xff, 0xff};

    pal.text_min_brightness = {0x11, 0x33, 0x55, 0xff};
    pal.text_low_brightness = {0x33, 0x77, 0xbb, 0xff};
    pal.text_high_brightness = {0x77, 0xaa, 0xff, 0xff};

    pal.piano_white_key = {0xdd, 0xdd, 0xdd, 0xff};
    pal.piano_white_shadow = {0x77, 0x77, 0x77, 0xff};
    pal.piano_black_key = {0x00, 0x00, 0x00, 0xff};
    pal.piano_pressed_key = {0xff, 0x00, 0x00, 0xff};

    pal.digit_on = {0x99, 0xaa, 0xff, 0xff};
    pal.digit_off = {0x66, 0x66, 0x66, 0xff};

    return pal;
}

Color_Palette &Color_Palette::get_current()
{
    static Color_Palette palette = create_default();
    return palette;
}
