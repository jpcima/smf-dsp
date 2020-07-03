//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "keystate.h"
#include <gsl/gsl>
#include <string>
#include <vector>

struct Player_Song_Metadata {
    std::string name;
    std::string author;
    std::vector<std::string> text;
    char format[32] = {};
    unsigned track_count = 0;
};

struct Player_State {
    Keyboard_State kb;
    unsigned repeat_mode = 0;
    double time_position = 0;
    double duration = 0;
    double tempo = 0;
    unsigned speed = 100;
    std::string file_path;
    Player_Song_Metadata song_metadata;
    float audio_levels[10] {};
};
