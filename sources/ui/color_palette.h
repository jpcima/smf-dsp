//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/misc.h"
#include <SimpleIni.h>
#include <SDL_pixels.h>
#include <gsl/gsl>
#include <memory>
#include <cassert>

struct Color_Palette
{
    Color_Palette();

    static Color_Palette create_default();
    static Color_Palette &get_current();

    bool load(const CSimpleIniA &ini, const char *section);
    static void save_defaults(CSimpleIniA &ini, const char *section, bool overwrite);

    SDL_Color &operator[](size_t index) noexcept;
    const SDL_Color &operator[](size_t index) const noexcept;

    static constexpr SDL_Color transparent() noexcept
        { return SDL_Color{0, 0, 0, 0}; }

private:
    std::unique_ptr<SDL_Color[]> colors_;
};

namespace Colors {
    enum {
        background,
        info_box_background,
        text_browser_foreground,
        metadata_label,
        metadata_value,
        text_min_brightness,
        text_low_brightness,
        text_high_brightness,
        piano_white_key,
        piano_white_shadow,
        piano_black_key,
        piano_pressed_key,
        digit_on,
        digit_off,
        box_frame,
        box_background,
        box_title,
        box_foreground,
        box_foreground_secondary,
        box_active_item_background,
        box_active_item_foreground,
    };

    maybe_unused static const gsl::cstring_span Name[] = {
        "background",
        "info-box-background",
        "text-browser-foreground",
        "metadata-label",
        "metadata-value",
        "text-min-brightness",
        "text-low-brightness",
        "text-high-brightness",
        "piano-white-key",
        "piano-white-shadow",
        "piano-black-key",
        "piano-pressed-key",
        "digit-on",
        "digit-off",
        "box-frame",
        "box-background",
        "box-title",
        "box-foreground",
        "box-foreground-secondary",
        "box-active-item-background",
        "box-active-item-foreground",
    };

    maybe_unused static constexpr size_t Count =
        sizeof(Name) / sizeof(Name[0]);
}

inline SDL_Color &Color_Palette::operator[](size_t index) noexcept
{
    assert(index < Colors::Count);
    return colors_[index];
}

inline const SDL_Color &Color_Palette::operator[](size_t index) const noexcept
{
    assert(index < Colors::Count);
    return colors_[index];
}
