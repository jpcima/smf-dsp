#pragma once
#include "utility/geometry.h"
#include <SDL.h>

inline void SDLpp_SetRenderDrawColor(SDL_Renderer *rr, const SDL_Color &color)
{
    SDL_SetRenderDrawColor(rr, color.r, color.g, color.b, color.a);
}

struct SDLpp_ClipState {
    SDL_Rect r;
};

inline void SDLpp_SaveClipState(SDL_Renderer *rr, SDLpp_ClipState &cs)
{
    cs.r.w = 0;
    cs.r.h = 0;
    SDL_RenderGetClipRect(rr, &cs.r);
}

inline void SDLpp_RestoreClipState(SDL_Renderer *rr, const SDLpp_ClipState &cs)
{
    if (cs.r.w > 0 && cs.r.h > 0)
        SDL_RenderSetClipRect(rr, &cs.r);
    else
        SDL_RenderSetClipRect(rr, nullptr);
}

inline void SDLpp_RenderDrawDottedHLine(SDL_Renderer *rr, int x1, int x2, int y)
{
    for (int x = x1; x <= x2; x += 2)
        SDL_RenderDrawPoint(rr, x, y);
}

inline void SDLpp_RenderDrawLine(SDL_Renderer *rr, const Point &p1, const Point &p2)
{
    SDL_RenderDrawLine(rr, p1.x, p1.y, p2.x, p2.y);
}
