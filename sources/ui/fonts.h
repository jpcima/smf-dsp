//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
#include <cstddef>

struct Font_Glyph {
    uint16_t unicode;
    uint8_t size;
    const void *data;
};

inline bool operator<(const Font_Glyph &a, const Font_Glyph &b)
{
    return a.unicode < b.unicode;
}

//
struct Font {
public:
    Font(const char *name, uint8_t gw, uint8_t gh, const Font_Glyph g[], size_t gn, int flags)
        : name_(name), gw_(gw), gh_(gh), g_(g), gn_(gn), flags_(flags) {}

    const char *name() const noexcept
        { return name_; }
    uint8_t width() const noexcept
        { return gw_; }
    uint8_t height() const noexcept
        { return gh_; }

    uint8_t max_char_width() const noexcept;
    const Font_Glyph *glyph(uint32_t ch) const noexcept;
    size_t find_glyph(uint32_t ch) const noexcept;
    size_t find_exact_glyph(uint32_t ch) const noexcept;

    size_t glyph_count() const noexcept
        { return gn_; }
    const Font_Glyph &nth_glyph(size_t index) const noexcept
        { return g_[index]; }

    enum Flag {
        Glyph_Fallback_Upper = 1 << 0,
        Glyph_Fallback_Lower = 1 << 1,
    };

    uintptr_t user_data = 0;

private:
    const char *name_ = nullptr;
    uint8_t gw_ = 0;
    uint8_t gh_ = 0;
    const Font_Glyph *g_ = nullptr;
    size_t gn_ = 0;
    int flags_ = 0;
    mutable uint8_t maxcw_ = 0;
};

//
extern Font font_s12;
extern Font font_s14;
extern Font font_s16;
extern Font font_s12b;
extern Font font_s14b;
extern Font font_s16b;
extern Font font_fmdsp_small;
extern Font font_fmdsp_medium;
extern Font font_digit;
