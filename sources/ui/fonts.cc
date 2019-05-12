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
