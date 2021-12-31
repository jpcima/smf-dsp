//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <memory>
struct Player_Song_Metadata;

class Metadata_Display {
public:
    explicit Metadata_Display(const Rect &bounds);
    ~Metadata_Display();

    void update_data(const Player_Song_Metadata &md);

    void paint(SDL_Renderer *rr);

private:
    const Rect bounds_;
    std::unique_ptr<Player_Song_Metadata> md_;
};
