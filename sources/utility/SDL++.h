//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <cstdint>

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

inline void SDLpp_RenderFillRect(SDL_Renderer *rr, const SDL_Rect &rect)
{
    SDL_RenderFillRect(rr, &rect);
}
inline void SDLpp_RenderDrawRect(SDL_Renderer *rr, const SDL_Rect &rect)
{
    SDL_RenderDrawRect(rr, &rect);
}

inline SDL_Surface *SDLpp_CreateRGBA32Surface(int width, int height)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    uint32_t rmask = 0xff000000;
    uint32_t gmask = 0x00ff0000;
    uint32_t bmask = 0x0000ff00;
    uint32_t amask = 0x000000ff;
#else
    uint32_t rmask = 0x000000ff;
    uint32_t gmask = 0x0000ff00;
    uint32_t bmask = 0x00ff0000;
    uint32_t amask = 0xff000000;
#endif
    return SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
}

inline SDL_Surface *SDLpp_CreateRGBA4444Surface(int width, int height)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    uint32_t rmask = 0xf000;
    uint32_t gmask = 0x0f00;
    uint32_t bmask = 0x00f0;
    uint32_t amask = 0x000f;
#else
    uint32_t rmask = 0x000f;
    uint32_t gmask = 0x00f0;
    uint32_t bmask = 0x0f00;
    uint32_t amask = 0xf000;
#endif
    return SDL_CreateRGBSurface(0, width, height, 16, rmask, gmask, bmask, amask);
}
