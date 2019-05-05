#pragma once
#include "keystate.h"
#include <gsl.hpp>
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
};
