//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

extern DECLSPEC SDL_Surface *SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc);

#ifdef __cplusplus
}
#endif
