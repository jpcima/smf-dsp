#include "main_layout.h"
#include "ui/piano.h"
#include "ui/text.h"
#include "ui/fonts.h"

void Main_Layout::create_layout(Rect bounds)
{
    part_middle = bounds;
    part_top = part_middle.take_from_top(60);
    part_bottom = part_middle.take_from_bottom(82);

    {
        Rect r = part_middle;
        row_channel_heading = r.take_from_top(10);
        for (unsigned ch = 0; ch < 16; ++ch) {
            r.chop_from_top(1);
            row_channel[ch] = r.take_from_top(14);
        }
    }

    {
        Rect r = part_bottom;
        info_heading = r.take_from_top(12);
        info_box = r;
        info_underline.p1 = Point(r.x, r.y - 3);
        info_underline.p2 = Point(r.x + r.w - 1, r.y - 3);
    }

    {
        //
        Rect r = Rect(part_top).chop_from_top(2).take_from_top(16);

        time_label.font = &font_fmdsp_medium;
        time_label.text = "TIME:";
        time_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(time_label);
        r.chop_from_left(time_label.bounds.w);

        r.chop_from_left(8);

        time_value.font = &font_digit;
        time_value.text = "88:88:88";
        time_value.bounds = Rect(r.x, r.y, 0, 0);
        measure_text(time_value);
        r.chop_from_left(time_value.bounds.w);

        r.chop_from_left(16);

        length_label.font = &font_fmdsp_medium;
        length_label.text = "LENGTH:";
        length_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(length_label);
        r.chop_from_left(length_label.bounds.w);

        r.chop_from_left(8);

        length_value.font = &font_digit;
        length_value.text = "88:88:88";
        length_value.bounds = Rect(r.x, r.y, 0, 0);
        measure_text(length_value);
        r.chop_from_left(length_value.bounds.w);

        r.chop_from_left(16);

        tempo_label.font = &font_fmdsp_medium;
        tempo_label.text = "TEMPO:";
        tempo_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(tempo_label);
        r.chop_from_left(tempo_label.bounds.w);

        r.chop_from_left(8);

        tempo_value.font = &font_digit;
        tempo_value.text = "888";
        tempo_value.bounds = Rect(r.x, r.y, 0, 0);
        measure_text(tempo_value);
        r.chop_from_left(tempo_value.bounds.w);

        r.chop_from_left(16);

        speed_label.font = &font_fmdsp_medium;
        speed_label.text = "SPEED:";
        speed_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(speed_label);
        r.chop_from_left(speed_label.bounds.w);

        r.chop_from_left(8);

        speed_value.font = &font_digit;
        speed_value.text = "888";
        speed_value.bounds = Rect(r.x, r.y, 0, 0);
        measure_text(speed_value);
        r.chop_from_left(speed_value.bounds.w);

        //
        r = part_top.off_by(Point(0, 16)).take_from_top(16);

        playing_label.font = &font_fmdsp_medium;
        playing_label.text = "PLAYING:";
        playing_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(playing_label);
        r.chop_from_left(playing_label.bounds.w);

        r.chop_from_left(8);

        playing_value.font = &font_s12;
        playing_value.bounds = r;
        playing_value.bounds.w = 290;

        //
        playing_progress = part_top.off_by(Point(0, 32))
            .take_from_left(playing_value.bounds.right() + 1)
            .take_from_top(2);

        //
        r = part_top.off_by(Point(0, 16)).take_from_top(16).chop_from_left(playing_value.bounds.right() + 1);

        r.chop_from_left(20);

        repeat_label.font = &font_fmdsp_medium;
        repeat_label.text = "REPEAT";
        repeat_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(repeat_label);

        r = r.off_by(Point(0, 10));

        multi_label.font = &font_fmdsp_medium;
        multi_label.text = "MULTI";
        multi_label.bounds = Rect(r.x, r.y + 3, 0, 0);
        measure_text(multi_label);
    }

    {
        Rect r = row_channel_heading;

        channel_heading_underline.p1 = Point(r.x, r.y + r.h - 1);
        channel_heading_underline.p2 = Point(r.x + r.w - 1, r.y + r.h - 1);

        r.chop_from_right(2);

        octkb_label.font = &font_fmdsp_medium;
        octkb_label.text = "8 OCTAVE KEYBOARD";
        octkb_label.bounds.x = r.take_from_right(Piano::best_width()).x;
        octkb_label.bounds.y = r.y;
        measure_text(octkb_label);

        r.chop_from_right(8);

        r.chop_from_right(16);

        pan_label.font = &font_fmdsp_medium;
        pan_label.text = "PAN";
        measure_text(pan_label);
        pan_label.bounds.x = r.take_from_right(pan_label.bounds.w).x;
        pan_label.bounds.y = r.y;

        r.chop_from_right(16);

        volume_label.font = &font_fmdsp_medium;
        volume_label.text = "VOL";
        measure_text(volume_label);
        volume_label.bounds.x = r.take_from_right(volume_label.bounds.w).x;
        volume_label.bounds.y = r.y;

        r.chop_from_right(16);

        r.chop_from_left(2);
        r.take_from_left(16);

        r.chop_from_left(8);

        instrument_label.font = &font_fmdsp_medium;
        instrument_label.text = "PROGRAM";
        instrument_label.bounds.x = r.chop_from_left(24).x;
        instrument_label.bounds.y = r.y;
        measure_text(instrument_label);
    }

    for (unsigned ch = 0; ch < 16; ++ch) {
        Rect r = row_channel[ch];

        r.chop_from_right(2);
        channel_piano[ch] = r.take_from_right(Piano::best_width());

        r.chop_from_right(8);

        channel_underline[ch].p1 = Point(r.x, r.y + r.h - 1);
        channel_underline[ch].p2 = Point(r.x + r.w - 1, r.y + r.h - 1);

        r.chop_from_right(16);

        channel_pan_value[ch].font = &font_s12;
        channel_pan_value[ch].text = "000";
        channel_pan_value[ch].bounds.x = r.take_from_right(pan_label.bounds.w).x;
        channel_pan_value[ch].bounds.y = r.y;
        measure_text(channel_pan_value[ch]);

        r.chop_from_right(16);

        channel_volume_value[ch].font = &font_s12;
        channel_volume_value[ch].text = "000";
        channel_volume_value[ch].bounds.x = r.take_from_right(volume_label.bounds.w).x;
        channel_volume_value[ch].bounds.y = r.y;
        measure_text(channel_volume_value[ch]);

        r.chop_from_right(16);

        r.chop_from_left(2);

        channel_number_label[ch] = r.take_from_left(16);

        r.chop_from_left(8);

        channel_midispec_label[ch] = r.take_from_left(16);

        r.chop_from_left(8);

        channel_instrument_label[ch] = r;
    }

    {
        Rect r = info_heading;

        file_label.font = &font_fmdsp_medium;
        file_label.text = "File:";
        measure_text(file_label);
        file_label.bounds.x = r.take_from_left(file_label.bounds.w).x;
        file_label.bounds.y = r.y;

        r.chop_from_left(8);

        file_num_label.font = &font_fmdsp_medium;
        file_num_label.text = "8888 / 8888";
        measure_text(file_num_label);
        file_num_label.bounds.x = r.take_from_right(file_num_label.bounds.w).x;
        file_num_label.bounds.y = r.y;

        r.chop_from_right(8);

        file_dir_path.font = &font_fmdsp_medium;
        file_dir_path.bounds = r;
    }

    {
        Rect r = Rect(part_top).chop_from_top(4).chop_from_right(4);

        logo_rect = r.take_from_right(r.h);

        r.chop_from_right(8);

        author_label.font = &font_fmdsp_medium;
        author_label.text = "(C)" PROGRAM_AUTHOR;
        version_label.font = &font_fmdsp_medium;
        version_label.text = "Ver." PROGRAM_VERSION;
        measure_text(author_label);
        measure_text(version_label);
        author_label.bounds.x = r.take_from_right(std::max(author_label.bounds.w, version_label.bounds.w)).x;
        author_label.bounds.y = r.y;
        version_label.bounds.x = author_label.bounds.x;
        version_label.bounds.y = r.y + 4 + author_label.bounds.h;
    }
}

void Main_Layout::measure_text(Text_Rect &tr)
{
    Text_Painter tp;
    tp.font = tr.font;
    tr.bounds.w = tp.measure_utf8(tr.text);
    tr.bounds.h = tp.font->height();
}
