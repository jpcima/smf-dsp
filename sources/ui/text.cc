//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "text.h"
#include "fonts.h"
#include "utility/SDL++.h"
#include <utf/utf.hpp>
#include <nonstd/scope.hpp>
#include <vector>
#include <stdexcept>

struct Font_Atlas {
    static Point size(const Font &font);
    static Rect coord(const Font &font, size_t index);
    static SDL_Texture *create(SDL_Renderer *rr, const Font &font);
};

struct Font_Cache_Entry {
    SDL_Renderer *rr = nullptr;
    SDL_Texture *tx = nullptr;
};

static std::vector<Font_Cache_Entry> font_cache;

void Text_Painter::draw_char(uint32_t ch)
{
    size_t font_cache_index = font->user_data;
    if (font_cache_index == 0) {
        font_cache.emplace_back();
        font->user_data = font_cache_index = font_cache.size();
    }
    --font_cache_index;

    Font_Cache_Entry &font_cache_entry = font_cache[font_cache_index];
    SDL_Texture *atlas = font_cache_entry.tx;

    if (atlas && font_cache_entry.rr != rr) {
        // if renderer was switched, ensure font caches were cleared first
        throw std::runtime_error("font cache renderer mismatch");
    }

    if (!atlas) {
        atlas = Font_Atlas::create(rr, *font);
        font_cache_entry.tx = atlas;
        font_cache_entry.rr = rr;
    }

    const Font &font = *this->font;
    size_t gi = font.find_glyph(ch);

    if (gi == ~size_t(0))
        pos.x += font.width();
    else {
        Rect gr = Font_Atlas::coord(font, gi);

        Rect clip;
        SDL_RenderGetClipRect(rr, &clip);

        // avoid drawing partial width characters
        if (!(clip.w && clip.h) || pos.x + gr.w <= clip.x + clip.w) {
            SDL_SetTextureColorMod(atlas, fg.r, fg.g, fg.b);
            SDL_SetTextureAlphaMod(atlas, fg.a);
            Rect dst(pos.x, pos.y, gr.w, gr.h);
            SDL_RenderCopy(rr, atlas, &gr, &dst);
        }

        pos.x += gr.w;
    }
}

void Text_Painter::draw_ucs4(nonstd::u32string_view str)
{
    for (uint32_t ch : str)
        draw_char(ch);
}

void Text_Painter::draw_utf8(nonstd::string_view str)
{
    for (const char *cur = str.begin(), *end = str.end(); cur != end;) {
        char32_t ch = utf::utf_traits<char>::decode(cur, end);
        if (ch == utf::incomplete) break;
        if (ch == utf::illegal) continue;
        draw_char(ch);
    }
}

size_t Text_Painter::measure_char(uint32_t ch)
{
    const Font_Glyph *g = font->glyph(ch);
    if (!g)
        return font->width();
    return font->width() * g->size;
}

size_t Text_Painter::measure_ucs4(nonstd::u32string_view str)
{
    size_t tw = 0;
    for (uint32_t ch : str)
        tw += measure_char(ch);
    return tw;
}

size_t Text_Painter::measure_utf8(nonstd::string_view str)
{
    size_t tw = 0;
    for (const char *cur = str.begin(), *end = str.end(); cur != end;) {
        char32_t ch = utf::utf_traits<char>::decode(cur, end);
        if (ch == utf::incomplete) break;
        if (ch == utf::illegal) continue;
        tw += measure_char(ch);
    }
    return tw;
}

Point Font_Atlas::size(const Font &font)
{
    Point size;
    size_t gn = font.glyph_count();
    unsigned maxgw = font.max_char_width();
    unsigned max_per_line = 8192 / maxgw;
    unsigned line_count = (gn + max_per_line - 1) / max_per_line;
    size.y = font.height() * line_count;
    size.x = maxgw * std::min<size_t>(max_per_line, gn);
    return size;
}

Rect Font_Atlas::coord(const Font &font, size_t index)
{
    Rect bounds;
    const Font_Glyph &g = font.nth_glyph(index);
    unsigned maxgw = font.max_char_width();
    unsigned max_per_line = 8192 / maxgw;
    bounds.w = g.size * font.width();
    bounds.h = font.height();
    bounds.x = maxgw * (index % max_per_line);
    bounds.y = bounds.h * (index / max_per_line);
    return bounds;
}

SDL_Texture *Font_Atlas::create(SDL_Renderer *rr, const Font &font)
{
    size_t gn = font.glyph_count();
    Point as = size(font);

    SDL_Surface *su = SDLpp_CreateRGBA4444Surface(as.x, as.y);
    if (!su)
        throw std::runtime_error("SDL_CreateRGBSurfaceWithFormat");
    auto surface_cleanup = nonstd::make_scope_exit([su] { SDL_FreeSurface(su); });

    if (SDL_MUSTLOCK(su)) {
        if (SDL_LockSurface(su) != 0)
            throw std::runtime_error("SDL_LockSurface");
    }

    uint8_t *su_pix = reinterpret_cast<uint8_t *>(su->pixels);
    unsigned su_pitch = su->pitch;

    for (size_t i = 0; i < gn; ++i) {
        const Font_Glyph &g = font.nth_glyph(i);
        Rect gr = coord(font, i);
        unsigned gsize = g.size;
        const void *gdata = g.data;
        unsigned gbits = 8 * gsize;

        for (unsigned row = 0; row < unsigned(gr.h); ++row) {
            uint16_t rowdata =
                (gsize == 1) ? reinterpret_cast<const uint8_t *>(gdata)[row] :
                (gsize == 2) ? reinterpret_cast<const uint16_t *>(gdata)[row] :
                0;
            for (unsigned col = 0; col < unsigned(gr.w); ++col) {
                uint8_t *pix = &su_pix[(gr.y + row) * su_pitch + (gr.x + col) * 2];
                if (rowdata & ((1 << (gbits - 1)) >> col))
                    *reinterpret_cast<uint16_t *>(pix) = 0xffff;
            }
        }
    }

    if (SDL_MUSTLOCK(su))
        SDL_UnlockSurface(su);

#if 0
    std::string filename = std::string("/tmp/font-") + font.name() + ".bmp";
    SDL_SaveBMP(su, filename.c_str());
#endif

    SDL_Texture *atlas = SDL_CreateTextureFromSurface(rr, su);
    if (!atlas)
        throw std::runtime_error("SDL_CreateTextureFromSurface");

    return atlas;
}

void Text_Painter::clear_font_caches()
{
    for (Font_Cache_Entry &ent : font_cache) {
        SDL_DestroyTexture(ent.tx);
        ent.tx = nullptr;
        ent.rr = nullptr;
    }
}
