//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>

class Level_Meter
{
public:
    explicit Level_Meter(const Rect &bounds);
    void value(float v);
    void paint(SDL_Renderer *rr, int paint);

private:
    Rect bounds_;
    float value_;
};
