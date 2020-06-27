//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <gsl/gsl>
#include <string>
#include <SDL.h>
struct Font;

struct Text_Painter {
    SDL_Renderer *rr = nullptr;
    Font *font = nullptr;
    Point pos;
    SDL_Color fg{0x00, 0x00, 0x00, 0xff};

    void draw_char(uint32_t ch);
    void draw_ucs4(gsl::basic_string_span<const char32_t> str);
    void draw_utf8(gsl::cstring_span str);

    size_t measure_char(uint32_t ch);
    size_t measure_ucs4(gsl::basic_string_span<const char32_t> str);
    size_t measure_utf8(gsl::cstring_span str);

    static void clear_font_caches();
};
