//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>

enum Paint_Type {
    Pt_Background = 1,
    Pt_Foreground = 2,
};

void RenderFillAlternating(SDL_Renderer *rr, const Rect &bounds, SDL_Color color1, SDL_Color color2);
