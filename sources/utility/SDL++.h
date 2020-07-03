//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <memory>
#include <cstdint>

struct SDLpp_Window_Deleter {
    void operator()(SDL_Window *x) { SDL_DestroyWindow(x); }
};

typedef std::unique_ptr<SDL_Window, SDLpp_Window_Deleter> SDLpp_Window_u;

///
struct SDLpp_Renderer_Deleter {
    void operator()(SDL_Renderer *x) { SDL_DestroyRenderer(x); }
};

typedef std::unique_ptr<SDL_Renderer, SDLpp_Renderer_Deleter> SDLpp_Renderer_u;

///
struct SDLpp_Surface_Deleter {
    void operator()(SDL_Surface *x) { SDL_FreeSurface(x); }
};

typedef std::unique_ptr<SDL_Surface, SDLpp_Surface_Deleter> SDLpp_Surface_u;

///
struct SDLpp_Texture_Deleter {
    void operator()(SDL_Texture *x) { SDL_DestroyTexture(x); }
};

typedef std::unique_ptr<SDL_Texture, SDLpp_Texture_Deleter> SDLpp_Texture_u;

///
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

inline void SDLpp_RenderDrawHLine(SDL_Renderer *rr, int x1, int x2, int y)
{
    SDL_Rect rect{x1, y, x2 - x1 + 1, 1};
    SDL_RenderFillRect(rr, &rect);
}

inline void SDLpp_RenderDrawVLine(SDL_Renderer *rr, int x, int y1, int y2)
{
    SDL_Rect rect{x, y1, 1, y2 - y1 + 1};
    SDL_RenderFillRect(rr, &rect);
}
