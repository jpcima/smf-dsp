//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "piano.h"
#include "color_palette.h"
#include "paint.h"
#include "utility/SDL++.h"
#include <cstdlib>

static constexpr unsigned num_octaves = 8;
static constexpr unsigned basic_octave = 1;

Piano::Piano(const Rect &bounds)
    : bounds_(bounds)
{
}

unsigned Piano::best_width()
{
    return 1 + 4 * 7 * num_octaves;
}

void Piano::set_keys(const uint8_t *keys)
{
    memcpy(keys_, keys, 128);
}

void Piano::paint(SDL_Renderer *rr, int paint)
{
    Rect bounds = bounds_;
    const Color_Palette &pal = Color_Palette::get_current();

    int x = bounds.x;
    int y = bounds.y;
    // int w = bounds.w;
    int h = bounds.h;

    for (unsigned oct = 0; oct < num_octaves; ++oct) {
        int octx = x + 1 + oct * 4 * 7;

        const unsigned bk[] = {1, 3, 6, 8, 10};
        const unsigned wk[] = {0, 2, 4, 5, 7, 9, 11};

        for (int key_w = 0; key_w < 7; ++key_w) {
            SDL_Color ck = pal.piano_white_key;
            SDL_Color cs = pal.piano_white_shadow;

            if (paint & Pt_Foreground) {
                unsigned kn = wk[key_w] + 12 * (oct + basic_octave);
                if (kn < 128 && keys_[kn] > 0) {
                    ck = pal.piano_pressed_key;
                    cs = ck;
                }
            }

            Rect rk(octx + key_w * 4, y, 3, h - 1);
            SDLpp_SetRenderDrawColor(rr, ck);
            SDL_RenderFillRect(rr, &rk);
            SDLpp_SetRenderDrawColor(rr, cs);
            SDL_RenderDrawLine(rr, rk.x, y + h - 1, rk.x + 2, y + h - 1);
        }

        //
        for (unsigned key_b = 0; key_b < 5; ++key_b) {
            SDL_Color ck = pal.piano_black_key;

            if (paint & Pt_Foreground) {
                unsigned kn = bk[key_b] + 12 * (oct + basic_octave);
                if (kn < 128 && keys_[kn] > 0)
                    ck = pal.piano_pressed_key;
            }

            int key_x = 2 + 4 * key_b + ((key_b < 2) ? 0 : 4);
            Rect rk(octx + key_x, y, 3, h / 2);
            SDLpp_SetRenderDrawColor(rr, ck);
            SDL_RenderFillRect(rr, &rk);
        }
    }
}
