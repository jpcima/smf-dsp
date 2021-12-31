//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <nonstd/string_view.hpp>
#include <string>
struct Font;

struct Text_Painter {
    SDL_Renderer *rr = nullptr;
    Font *font = nullptr;
    Point pos;
    SDL_Color fg{0x00, 0x00, 0x00, 0xff};

    void draw_char(uint32_t ch);
    void draw_ucs4(nonstd::u32string_view str);
    void draw_utf8(nonstd::string_view str);

    size_t measure_char(uint32_t ch);
    size_t measure_ucs4(nonstd::u32string_view str);
    size_t measure_utf8(nonstd::string_view str);

    static void clear_font_caches();
};
