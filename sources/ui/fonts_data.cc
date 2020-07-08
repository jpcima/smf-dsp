//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fonts.h"

#include "shinonome12.dat"
#include "shinonome14.dat"
//#include "shinonome16.dat"
//#include "shinonome12b.dat"
//#include "shinonome14b.dat"
//#include "shinonome16b.dat"
#include "fmdsp_small.dat"
#include "fmdsp_medium.dat"
#include "digit.dat"

#define F_(...)
#define F_IDENTITY(...) __VA_ARGS__
#define F_GLYPH_DATA(unicode, type, ...) static const type glyph_data_##unicode[] = __VA_ARGS__;
#define F_GLYPH(unicode, type, ...) {0x##unicode, sizeof(type), glyph_data_##unicode},
#define F_WIDTH(x) static const unsigned width = x;
#define F_HEIGHT(x) static const unsigned height = x;
#define F_FONTNS(name, id, font, flags)                             \
    namespace ns_##id {                                             \
        font(F_WIDTH, F_HEIGHT, F_GLYPH_DATA)                       \
        static const Font_Glyph glyphs[] = {font(F_, F_, F_GLYPH)}; \
    }                                                               \
    Font id(name, ns_##id::width, ns_##id::height, ns_##id::glyphs, \
            sizeof(ns_##id::glyphs) / sizeof(Font_Glyph), flags);

F_FONTNS("S12", font_s12, FONT_shinonome12, 0)
F_FONTNS("S14", font_s14, FONT_shinonome14, 0)
//F_FONTNS("S16", font_s16, FONT_shinonome16, 0)
//F_FONTNS("S12b", font_s12b, FONT_shinonome12b, 0)
//F_FONTNS("S14b", font_s14b, FONT_shinonome14b, 0)
//F_FONTNS("S16b", font_s16b, FONT_shinonome16b, 0)
F_FONTNS("FM small", font_fmdsp_small, FONT_fmdsp_small, Font::Glyph_Fallback_Upper)
F_FONTNS("FM medium", font_fmdsp_medium, FONT_fmdsp_medium, 0)
F_FONTNS("Digit", font_digit, FONT_digit, 0)
