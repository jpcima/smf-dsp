//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fonts.h"
#include "utility/charset.h"
#include <algorithm>

const Font_Glyph *Font::glyph(uint32_t ch) const noexcept
{
    size_t index = find_glyph(ch);
    return (index != ~size_t(0)) ? &g_[index] : nullptr;
}

size_t Font::find_glyph(uint32_t ch) const noexcept
{
    size_t index = ~size_t(0);
    int flags = flags_;

    uint32_t candidates[16];
    size_t ncandidates = 0;

    candidates[ncandidates++] = ch;
    if (flags & Font::Glyph_Fallback_Upper)
        candidates[ncandidates++] = unicode_toupper(ch);
    if (flags & Font::Glyph_Fallback_Lower)
        candidates[ncandidates++] = unicode_tolower(ch);

    for (uint32_t nth = 0; nth < ncandidates && index == ~size_t(0); ++nth)
        index = find_exact_glyph(candidates[nth]);

    return index;
}

size_t Font::find_exact_glyph(uint32_t ch) const noexcept
{
    size_t index = ~size_t(0);
    if (ch > 0xFFFF)
        return index;

    Font_Glyph ref;
    ref.unicode = static_cast<uint16_t>(ch);

    const Font_Glyph *first = g_, *last = first + gn_;
    const Font_Glyph *pos = std::lower_bound(first, last, ref);

    if (pos != last && pos->unicode == ch)
        index = std::distance(first, pos);

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
