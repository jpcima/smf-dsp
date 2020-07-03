//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "SDL_stb_image.h"
#define STB_IMAGE_IMPLEMENTATION 1
#include <stb_image.h>
#include <string.h>

typedef struct IMG_stbi_stream {
    SDL_RWops *handle;
    int eof;
} IMG_stbi_stream;

static int IMG_stbi_read(void *user, char *data, int size)
{
    IMG_stbi_stream *stream = (IMG_stbi_stream *)user;
    if (stream->eof)
        return 0;
    int n = (int)SDL_RWread(stream->handle, data, 1, size);
    if (n < 1)
        stream->eof = 1;
    return n;
}

static void IMG_stbi_skip(void *user, int n)
{
    IMG_stbi_stream *stream = (IMG_stbi_stream *)user;
    if (SDL_RWseek(stream->handle, n, SEEK_CUR) == -1)
        stream->eof = 1;
}

static int IMG_stbi_eof(void *user)
{
    IMG_stbi_stream *stream = (IMG_stbi_stream *)user;
    return stream->eof;
}

SDL_Surface *SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc)
{
    stbi_io_callbacks cb;
    cb.read = &IMG_stbi_read;
    cb.skip = &IMG_stbi_skip;
    cb.eof = &IMG_stbi_eof;

    IMG_stbi_stream stream;
    stream.handle = src;
    stream.eof = 0;

    int w, h;
    const int channels = 4;
    stbi_uc *image = stbi_load_from_callbacks(&cb, &stream, &w, &h, NULL, channels);

    if (freesrc)
        SDL_RWclose(src);

    if (!image)
        return NULL;

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        stbi_image_free(image);
        return NULL;
    }

    if (SDL_LockSurface(surface) == -1) {
        SDL_FreeSurface(surface);
        stbi_image_free(image);
        return NULL;
    }

    uint8_t *pixels = (uint8_t *)surface->pixels;
    int pitch = surface->pitch;

    for (int y = 0; y < h; ++y)
        memcpy(pixels + y * pitch, image + y * (4 * w), 4 * w);

    SDL_UnlockSurface(surface);
    stbi_image_free(image);

    return surface;
}
