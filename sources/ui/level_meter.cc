//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ui/level_meter.h"
#include "ui/color_palette.h"
#include "ui/paint.h"
#include "utility/SDL++.h"

Level_Meter::Level_Meter(const Rect &bounds)
    : bounds_(bounds)
{
}

void Level_Meter::value(float v)
{
    value_ = v;
}

void Level_Meter::paint(SDL_Renderer *rr, int paint)
{
    const Rect bounds = bounds_;
    const float v = value_;
    const Color_Palette &pal = Color_Palette::get_current();

    for (int y = bounds.top(); y <= bounds.bottom(); y += 2) {
        const float ry = 1.0f - (y - bounds.top()) * (1.0f / (bounds.h - 1));
        if (paint & Pt_Background) {
            SDLpp_SetRenderDrawColor(rr, pal[Colors::meter_bar_off]);
            SDL_RenderDrawLine(rr, bounds.left(), y, bounds.right(), y);
        }
        else if ((paint & Pt_Foreground) && ry < v) {
            SDLpp_SetRenderDrawColor(rr, pal[Colors::meter_bar_on]);
            SDL_RenderDrawLine(rr, bounds.left(), y, bounds.right(), y);
        }
    }
}
