//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "application.h"
#include "player/player.h"
#include "player/playlist.h"
#include "player/command.h"
#include "player/instrument.h"
#include "player/instruments/synth_fx.h"
#include "synth/synth_host.h"
#include "data/ins_names.h"
#include "ui/main_layout.h"
#include "ui/file_browser.h"
#include "ui/file_entry.h"
#include "ui/metadata_display.h"
#include "ui/level_meter.h"
#include "ui/piano.h"
#include "ui/modal_box.h"
#include "ui/text.h"
#include "ui/fonts.h"
#include "ui/color_palette.h"
#include "ui/paint.h"
#include "utility/SDL++.h"
#include "utility/paths.h"
#include "utility/charset.h"
#include "utility/strings.h"
#include "utility/desktop.h"
#include "utility/logs.h"
#if defined(HAVE_SDL2_IMAGE)
#include <SDL_image.h>
#else
#include "utility/SDL_stb_image.h"
#endif
#include <gsl/gsl>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

const Point Application::size_{640, 400};

static constexpr unsigned fadeout_delay = 10;
static constexpr unsigned quit_by_esc_key_delay = 500;

static constexpr char default_synth_id[] = "adlmidi";

static const std::pair<gsl::cstring_span, gsl::cstring_span> help_items[] = {
    {"F1", "Open the help screen"},
    {"F2", "Select a MIDI device for playback"},
    {"F3", "Select a synthesizer device for playback"},
    {"F4", "Configure global audio effects"},
    {"F9", "Select a theme for the user interface"},
    {"F12", "Open the configuration directory"},
    {"Tab", "Switch between file browser and track info"},
    {"Esc", "Quit the program"},
    {u8"Page \u2191\u2193", "Go to next/previous track in playlist/folder"},
    {"Space", "Pause/unpause"},
    {"Home", "Seek to beginning of current track"},
    {"End", "Seek to end of current track"},
    {u8"\u2190\u2191\u2193\u2192", "Navigate in the file browser"},
    {u8"\u2190\u2192", u8"In track info view, seek track by ±5 seconds"},
    {u8"Shift \u2190\u2192", u8"In any view, seek track by ±10 seconds"},
    {"[]", "Increase/decrease speed by 1%"},
    {"/", "Scan songs in the current folder and play them at random"},
    {"`", "Switch between repeat modes: On/Off, and Single/Multi"},
};

template <int EventType>
uint32_t timer_push_event(uint32_t interval, void *user_data)
{
    SDL_Event event;
    event.type = EventType;
    event.user.data1 = user_data;
    event.user.data2 = nullptr;
    SDL_PushEvent(&event);
    return interval;
}

Application::Application()
{
    Log::i("Configuration directory: %s", get_configuration_dir().c_str());

    std::unique_ptr<CSimpleIniA> ini = initialize_config();

    if (const char *value = ini->GetValue("", "theme"))
        load_theme(value);
    else
        load_default_theme();

    if (const char *value = ini->GetValue("", "midi-out-device"))
        last_midi_output_choice_.assign(value);
    if (const char *value = ini->GetValue("", "synth-device"))
        last_synth_choice_.assign(value);

    Rect bounds(0, 0, size_.x, size_.y);
    Main_Layout *layout = new Main_Layout;
    layout_.reset(layout);
    layout->create_layout(bounds);

    File_Browser *fb = new File_Browser(layout->info_box);
    file_browser_.reset(fb);
    fb->FileOpenCallback = [this](const std::string &dir, const File_Entry *entries, size_t index, size_t count)
                               { play_file(dir, entries, index, count); };
    fb->FileFilterCallback = &filter_file_entry;

    Metadata_Display *mdd = new Metadata_Display(layout->info_box);
    metadata_display_.reset(mdd);

    for (int i = 0; i < 10; ++i)
        level_meter_[i].reset(new Level_Meter(layout->level_meter_rect[i]));

    std::string initial_path;
    if (const char *value = ini->GetValue("", "initial-path"))
        initial_path = make_path_canonical(expand_path_tilde(value));

    if (!initial_path.empty())
        fb->set_current_path(initial_path);
    else {
        std::string home = get_home_directory();
        if (!home.empty())
            fb->set_cwd(home);
    }

    Player *pl = new Player;
    player_.reset(pl);

    ps_.reset(new Player_State);
    pl->StateCallback = [this](const Player_State &ps)
                            { receive_state_in_other_thread(ps); };

    uint32_t update_interval = 50;
    update_timer_ = SDL_AddTimer(update_interval, &timer_push_event<SDL_USEREVENT>, this);
    if (update_timer_ == 0)
        throw std::runtime_error("SDL_AddTimer");

    choose_midi_output(false, last_midi_output_choice_);
    choose_synth(false, last_synth_choice_);
}

Application::~Application()
{
    if (update_timer_)
        SDL_RemoveTimer(update_timer_);
}

SDL_Window *Application::init_window()
{
    SDL_Window *win = SDL_CreateWindow(
        PROGRAM_DISPLAY_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        size_.x, size_.y, 0);
    if (!win)
        return nullptr;

    static const uint8_t png_data[] = {
        #include "icon.dat"
    };
    size_t png_size = sizeof(png_data);

    SDLpp_Surface_u icon(IMG_Load_RW(SDL_RWFromConstMem(png_data, png_size), true));
    if (icon)
        SDL_SetWindowIcon(win, icon.get());

    window_.reset(win);
    return win;
}

SDL_Renderer *Application::init_renderer()
{
    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini) ini = create_configuration();

    uint32_t flags = 0;
    if (ini->GetBoolValue("", "force-software-renderer"))
        flags |= SDL_RENDERER_SOFTWARE;

    SDL_Renderer *rr = SDL_CreateRenderer(window_.get(), -1, flags);
    if (!rr)
        return nullptr;

    SDL_RendererInfo ri = {};
    SDL_GetRendererInfo(rr, &ri);
    Log::i("Renderer type: %s", ri.name);

    renderer_.reset(rr);
    return rr;
}

void Application::exec()
{
    SDL_Renderer *rr = renderer_.get();

    SDL_Event event;
    bool shutting_down = false;
    while (!should_quit() && SDL_WaitEvent(&event)) {
        bool update = false;

        switch (event.type) {
        case SDL_KEYDOWN:
            if (!shutting_down)
                update = handle_key_pressed(event.key);
            break;
        case SDL_KEYUP:
            if (!shutting_down)
                update = handle_key_released(event.key);
            break;
        case SDL_TEXTINPUT:
            if (!shutting_down)
                update = handle_text_input(event.text);
            break;
        case SDL_WINDOWEVENT:
            update = true;
            break;
        case SDL_USEREVENT:
            update = true;
            if (!shutting_down) {
                request_update();
                update_modals();
            }
            advance_shutdown();
            break;
        case SDL_USEREVENT + 1:
            engage_shutdown_if_esc_key();
            break;
        case SDL_QUIT:
            shutting_down = true;
            engage_shutdown();
            break;
        default:
            //Log::i("SDL event %X", event.type);
            break;
        }

        if (update) {
            paint_cached_background(rr);
            paint(rr, Pt_Foreground);
            SDL_RenderPresent(rr);
        }
    }
}

void Application::set_scale_factor(SDL_Window *win, unsigned sf)
{
    if (sf < 1) sf = 1;
    if (sf > 3) sf = 3;

    if (scale_factor_ != sf) {
        scale_factor_ = sf;
        SDL_SetWindowSize(win, size_.x * sf, size_.y * sf);
    }
}

void Application::paint(SDL_Renderer *rr, int paint)
{
    const Main_Layout &lo = *layout_;
    const Color_Palette &pal = Color_Palette::get_current();

    SDLpp_ClipState clip;

    SDL_RenderSetScale(rr, scale_factor_, scale_factor_);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);

    auto draw_text_rect = [rr](const Main_Layout::Text_Rect &tr, gsl::cstring_span str, const SDL_Color &color) {
                              Text_Painter tp;
                              tp.rr = rr;
                              tp.fg = color;
                              tp.pos = tr.bounds.origin();
                              tp.font = tr.font;
                              tp.draw_utf8(str);
                          };

    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, pal[Colors::background]);
        SDL_RenderClear(rr);
    }

    SDL_Surface *wallpaper_image = wallpaper_image_.get();
    if ((paint & Pt_Background) && wallpaper_image) {
        Rect bounds(0, 0, size_.x, size_.y);
        Rect src = Rect(0, 0, wallpaper_image->w, wallpaper_image->h);
        Rect dst = Rect((size_.x - src.w) / 2, (size_.y - src.h) / 2, src.w, src.h);
        SDLpp_Texture_u wallpaper_tex{SDL_CreateTextureFromSurface(rr, wallpaper_image)};
        if (wallpaper_tex)
            SDL_RenderCopy(rr, wallpaper_tex.get(), &src, &dst);
    }

    SDL_Surface *logo_image = logo_image_.get();
    if ((paint & Pt_Background) && logo_image) {
        Rect r = lo.logo_rect;
        Rect src = Rect(0, 0, logo_image->w, logo_image->h);
        Rect dst = Rect(r.x + r.w - src.w, r.y, src.w, src.h);
        SDLpp_Texture_u logo_tex{SDL_CreateTextureFromSurface(rr, logo_image)};
        if (logo_tex)
            SDL_RenderCopy(rr, logo_tex.get(), &src, &dst);
    }

    if (paint & Pt_Background) {
        draw_text_rect(lo.author_label, lo.author_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.version_label, lo.version_label.text, pal[Colors::text_low_brightness]);
    }

    if (paint & Pt_Background)
        RenderFillAlternating(rr, lo.info_box, pal[Colors::info_box_background], pal.transparent());

    std::unique_lock<std::mutex> ps_lock(ps_mutex_);
    Player_State ps = *ps_;
    ps_lock.unlock();

    char buf_hms[9];
    auto hms = [](char *dst, unsigned ss) -> char * {
                   unsigned mm = ss / 60;
                   ss -= 60 * mm;
                   unsigned hh = mm / 60;
                   mm -= 60 * hh;
                   if (hh < 100)
                       sprintf(dst, "%02u:", hh);
                   else
                       strcpy(dst, "--:");
                   sprintf(dst + 3, "%02u:%02u", mm, ss);
                   return dst;
               };

    // Draw top
    if (paint & Pt_Background) {
        draw_text_rect(lo.time_label, lo.time_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.time_value, lo.time_value.text, pal[Colors::digit_off]);
    }
    if (paint & Pt_Foreground)
        draw_text_rect(lo.time_value, hms(buf_hms, ps.time_position), pal[Colors::digit_on]);

    if (paint & Pt_Background) {
        draw_text_rect(lo.length_label, lo.length_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.length_value, lo.length_value.text, pal[Colors::digit_off]);
    }
    if (paint & Pt_Foreground)
        draw_text_rect(lo.length_value, hms(buf_hms, ps.duration), pal[Colors::digit_on]);
    if (paint & Pt_Background)
        draw_text_rect(lo.playing_label, lo.playing_label.text, pal[Colors::text_low_brightness]);
    if (paint & Pt_Foreground) {
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.playing_value.bounds);
        draw_text_rect(lo.playing_value, path_file_name(ps.file_path), pal[Colors::text_high_brightness]);
        SDLpp_RestoreClipState(rr, clip);
    }
    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, pal[Colors::text_min_brightness]);
        SDL_RenderFillRect(rr, &lo.playing_progress);
    }
    if (paint & Pt_Foreground) {
        SDLpp_SetRenderDrawColor(rr, pal[Colors::text_high_brightness]);
        Rect r = lo.playing_progress;
        r.w = int(lo.playing_progress.w * ps.time_position / ps.duration + 0.5);
        SDL_RenderFillRect(rr, &r);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.tempo_label, lo.tempo_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.tempo_value, lo.tempo_value.text, pal[Colors::digit_off]);
    }
    if (paint & Pt_Foreground) {
        char tempo_str[4] = "---";
        long tempo_val = long(ps.tempo + 0.5);
        if (tempo_val >= 0 && tempo_val < 1000)
            sprintf(tempo_str, "%03ld", tempo_val);
        draw_text_rect(lo.tempo_value, tempo_str, pal[Colors::digit_on]);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.speed_label, lo.speed_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.speed_value, lo.speed_value.text, pal[Colors::digit_off]);
    }
    if (paint & Pt_Foreground) {
        char speed_str[4] = "---";
        unsigned speed_val = ps.speed;
        if (speed_val < 1000)
            sprintf(speed_str, "%03u", speed_val);
        draw_text_rect(lo.speed_value, speed_str, pal[Colors::digit_on]);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.repeat_label, lo.repeat_label.text, pal[Colors::digit_off]);
        draw_text_rect(lo.multi_label, lo.multi_label.text, pal[Colors::digit_off]);
    }
    if (paint & Pt_Foreground) {
        if ((ps.repeat_mode & (Repeat_On|Repeat_Off)) == Repeat_On)
            draw_text_rect(lo.repeat_label, lo.repeat_label.text, pal[Colors::digit_on]);
        if ((ps.repeat_mode & (Repeat_Multi|Repeat_Single)) == Repeat_Multi)
            draw_text_rect(lo.multi_label, lo.multi_label.text, pal[Colors::digit_on]);
    }

    // Draw audio levels
    for (int i = 0; i < 10; ++i) {
        Level_Meter &lm = *level_meter_[i];
        const float dbmin = -60.0;
        const float dbmax = 0.0;
        const float db = 20.0f * std::log10(
            std::max(ps.audio_levels[i], std::pow(10.0f, 0.05f * dbmin)));
        lm.value((db - dbmin) * (1.0f / (dbmax - dbmin)));
        lm.paint(rr, paint);
    }

    // Draw MIDI channel info heading
    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, pal[Colors::text_min_brightness]);
        SDLpp_RenderDrawHLine(rr, lo.channel_heading_underline.p1.x, lo.channel_heading_underline.p2.x, lo.channel_heading_underline.p1.y);

        draw_text_rect(lo.octkb_label, lo.octkb_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.volume_label, lo.volume_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.pan_label, lo.pan_label.text, pal[Colors::text_low_brightness]);
        draw_text_rect(lo.instrument_label, lo.instrument_label.text, pal[Colors::text_low_brightness]);
    }

    // Draw MIDI channel info
    for (unsigned ch = 0; ch < 16; ++ch) {
        const Channel_State &cs = ps.kb.channel[ch];

        Piano piano(lo.channel_piano[ch]);
        piano.set_keys(cs.key);
        piano.paint(rr, paint);

        if (paint & Pt_Background) {
            SDLpp_SetRenderDrawColor(rr, pal[Colors::text_min_brightness]);
            SDLpp_RenderDrawDottedHLine(rr, lo.channel_underline[ch].p1.x, lo.channel_underline[ch].p2.x, lo.channel_underline[ch].p1.y);
        }

        if (paint & Pt_Background) {
            Rect r = lo.channel_number_label[ch];
            Text_Painter tp;
            tp.rr = rr;
            tp.font = &font_fmdsp_small;

            tp.fg = pal[Colors::text_high_brightness];
            tp.font = &font_fmdsp_small;
            tp.pos = Point(r.x, r.y - 1);
            tp.draw_utf8("CH");

            tp.font = &font_fmdsp_medium;
            if (ch >= 9) {
                char32_t ch_str[3] = {U'0' + (ch + 1) / 10, U'0' + (ch + 1) % 10, U'\0'};
                tp.pos = Point(r.x, r.y + 6);
                tp.draw_ucs4(ch_str);
            }
            else {
                char32_t ch_str[2] = {U'0' + (ch + 1) % 10, U'\0'};
                tp.pos = Point(r.x + 3, r.y + 6);
                tp.draw_ucs4(ch_str);
            }
        }

        //
        {
            Text_Painter tp;
            tp.rr = rr;

            const char *patch_name = nullptr;
            unsigned patch_spec = Midi_Spec_GM1;

            if (paint & Pt_Foreground) {
                Rect r = lo.channel_midispec_label[ch];

                unsigned desired_spec = Midi_Spec_GM1;
                switch (ps.kb.midispec) {
                default: break;
                case KMS_GeneralMidi2: desired_spec |= Midi_Spec_GM2; break;
                case KMS_YamahaXG: desired_spec |= Midi_Spec_XG; break;
                case KMS_RolandGS: desired_spec |= Midi_Spec_GS|Midi_Spec_SC; break;
                }

                if (cs.percussion) {
                    Midi_Program_Id bank_id(true, 0, cs.pgm, 0);
                    const Midi_Program *bnk = Midi_Data::get_bank(bank_id, desired_spec, &patch_spec);
                    patch_name = bnk ? bnk->bank_name : nullptr;
                }
                else {
                    Midi_Program_Id pgm_id(false, cs.ctl[0], cs.ctl[32], cs.pgm);
                    const Midi_Program *pgm = Midi_Data::get_program(pgm_id, desired_spec, &patch_spec);
                    pgm = pgm ? pgm : Midi_Data::get_fallback_program(pgm_id, desired_spec, &patch_spec);
                    patch_name = pgm ? pgm->patch_name : nullptr;
                }

                tp.fg = pal[Colors::text_high_brightness];
                tp.font = &font_fmdsp_small;
                tp.pos = r.origin();
                switch (patch_spec) {
                default:
                    tp.draw_char(U'G');
                    tp.pos.y += 6;
                    tp.draw_char(U'M');
                    break;
                case Midi_Spec_XG:
                    tp.draw_char(U'X');
                    tp.pos.y += 6;
                    tp.draw_char(U'G');
                    break;
                case Midi_Spec_GS:
                    tp.draw_char(U'G');
                    tp.pos.y += 6;
                    tp.draw_char(U'S');
                    break;
                case Midi_Spec_SC:
                    tp.draw_char(U'S');
                    tp.pos.y += 6;
                    tp.draw_char(U'C');
                    break;
                }
            }

            if ((paint & Pt_Foreground) && patch_name) {
                Rect r = lo.channel_instrument_label[ch];
                tp.font = &font_s12;
                tp.pos = r.origin();
                tp.draw_utf8(gsl::ensure_z(patch_name));
            }

            if (paint & Pt_Foreground) {
                char text[8];
                sprintf(text, "%3u", cs.ctl[0x07]);
                draw_text_rect(lo.channel_volume_value[ch], text, pal[Colors::text_high_brightness]);
                sprintf(text, "%3d", cs.ctl[0x0a] - 64);
                draw_text_rect(lo.channel_pan_value[ch], text, pal[Colors::text_high_brightness]);
            }
        }
    }

    // Draw file info
    File_Browser &fb = *file_browser_;
    Metadata_Display &mdd = *metadata_display_;

    if (paint & Pt_Background)
        draw_text_rect(lo.file_label, lo.file_label.text, pal[Colors::text_low_brightness]);

    if (paint & Pt_Foreground) {
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.file_dir_path.bounds);
        if (info_mode_ == Info_File)
            draw_text_rect(lo.file_dir_path, get_display_path(fb.cwd()), pal[Colors::text_high_brightness]);
        else {
            gsl::cstring_span file_dir = path_directory(ps.file_path);
            draw_text_rect(lo.file_dir_path, get_display_path(file_dir), pal[Colors::text_high_brightness]);
        }
        SDLpp_RestoreClipState(rr, clip);

        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.file_num_label.bounds);
        draw_text_rect(lo.file_num_label, std::to_string(fb.current_index() + 1) + " / " + std::to_string(fb.current_count()), pal[Colors::text_high_brightness]);
        SDLpp_RestoreClipState(rr, clip);
    }
    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, pal[Colors::text_low_brightness]);
        SDLpp_RenderDrawHLine(rr, lo.info_underline.p1.x, lo.info_underline.p2.x, lo.info_underline.p1.y);
    }

    // Draw info panel
    if (paint & Pt_Foreground) {
        switch (info_mode_) {
        case Info_File:
            fb.paint(rr);
            break;
        case Info_Metadata:
            mdd.update_data(ps.song_metadata);
            mdd.paint(rr);
            break;
        default:
            break;
        }
    }

    if (paint & Pt_Foreground && !modal_.empty()) {
        Rect bounds(0, 0, size_.x, size_.y);
        unsigned alpha = 0x80;
        SDL_SetRenderDrawColor(rr, 0x00, 0x00, 0x00, alpha);
        SDL_RenderFillRect(rr, &bounds);
        Modal_Box &modal = *modal_.back();
        modal.paint(rr);
    }

    if (paint & Pt_Foreground && fadeout_engaged_) {
        Rect bounds(0, 0, size_.x, size_.y);
        unsigned alpha = 0xff - 0xff * std::max(0, fadeout_time_) / fadeout_delay;
        SDL_SetRenderDrawColor(rr, 0x00, 0x00, 0x00, alpha);
        SDL_RenderFillRect(rr, &bounds);
    }
}

void Application::paint_cached_background(SDL_Renderer *rr)
{
    SDL_Texture *bg = cached_background_.get();

    if (!bg) {
        // clear font caches between paints on differents renderers
        Text_Painter::clear_font_caches();

        SDL_Surface *su = SDLpp_CreateRGBA32Surface(size_.x, size_.y);
        if (!su)
            throw std::runtime_error("SDL_CreateRGBSurface");
        auto su_cleanup = gsl::finally([su] { SDL_FreeSurface(su); });

        SDL_Renderer *sr = SDL_CreateSoftwareRenderer(su);
        if (!sr)
            throw std::runtime_error("SDL_CreateSoftwareRenderer");
        auto sr_cleanup = gsl::finally([sr] { SDL_DestroyRenderer(sr); });

        paint(sr, Pt_Background);
        SDL_RenderPresent(sr);

        bg = SDL_CreateTextureFromSurface(rr, su);
        if (!bg)
            throw std::runtime_error("SDL_CreateTextureFromSurface");

        Text_Painter::clear_font_caches();

        cached_background_.reset(bg);
    }

    Rect bounds(0, 0, size_.x, size_.y);
    SDL_RenderCopy(rr, bg, &bounds, &bounds);
}

bool Application::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

    if (!modal_.empty()) {
        Modal_Box &modal = *modal_.back();
        return modal.handle_key_pressed(event);
    }

    switch (info_mode_) {
    case Info_File: {
        File_Browser &fb = *file_browser_;
        if (fb.handle_key_pressed(event))
            return true;
        if (keymod == KMOD_NONE && event.keysym.scancode == SDL_SCANCODE_SLASH) {
            if (const File_Entry *ent = fb.current_entry())
                play_random(fb.cwd(), *ent);
        }
        break;
    }
    default:
        break;
    }

    switch (event.keysym.scancode) {
    case SDL_SCANCODE_MINUS:
        if (keymod == KMOD_NONE) {
            set_scale_factor(SDL_GetWindowFromID(event.windowID), scale_factor_ - 1);
            return true;
        }
        break;
    case SDL_SCANCODE_EQUALS:
        if (keymod == KMOD_NONE) {
            set_scale_factor(SDL_GetWindowFromID(event.windowID), scale_factor_ + 1);
            return true;
        }
        break;
    case SDL_SCANCODE_PAGEUP:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Next> cmd(new Pcmd_Next);
            cmd->play_offset = -1;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_PAGEDOWN:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Next> cmd(new Pcmd_Next);
            cmd->play_offset = +1;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_SPACE:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Pause> cmd(new Pcmd_Pause);
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_HOME:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Rewind> cmd(new Pcmd_Rewind);
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_END:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Seek_End> cmd(new Pcmd_Seek_End);
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_LEFT:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Seek_Cur> cmd(new Pcmd_Seek_Cur);
            cmd->time_offset = -5;
            player_->push_command(std::move(cmd));
            return true;
        }
        else if ((keymod & KMOD_SHIFT) && !(keymod & ~KMOD_SHIFT)) {
            std::unique_ptr<Pcmd_Seek_Cur> cmd(new Pcmd_Seek_Cur);
            cmd->time_offset = -10;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_RIGHT:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Seek_Cur> cmd(new Pcmd_Seek_Cur);
            cmd->time_offset = +5;
            player_->push_command(std::move(cmd));
            return true;
        }
        else if ((keymod & KMOD_SHIFT) && !(keymod & ~KMOD_SHIFT)) {
            std::unique_ptr<Pcmd_Seek_Cur> cmd(new Pcmd_Seek_Cur);
            cmd->time_offset = +10;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_LEFTBRACKET:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Speed> cmd(new Pcmd_Speed);
            cmd->increment = -1;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_RIGHTBRACKET:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Speed> cmd(new Pcmd_Speed);
            cmd->increment = +1;
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_GRAVE:
        if (keymod == KMOD_NONE) {
            std::unique_ptr<Pcmd_Repeat_Mode> cmd(new Pcmd_Repeat_Mode);
            player_->push_command(std::move(cmd));
            return true;
        }
        break;
    case SDL_SCANCODE_TAB:
        if (keymod == KMOD_NONE) {
            info_mode_ = Info_Mode((unsigned(info_mode_) + 1) % Info_Mode_Count);
            return true;
        }
        break;
    case SDL_SCANCODE_F1:
        if (keymod == KMOD_NONE) {
            open_help_dialog();
            return true;
        }
        break;
    case SDL_SCANCODE_F2:
        if (keymod == KMOD_NONE) {
            choose_midi_output(true, last_midi_output_choice_);
            return true;
        }
        break;
    case SDL_SCANCODE_F3:
        if (keymod == KMOD_NONE) {
            choose_synth(true, last_synth_choice_);
            return true;
        }
        break;
    case SDL_SCANCODE_F4:
        if (keymod == KMOD_NONE) {
            open_fx_dialog();
            return true;
        }
        break;
    case SDL_SCANCODE_F9:
        if (keymod == KMOD_NONE) {
            choose_theme(last_theme_choice_);
            return true;
        }
        break;
    case SDL_SCANCODE_F12:
        if (keymod == KMOD_NONE) {
            open_file_externally(get_configuration_dir());
            return true;
        }
        break;
    case SDL_SCANCODE_ESCAPE:
        if (keymod == KMOD_NONE && !event.repeat) {
            if (esc_key_timer_ == 0)
                esc_key_timer_ = SDL_AddTimer(quit_by_esc_key_delay, &timer_push_event<SDL_USEREVENT + 1>, this);
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

bool Application::handle_key_released(const SDL_KeyboardEvent &event)
{
    if (!modal_.empty()) {
        Modal_Box &modal = *modal_.back();
        return modal.handle_key_released(event);
    }

    switch (info_mode_) {
    case Info_File:
        if (file_browser_->handle_key_released(event))
            return true;
        break;
    default:
        break;
    }

    switch (event.keysym.scancode) {
    default:
        break;
    case SDL_SCANCODE_ESCAPE:
        if (esc_key_timer_ != 0) {
            SDL_RemoveTimer(esc_key_timer_);
            esc_key_timer_ = 0;
        }
        break;
    }

    return false;
}

bool Application::handle_text_input(const SDL_TextInputEvent &event)
{
    if (!modal_.empty()) {
        Modal_Box &modal = *modal_.back();
        return modal.handle_text_input(event);
    }

    switch (info_mode_) {
    case Info_File:
        if (file_browser_->handle_text_input(event))
            return true;
        break;
    default:
        break;
    }

    return false;
}

void Application::play_file(const std::string &dir, const File_Entry *entries, size_t index, size_t count)
{
    std::unique_ptr<Pcmd_Play> cmd(new Pcmd_Play);

    Linear_Play_List *pll = new Linear_Play_List;
    cmd->play_list.reset(pll);

    size_t play_index = 0;
    size_t play_size = 0;

    for (size_t i = 0; i < count; ++i) {
        if (entries[i].type == 'F') {
            if (i == index)
                play_index = play_size;
            pll->add_file(dir + entries[i].name);
            ++play_size;
        }
    }

    cmd->play_index = play_index;
    player_->push_command(std::move(cmd));
}

void Application::play_random(const std::string &dir, const File_Entry &entry)
{
    std::unique_ptr<Pcmd_Play> cmd(new Pcmd_Play);

    std::string path;
    if (true) // in current browsed dir
        path = dir;
    else // in current highlighted entry
        path = (entry.type == 'D' && entry.name != "..") ? (dir + entry.name) : dir;

    Random_Play_List *pll = new Random_Play_List(path, &filter_file_name);
    cmd->play_list.reset(pll);

    player_->push_command(std::move(cmd));
}

void Application::set_current_path(const std::string &path)
{
    file_browser_->set_current_path(path);
}

bool Application::filter_file_name(const std::string &name)
{
    size_t len = name.size();
    if (len < 4 || name[len - 4] != '.')
        return false;

    char ext[3];
    memcpy(ext, &name[len - 3], 3);

    std::transform(
        ext, ext + 3, ext,
        [](char c) -> char { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; });

    return !memcmp(ext, "MID", 3) || !memcmp(ext, "RMI", 3) || !memcmp(ext, "KAR", 3);
}

bool Application::filter_file_entry(const File_Entry &ent)
{
    return ent.type != 'F' || filter_file_name(ent.name);
}

void Application::request_update()
{
    std::unique_ptr<Pcmd_Request_State> cmd(new Pcmd_Request_State);
    player_->push_command(std::move(cmd));
}

void Application::update_modals()
{
    if (modal_.empty())
        return;

    Modal_Box &modal = *modal_.back();
    if (modal.has_completed())
        modal_.pop_back();
}

void Application::open_help_dialog()
{
    const Rect bounds = Rect(0, 0, size_.x, size_.y).reduced(Point(100, 50));

    class Help_Box : public Modal_Box {
    public:
        using Modal_Box::Modal_Box;

        void paint_contents(SDL_Renderer *rr) override
        {
            Rect r = get_content_bounds();
            const Color_Palette &pal = Color_Palette::get_current();

            Text_Painter tp;
            tp.rr = rr;
            tp.font = &font_s12;
            tp.fg = pal[Colors::box_foreground];
            int fw = tp.font->width();
            int fh = tp.font->height();

            const size_t count = sizeof(help_items) / sizeof(help_items[0]);

            for (size_t i = 0; i < count; ++i) {
                Rect row = r.take_from_top(fh);
                Rect cols[2];
                cols[0] = Rect(row).take_from_left(fw * 10);
                cols[1] = Rect(row);
                cols[1].chop_from_left(cols[0].w + fw * 2);

                for (int c = 0; c < 2; ++c) {
                    SDLpp_ClipState clip;
                    SDLpp_SaveClipState(rr, clip);
                    SDL_RenderSetClipRect(rr, &cols[c]);
                    tp.pos = cols[c].origin();
                    tp.draw_utf8(!c ? help_items[i].first : help_items[i].second);
                    SDLpp_RestoreClipState(rr, clip);
                }
            }
        }

        bool handle_key_pressed(const SDL_KeyboardEvent &event) override
        {
            int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

            switch (event.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                if (keymod == KMOD_NONE) {
                    finish();
                    return true;
                }
                break;
            default:
                break;
            }

            return false;
        }
    };

    Help_Box *modal = new Help_Box(bounds, "Help");
    modal_.emplace_back(modal);
}

void Application::open_fx_dialog()
{
    const Rect bounds = Rect(0, 0, size_.x, size_.y).reduced(Point(100, 100));

    class Fx_Box : public Modal_Box {
    public:
        Fx_Box(Player_State &ps, const Rect &bounds, std::string title)
            : Modal_Box(bounds, std::move(title))
        {
            gsl::span<const Fx_Parameter> items = Synth_Fx::parameters();
            if (items.size() > 0)
                sel_ = 0;

            int *values = values_;
            for (size_t nth = 0; nth < items.size(); ++nth)
                values[nth] = ps.fx_parameters[nth];
        }

        void paint_contents(SDL_Renderer *rr) override
        {
            Rect r = get_content_bounds();
            const Color_Palette &pal = Color_Palette::get_current();

            Text_Painter tp;
            tp.rr = rr;
            tp.font = &font_s12;
            int fw = tp.font->width();
            int fh = tp.font->height();

            gsl::span<const Fx_Parameter> items = Synth_Fx::parameters();
            const size_t sel = sel_;
            const int *values = values_;

            for (size_t nth = 0; nth < items.size(); ++nth) {
                Rect row = r.take_from_top(fh);

                if (items[nth].flags & Fx_Parameter::HasSeparator)
                    row = r.take_from_top(fh);

                Rect cols[2];
                cols[0] = Rect(row).take_from_left(fw * 16);
                cols[1] = Rect(row);
                cols[1].chop_from_left(cols[0].w + fw * 2);

                tp.pos = cols[0].origin();
                tp.fg = pal[Colors::box_foreground];
                if (nth == sel) {
                    tp.fg = pal[Colors::box_active_item_foreground];
                    SDLpp_SetRenderDrawColor(rr, pal[Colors::box_active_item_background]);
                    SDL_RenderFillRect(rr, &cols[0]);
                }
                tp.draw_utf8(items[nth].name);

                switch (items[nth].type) {
                default:
                case Fx_Parameter::Integer:
                    for (int i = 0, j = values[nth]; i < 100; ++i) {
                        if (i < j)
                            SDLpp_SetRenderDrawColor(rr, pal[Colors::box_foreground]);
                        else
                            SDLpp_SetRenderDrawColor(rr, pal[Colors::box_foreground_secondary]);
                        SDLpp_RenderDrawVLine(rr, cols[1].x + 2 * i, cols[1].y + 1, cols[1].bottom() - 1);
                    }
                    break;
                case Fx_Parameter::Boolean:
                    tp.pos = cols[1].origin();
                    tp.fg = pal[Colors::box_foreground];
                    tp.draw_utf8("\u3010");
                    if (values[nth] > 0) {
                        tp.draw_utf8("On");
                    }
                    else {
                        tp.fg = pal[Colors::box_foreground_secondary];
                        tp.draw_utf8("Off");
                    }
                    tp.fg = pal[Colors::box_foreground];
                    tp.draw_utf8("\u3011");
                    break;
                }
            }
        }

        bool handle_key_pressed(const SDL_KeyboardEvent &event) override
        {
            gsl::span<const Fx_Parameter> items = Synth_Fx::parameters();
            int *values = values_;

            int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);

            switch (event.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                if (keymod == KMOD_NONE) {
                    finish();
                    return true;
                }
                break;
            case SDL_SCANCODE_UP: {
                size_t sel = sel_;
                if (sel != ~size_t(0) && sel > 0)
                    sel_ = sel - 1;
                return true;
            }
            case SDL_SCANCODE_DOWN: {
                size_t sel = sel_;
                if (sel != ~size_t(0) && sel + 1 < items.size())
                    sel_ = sel + 1;
                return true;
            }
            case SDL_SCANCODE_PAGEUP: {
                size_t sel = sel_;
                if (sel != ~size_t(0))
                    sel_ = std::max<ssize_t>(0, sel - 10);
                return true;
            }
            case SDL_SCANCODE_PAGEDOWN: {
                size_t sel = sel_;
                if (sel != ~size_t(0))
                    sel_ = std::min(items.size() - 1, sel + 10);
                return true;
            }
            case SDL_SCANCODE_HOME: {
                size_t sel = sel_;
                if (sel != ~size_t(0))
                    sel_ = 0;
                return true;
            }
            case SDL_SCANCODE_END: {
                size_t sel = sel_;
                if (sel != ~size_t(0))
                    sel_ = items.size() - 1;
                return true;
            }
            case SDL_SCANCODE_LEFT: {
                size_t sel = sel_;
                int amount = (keymod & KMOD_SHIFT) ? 5 : 1;
                if (sel != ~size_t(0)) {
                    const Fx_Parameter &fxp = items[sel];
                    if (fxp.type == Fx_Parameter::Boolean)
                        change_value(sel, 0);
                    else
                        change_value(sel, std::max(0, values[sel] - amount));
                }
                return true;
            }
            case SDL_SCANCODE_RIGHT: {
                size_t sel = sel_;
                int amount = (keymod & KMOD_SHIFT) ? 5 : 1;
                if (sel != ~size_t(0)) {
                    const Fx_Parameter &fxp = items[sel];
                    if (fxp.type == Fx_Parameter::Boolean)
                        change_value(sel, 100);
                    else
                        change_value(sel, std::min(100, values[sel] + amount));
                }
                return true;
            }
            case SDL_SCANCODE_BACKSPACE: {
                size_t sel = sel_;
                if (sel != ~size_t(0))
                    change_value(sel, items[sel].default_value);
                return true;
            }
            default:
                break;
            }

            return false;
        }

        void change_value(size_t index, int value)
        {
            values_[index] = value;
            if (ValueChangeCallback)
                ValueChangeCallback(index, value);
        }

        std::function<void(size_t, int)> ValueChangeCallback;

    private:
        size_t sel_ = ~size_t(0);
        int values_[Synth_Fx::Parameter_Count] {};
    };

    Fx_Box *modal = new Fx_Box(*ps_, bounds, "Global effects");
    modal_.emplace_back(modal);

    modal->ValueChangeCallback = [this](size_t index, int value) {
        std::unique_ptr<Pcmd_Set_Fx_Parameter> cmd(new Pcmd_Set_Fx_Parameter);
        cmd->index = index;
        cmd->value = value;
        player_->push_command(std::move(cmd));
    };
}

void Application::choose_midi_output(bool ask, gsl::cstring_span choice)
{
    std::vector<Midi_Output> outputs;
    get_midi_outputs(outputs);

    std::vector<std::string> choices;
    choices.reserve(1 + outputs.size());

    size_t default_index = ~size_t(0);

    choices.push_back("(no device)");
    for (const Midi_Output &x : outputs) {
        std::string name = x.name;
        if (default_index == ~size_t(0) && name == choice)
            default_index = choices.size();
        choices.push_back(std::move(name));
    }

    auto choose =
        [this, choices, outputs](size_t index) {
            if (index >= choices.size())
                return;

            last_midi_output_choice_ = (index > 0) ? choices[index] : std::string();

            std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
            if (!ini) ini = create_configuration();
            ini->SetValue("", "midi-out-device", last_midi_output_choice_.c_str(), nullptr, true);
            save_global_configuration(*ini);

            if (index < choices.size()) {
                std::unique_ptr<Pcmd_Set_Midi_Output> cmd(new Pcmd_Set_Midi_Output);
                if (index > 0)
                    cmd->midi_output_id = outputs[index - 1].id;
                player_->push_command(std::move(cmd));
                return;
            }
        };

    if (!ask)
        choose(default_index);
    else {
        Modal_Selection_Box *modal = new Modal_Selection_Box(
            Rect(0, 0, size_.x, size_.y).reduced(Point(100, 100)), "MIDI device", choices);
        modal_.emplace_back(modal);

        if (default_index != ~size_t(0))
            modal->set_selection_index(default_index);

        modal->CompletionCallback =
            [modal, choose] {
                size_t index = modal->completion_result<size_t>(0);
                choose(index);
            };
    }
}

void Application::choose_synth(bool ask, gsl::cstring_span choice)
{
    const std::vector<Synth_Host::Plugin_Info> &plugins = Synth_Host::plugins();

    std::vector<std::string> choices;
    choices.reserve(1 + plugins.size());

    size_t default_index = ~size_t(0);

    choices.push_back("(no device)");
    for (const Synth_Host::Plugin_Info &plugin : plugins) {
        std::string name = plugin.name;
        if (default_index == ~size_t(0) && name == choice)
            default_index = choices.size();
        choices.push_back(std::move(name));
    }

    auto choose =
        [this, choices, plugins](size_t index) {
            if (index >= choices.size())
                return;

            last_synth_choice_ = (index > 0) ? choices[index] : std::string();

            std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
            if (!ini) ini = create_configuration();
            ini->SetValue("", "synth-device", last_synth_choice_.c_str(), nullptr, true);
            save_global_configuration(*ini);

            if (index < choices.size()) {
                std::unique_ptr<Pcmd_Set_Synth> cmd(new Pcmd_Set_Synth);
                if (index > 0)
                    cmd->synth_plugin_id = plugins[index - 1].id;
                player_->push_command(std::move(cmd));
                return;
            }
        };

    if (!ask)
        choose(default_index);
    else {
        Modal_Selection_Box *modal = new Modal_Selection_Box(
            Rect(0, 0, size_.x, size_.y).reduced(Point(100, 100)), "Synthesis device", choices);
        modal_.emplace_back(modal);

        if (default_index != ~size_t(0))
            modal->set_selection_index(default_index);

        modal->CompletionCallback =
            [modal, choose] {
                size_t index = modal->completion_result<size_t>(0);
                choose(index);
            };
    }
}

void Application::get_midi_outputs(std::vector<Midi_Output> &outputs)
{
    std::unique_ptr<Pcmd_Get_Midi_Outputs> cmd(new Pcmd_Get_Midi_Outputs);
    std::mutex wait_mutex;
    std::condition_variable wait_cond;

    cmd->midi_outputs = &outputs;
    cmd->wait_mutex = &wait_mutex;
    cmd->wait_cond = &wait_cond;

    std::unique_lock<std::mutex> lock(wait_mutex);
    player_->push_command(std::move(cmd));
    wait_cond.wait(lock);
}

void Application::choose_theme(gsl::cstring_span choice)
{
    std::vector<std::string> themes;
    get_themes(themes);

    size_t default_index = ~size_t(0);
    for (size_t i = 0, n = themes.size(); i < n && default_index == ~size_t(0); ++i) {
        if (themes[i] == choice)
            default_index = i;
    }

    Modal_Selection_Box *modal = new Modal_Selection_Box(
        Rect(0, 0, size_.x, size_.y).reduced(Point(100, 100)), "Theme", themes);
    modal_.emplace_back(modal);

    if (default_index != ~size_t(0))
        modal->set_selection_index(default_index);

    modal->CompletionCallback =
        [this, modal, themes] {
            size_t index = modal->completion_result<size_t>(0);
            if (index >= themes.size())
                return;
            load_theme(themes[index]);
        };
}

void Application::get_themes(std::vector<std::string> &themes)
{
    std::string dir_path = get_configuration_dir();
    Dir dir{dir_path};

    if (!dir)
        return;

    std::string file_name;
    while (dir.read_next(file_name)) {
        bool is_theme = string_starts_with<char>(file_name, "t_") &&
            string_ends_with<char>(file_name, ".ini");
        if (is_theme)
            themes.push_back(file_name.substr(2, file_name.size() - 6));
    }
}

void Application::load_theme(gsl::cstring_span theme)
{
    load_default_theme();

    if (theme.empty())
        theme = "default";

    Log::i("Loading theme: %s", gsl::to_string(theme).c_str());

    if (theme != "default") {
        std::string filename = "t_" + gsl::to_string(theme);
        std::unique_ptr<CSimpleIniA> ini = load_configuration(filename);
        if (!ini) {
            Log::e("Cannot load theme file: %s.ini", filename.c_str());
            return;
        }

        load_theme_configuration(*ini);
    }

    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini) ini = create_configuration();
    ini->SetValue("", "theme", gsl::to_string(theme).c_str(), nullptr, true);
    save_global_configuration(*ini);

    last_theme_choice_ = gsl::to_string(theme);
}

void Application::load_default_theme()
{
    cached_background_.reset();

    Color_Palette &pal = Color_Palette::get_current();
    pal = Color_Palette::create_default();

    static const uint8_t png_data[] = {
        #include "theme-image.dat"
    };
    size_t png_size = sizeof(png_data);

    logo_image_.reset(IMG_Load_RW(SDL_RWFromConstMem(png_data, png_size), true));
    wallpaper_image_.reset();
}

void Application::load_theme_configuration(const CSimpleIniA &ini)
{
    Color_Palette &pal = Color_Palette::get_current();
    if (!pal.load(ini, "color"))
        Log::e("Cannot load the color palette");

    //
    struct Image_Assoc {
        const char *key;
        SDLpp_Surface_u *image;
    };

    const Image_Assoc image_assoc[] = {
        {"logo", &logo_image_},
        {"wallpaper", &wallpaper_image_},
    };

    for (const Image_Assoc &ia : image_assoc) {
        std::string path;
        if (const char *value = ini.GetValue("image", ia.key))
            path.assign(value);
        if (!path.empty()) {
            if (!is_path_absolute(path))
                path = get_configuration_dir() + path;
            SDL_Surface *image = IMG_Load_RW(SDL_RWFromFile(path.c_str(), "rb"), true);
            ia.image->reset(image);
        }
    }
}

void Application::engage_shutdown()
{
    Log::i("Engage shutdown");

    if (!fadeout_engaged_) {
        fadeout_engaged_ = true;
        fadeout_time_ = fadeout_delay;

        std::mutex wait_mutex;
        std::condition_variable wait_cond;
        std::unique_lock<std::mutex> lock(wait_mutex);
        std::unique_ptr<Pcmd_Shutdown> cmd(new Pcmd_Shutdown);

        cmd->wait_mutex = &wait_mutex;
        cmd->wait_cond = &wait_cond;
        player_->push_command(std::move(cmd));
        wait_cond.wait(lock);
    }
}

void Application::engage_shutdown_if_esc_key()
{
    if (esc_key_timer_ == 0)
        return;

    SDL_RemoveTimer(esc_key_timer_);
    esc_key_timer_ = 0;

    engage_shutdown();
}

void Application::advance_shutdown()
{
    if (fadeout_engaged_)
        --fadeout_time_;
}

bool Application::should_quit() const
{
    return fadeout_engaged_ && fadeout_time_ < 0;
}

std::unique_ptr<CSimpleIniA> Application::initialize_config()
{
    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini) ini = create_configuration();

    bool ini_update = false;

    if (!ini->GetValue("", "initial-path")) {
        ini->SetValue("", "initial-path", "", "; Path of directory opened initially");
        ini_update = true;
    }

    if (!ini->GetValue("", "midi-out-device")) {
        ini->SetValue("", "midi-out-device", "", "; Selected device for MIDI output");
        ini_update = true;
    }

    if (!ini->GetValue("", "synth-device")) {
        const std::vector<Synth_Host::Plugin_Info> &plugins = Synth_Host::plugins();
        const char *default_synth_name = nullptr;

        for (size_t i = 0, n = plugins.size(); i < n && !default_synth_name; ++i) {
            const Synth_Host::Plugin_Info &info = plugins[i];
            if (info.id == default_synth_id)
                default_synth_name = info.name.c_str();
        }

        ini->SetValue("", "synth-device", default_synth_name ? default_synth_name : "", "; Selected device for synthesis");
        ini_update = true;
    }

    if (!ini->GetValue("", "synth-sample-rate")) {
        ini->SetDoubleValue("", "synth-sample-rate", 44100, "; Sample rate of synthesized audio stream (Hz) [22050:192000]");
        ini_update = true;
    }

    if (!ini->GetValue("", "synth-audio-latency")) {
        ini->SetDoubleValue("", "synth-audio-latency", 50, "; Latency of synthesized audio stream (ms) [1:500]");
        ini_update = true;
    }

    if (!ini->GetValue("", "theme")) {
        ini->SetValue("", "theme", "default", "; Theme of the graphical interface");
        ini_update = true;
    }

    if (!ini->GetValue("", "force-software-renderer")) {
        ini->SetBoolValue("", "force-software-renderer", false, "; Force software rendering in case of graphical problems");
        ini_update = true;
    }

    if (ini_update)
        save_global_configuration(*ini);

    {
        std::unique_ptr<CSimpleIniA> theme_ini = create_configuration();
        theme_ini->SetFileComment(
            "; This document describes the default theme." "\n"
            "; Important: do not edit this file directly! changes will be lost." "\n"
            "; If you want to design a theme, duplicate this file with a different name.");
        theme_ini->SetValue("image", "logo", "", "; Image file to use as a logo");
        theme_ini->SetValue("image", "wallpaper", "", "; Image file to use as a wallpaper");
        Color_Palette::save_defaults(*theme_ini, "color", true);
        save_configuration("t_default", *theme_ini);
    }

    return ini;
}

void Application::receive_state_in_other_thread(const Player_State &ps)
{
    std::unique_lock<std::mutex> lock(ps_mutex_, std::try_to_lock);
    if (lock.owns_lock())
        *ps_ = ps;
}
