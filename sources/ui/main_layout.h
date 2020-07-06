//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "utility/geometry.h"
#include <gsl/gsl>
struct Font;

class Main_Layout {
public:
    void create_layout(Rect bounds);

    // structures
    struct Text_Rect {
        Font *font = nullptr;
        gsl::cstring_span text;
        Rect bounds;
    };
    struct Line {
        Point p1, p2;
    };

    // vertical divisions from the whole
    Rect part_top;
    Rect part_middle;
    Rect part_bottom;

    // channel rows
    Rect row_channel_heading;
    Rect row_channel[16];

    // bottom info / file browser
    Rect info_heading;
    Rect info_box;
    Line info_underline;

    // top info
    Text_Rect time_label;
    Text_Rect time_value;
    Text_Rect length_label;
    Text_Rect length_value;
    Text_Rect tempo_label;
    Text_Rect tempo_value;
    Text_Rect speed_label;
    Text_Rect speed_value;
    Text_Rect playing_label;
    Text_Rect playing_value;
    Rect playing_progress;
    Text_Rect repeat_label;
    Text_Rect multi_label;

    // channel heading
    Line channel_heading_underline;
    Text_Rect octkb_label;
    Rect octkb_triangle;
    Text_Rect volume_label;
    Text_Rect pan_label;
    Text_Rect instrument_label;

    // channel info
    Rect channel_piano[16];
    Line channel_underline[16];
    Text_Rect channel_volume_value[16];
    Text_Rect channel_pan_value[16];
    Rect channel_number_label[16];
    Rect channel_midispec_label[16];
    Rect channel_instrument_label[16];

    // file info
    Text_Rect file_label;
    Text_Rect file_num_label;
    Text_Rect file_dir_path;

    // program identification and version
    Text_Rect author_label;
    Text_Rect version_label;
    Rect logo_rect;
    Rect level_meter_rect[10];

    static void measure_text(Text_Rect &tr);
};
