#pragma once
#include "utility/geometry.h"
#include <SDL.h>

enum Paint_Type {
    Pt_Background = 1,
    Pt_Foreground = 2,
};

void RenderFillAlternating(SDL_Renderer *rr, const Rect &bounds, SDL_Color color1, SDL_Color color2);
