//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "color_palette.h"
#include <cstdio>

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

///
static int hex_digit_from_char(char c)
{
    return (c >= '0' && c <= '9') ? (c - '0') :
        (c >= 'a' && c <= 'z') ? (c - 'a' + 10) :
        (c >= 'A' && c <= 'Z') ? (c - 'A' + 10) : -1;
}

static bool color_from_hex(gsl::cstring_span hex, SDL_Color &color)
{
    if (hex.empty() || hex[0] != '#')
        return false;

    hex = hex.subspan(1);
    size_t length = hex.size();

    uint32_t rgba = 0;
    if (length == 6 || length == 8) {
        for (size_t i = 0; i < length; ++i) {
            int d = hex_digit_from_char(hex[i]);
            if (d == -1)
                return false;
            rgba = (rgba << 4) | d;
        }
    }
    if (length == 6)
        rgba = (rgba << 8) | 0xff;

    color.r = rgba >> 24;
    color.g = (rgba >> 16) & 0xff;
    color.b = (rgba >> 8) & 0xff;
    color.a = rgba & 0xff;

    return true;
}

static std::string hex_from_color(SDL_Color color)
{
    char rgb_string[16];
    if (color.a == 0xff)
        sprintf(rgb_string, "#%02X%02X%02X", color.r, color.g, color.b);
    else
        sprintf(rgb_string, "#%02X%02X%02X%02X", color.r, color.g, color.b, color.a);
    return rgb_string;
}

///
bool Color_Palette::load(CSimpleIniA &ini, const char *section)
{
    const CSimpleIniA::TKeyVal *dict = ini.GetSection(section);

    if (!dict)
        return false;

    bool is_color_set[color_count] = {};
    size_t num_colors_set = 0;

    CSimpleIniA::TKeyVal::const_iterator it = dict->begin(), end = dict->end();
    for (; it != end; ++it) {
        gsl::cstring_span key = it->first.pItem;
        gsl::cstring_span value = it->second;

        unsigned index = ~0u;
        for (unsigned i = 0; i < color_count && index == ~0u; ++i) {
            if (key == color_name[i])
                index = i;
        }

        if (index == ~0u) {
            fprintf(stderr, "Colors: unknown color entry \"%s\"\n", key.data());
            continue;
        }

        if (!color_from_hex(value, color_by_index[index])) {
            fprintf(stderr, "Colors: cannot parse hexadecimal RGBA.\n");
            continue;
        }

        if (!is_color_set[index]) {
            is_color_set[index] = true;
            ++num_colors_set;
        }
    }

    if (num_colors_set != color_count)
        fprintf(stderr, "Colors: the color palette is incomplete.\n");

    return true;
}

void Color_Palette::save(CSimpleIniA &ini, const char *section) const
{
    for (unsigned index = 0; index < color_count; ++index) {
        SDL_Color color = color_by_index[index];
        std::string value = hex_from_color(color);
        ini.SetValue(section, color_name[index].data(), value.c_str());
    }
}
