//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fonts.h"
#include "utility/charset.h"
#include <unicode/unorm2.h>
#include <unicode/uiter.h>
#include <algorithm>

const Font_Glyph *Font::glyph(uint32_t ch) const noexcept
{
    size_t index = find_glyph(ch);
    return (index != ~size_t(0)) ? &g_[index] : nullptr;
}

static uint32_t decomposition_lead_char(const UNormalizer2 *norm, uint32_t ch)
{
    UChar dec[8];
    UErrorCode status = U_ZERO_ERROR;
    int32_t declen = unorm2_getDecomposition(
        norm, ch, dec, sizeof(dec) / sizeof(dec[0]), &status);
    UChar32 dc = U_SENTINEL;
    if (declen > 0) {
        UCharIterator iter;
        uiter_setString(&iter, dec, declen);
        dc = uiter_current32(&iter);
    }
    return (dc != U_SENTINEL) ? dc : 0;
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

    // try the character decomposition
    if (index == ~size_t(0)) {
        static const UNormalizer2 *norm =
            []() -> const UNormalizer2 * {
                UErrorCode status = U_ZERO_ERROR;
                const UNormalizer2 *norm = unorm2_getNFDInstance(&status);
                return U_SUCCESS(status) ? norm : nullptr;
            }();
        if (norm) {
            for (uint32_t nth = 0; nth < ncandidates && index == ~size_t(0); ++nth) {
                uint32_t dc = decomposition_lead_char(norm, candidates[nth]);
                if (dc != 0)
                    index = find_exact_glyph(dc);
            }
        }
    }

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
