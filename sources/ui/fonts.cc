#include "fonts.h"
#include <algorithm>

const Font_Glyph *Font::glyph(uint32_t ch) const noexcept
{
    size_t index = find_glyph(ch);
    return (index != ~size_t(0)) ? &g_[index] : nullptr;
}

size_t Font::find_glyph(uint32_t ch) const noexcept
{
    size_t index = ~size_t(0);
    if (ch > 0xFFFF)
        return index;

    int flags = flags_;

    bool retry;
    do {
        retry = false;

        Font_Glyph ref;
        ref.unicode = static_cast<uint16_t>(ch);

        const Font_Glyph *first = g_, *last = first + gn_;
        const Font_Glyph *pos = std::lower_bound(first, last, ref);

        if (pos != last && pos->unicode == ch)
            index = std::distance(first, pos);
        else {
            if ((flags & Font::Glyph_Fallback_Upper) && ch >= 'a' && ch <= 'z') {
                ch = ch - 'a' + 'A';
                flags ^= Font::Glyph_Fallback_Upper;
                retry = true;
            }
            else if ((flags & Font::Glyph_Fallback_Lower) && ch >= 'A' && ch <= 'Z') {
                ch = ch - 'A' + 'a';
                flags ^= Font::Glyph_Fallback_Lower;
                retry = true;
            }
        }
    } while (retry);

    return index;
}

uint8_t Font::max_char_width() const noexcept
{
    uint8_t maxsize = maxcw_;
    if (maxsize != 0)
        return maxsize * gw_;
    for (size_t i = 0, n = gn_; i < n; ++i)
        maxsize = std::max(maxsize, g_[i].size);
    maxcw_ = maxsize;
    return maxsize * gw_;
}

namespace Font_Data_S12 {
#include "shinonome12.dat"
}
namespace Font_Data_S14 {
#include "shinonome14.dat"
}
namespace Font_Data_S16 {
#include "shinonome16.dat"
}
namespace Font_Data_S12b {
#include "shinonome12b.dat"
}
namespace Font_Data_S14b {
#include "shinonome14b.dat"
}
namespace Font_Data_S16b {
#include "shinonome16b.dat"
}
namespace Font_Data_FMDSP_Small {
#include "fmdsp_small.dat"
}
namespace Font_Data_FMDSP_Medium {
#include "fmdsp_medium.dat"
}
namespace Font_Data_Digit {
#include "digit.dat"
}

#define DEF_FONT(name, id, ns, flags) Font id(                          \
        name, ns::font_width, ns::font_height,                          \
        ns::font_glyphs, sizeof(ns::font_glyphs) / sizeof(Font_Glyph),  \
        (flags))

DEF_FONT("S12", font_s12, Font_Data_S12, 0);
DEF_FONT("S14", font_s14, Font_Data_S14, 0);
DEF_FONT("S16", font_s16, Font_Data_S16, 0);
DEF_FONT("S12b", font_s12b, Font_Data_S12b, 0);
DEF_FONT("S14b", font_s14b, Font_Data_S14b, 0);
DEF_FONT("S16b", font_s16b, Font_Data_S16b, 0);
DEF_FONT("FM small", font_fmdsp_small, Font_Data_FMDSP_Small, Font::Glyph_Fallback_Upper);
DEF_FONT("FM medium", font_fmdsp_medium, Font_Data_FMDSP_Medium, 0);
DEF_FONT("Digit", font_digit, Font_Data_Digit, 0);
