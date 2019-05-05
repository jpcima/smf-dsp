#include "ui/paint.h"
#include "utility/SDL++.h"

void RenderFillAlternating(SDL_Renderer *rr, const Rect &bounds, SDL_Color color1, SDL_Color color2)
{
    for (int x = 0, w = bounds.w; x < w; ++x) {
        for (int y = 0, h = bounds.h; y < h; ++y) {
            SDLpp_SetRenderDrawColor(rr, ((y + x) & 1) ? color1 : color2);
            SDL_RenderDrawPoint(rr, bounds.x + x, bounds.y + y);
        }
    }
}
