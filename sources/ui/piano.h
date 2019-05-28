//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>

class Piano {
public:
    explicit Piano(const Rect &bounds);

    static unsigned best_width();

    void set_keys(const uint8_t *keys);

    void paint(SDL_Renderer *rr, int paint);

private:
    Rect bounds_;
    uint8_t keys_[128] {};
};
