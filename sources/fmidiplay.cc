#include "fmidiplay.h"
#include "player/player.h"
#include "player/playlist.h"
#include "player/command.h"
#include "data/ins_names.h"
#include "ui/main_layout.h"
#include "ui/file_browser.h"
#include "ui/file_entry.h"
#include "ui/metadata_display.h"
#include "ui/piano.h"
#include "ui/modal_box.h"
#include "ui/text.h"
#include "ui/fonts.h"
#include "ui/color_palette.h"
#include "ui/paint.h"
#include "utility/SDL++.h"
#include "utility/paths.h"
#include <gsl.hpp>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

const char Application::title_[] = "FMidiPlay";
const Point Application::size_{640, 400};

static constexpr unsigned fadeout_delay = 10;

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

    std::string home = get_home_directory();
    if (!home.empty())
        fb->set_cwd(home);

    Player *pl = new Player;
    player_.reset(pl);

    ps_.reset(new Player_State);
    pl->StateCallback = [this](const Player_State &ps)
                            { receive_state_in_other_thread(ps); };

    uint32_t update_interval = 50;
    update_timer_ = SDL_AddTimer(update_interval, &timer_push_event<SDL_USEREVENT>, this);
    if (update_timer_ == 0)
        throw std::runtime_error("SDL_AddTimer");
}

Application::~Application()
{
    if (update_timer_)
        SDL_RemoveTimer(update_timer_);
    SDL_DestroyTexture(cached_background_);
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

    SDLpp_ClipState clip;

    SDL_RenderSetScale(rr, scale_factor_, scale_factor_);
    SDL_SetRenderDrawBlendMode(rr, SDL_BLENDMODE_BLEND);

    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, Color_Palette::background);
        SDL_RenderClear(rr);
    }

    #pragma message("XXX remove me")
    if (paint & Pt_Background) {
        Rect r(size_.x - 32, 2, 10, 10);
        SDL_SetRenderDrawColor(rr, 0xff, 0x00, 0x00, 0xff);
        SDL_RenderDrawRect(rr, &r);
        r = r.off_by(Point(10, 10));
        SDL_SetRenderDrawColor(rr, 0x00, 0xff, 0x00, 0xff);
        SDL_RenderDrawRect(rr, &r);
        r = r.off_by(Point(10, 10));
        SDL_SetRenderDrawColor(rr, 0x00, 0x00, 0xff, 0xff);
        SDL_RenderDrawRect(rr, &r);
    }

    if (paint & Pt_Background)
        RenderFillAlternating(rr, lo.info_box, Color_Palette::info_box_background, Color_Palette::transparent());

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

    auto draw_text_rect = [rr](const Main_Layout::Text_Rect &tr, gsl::cstring_span str, const SDL_Color &color) {
                              Text_Painter tp;
                              tp.rr = rr;
                              tp.fg = color;
                              tp.pos = tr.bounds.origin();
                              tp.font = tr.font;
                              tp.draw_utf8(str);
                          };

    // Draw top
    if (paint & Pt_Background) {
        draw_text_rect(lo.time_label, lo.time_label.text, Color_Palette::text_low_brightness);
        draw_text_rect(lo.time_value, lo.time_value.text, Color_Palette::digit_off);
    }
    if (paint & Pt_Foreground)
        draw_text_rect(lo.time_value, hms(buf_hms, ps.time_position), Color_Palette::digit_on);

    if (paint & Pt_Background) {
        draw_text_rect(lo.length_label, lo.length_label.text, Color_Palette::text_low_brightness);
        draw_text_rect(lo.length_value, lo.length_value.text, Color_Palette::digit_off);
    }
    if (paint & Pt_Foreground)
        draw_text_rect(lo.length_value, hms(buf_hms, ps.duration), Color_Palette::digit_on);
    if (paint & Pt_Background)
        draw_text_rect(lo.playing_label, lo.playing_label.text, Color_Palette::text_low_brightness);
    if (paint & Pt_Foreground) {
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.playing_value.bounds);
        draw_text_rect(lo.playing_value, path_file_name(ps.file_path), Color_Palette::text_high_brightness);
        SDLpp_RestoreClipState(rr, clip);
    }
    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, Color_Palette::text_min_brightness);
        SDL_RenderFillRect(rr, &lo.playing_progress);
    }
    if (paint & Pt_Foreground) {
        SDLpp_SetRenderDrawColor(rr, Color_Palette::text_high_brightness);
        Rect r = lo.playing_progress;
        r.w = int(lo.playing_progress.w * ps.time_position / ps.duration + 0.5);
        SDL_RenderFillRect(rr, &r);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.tempo_label, lo.tempo_label.text, Color_Palette::text_low_brightness);
        draw_text_rect(lo.tempo_value, lo.tempo_value.text, Color_Palette::digit_off);
    }
    if (paint & Pt_Foreground) {
        char tempo_str[4] = "---";
        long tempo_val = long(ps.tempo + 0.5);
        if (tempo_val >= 0 && tempo_val < 1000)
            sprintf(tempo_str, "%03ld", tempo_val);
        draw_text_rect(lo.tempo_value, tempo_str, Color_Palette::digit_on);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.speed_label, lo.speed_label.text, Color_Palette::text_low_brightness);
        draw_text_rect(lo.speed_value, lo.speed_value.text, Color_Palette::digit_off);
    }
    if (paint & Pt_Foreground) {
        char speed_str[4] = "---";
        unsigned speed_val = ps.speed;
        if (speed_val < 1000)
            sprintf(speed_str, "%03u", speed_val);
        draw_text_rect(lo.speed_value, speed_str, Color_Palette::digit_on);
    }
    if (paint & Pt_Background) {
        draw_text_rect(lo.repeat_label, lo.repeat_label.text, Color_Palette::digit_off);
        draw_text_rect(lo.multi_label, lo.multi_label.text, Color_Palette::digit_off);
    }
    if (paint & Pt_Foreground) {
        if ((ps.repeat_mode & (Repeat_On|Repeat_Off)) == Repeat_On)
            draw_text_rect(lo.repeat_label, lo.repeat_label.text, Color_Palette::digit_on);
        if ((ps.repeat_mode & (Repeat_Multi|Repeat_Single)) == Repeat_Multi)
            draw_text_rect(lo.multi_label, lo.multi_label.text, Color_Palette::digit_on);
    }

    // Draw MIDI channel info heading
    {
        if (paint & Pt_Background) {
            SDLpp_SetRenderDrawColor(rr, Color_Palette::text_min_brightness);
            SDLpp_RenderDrawLine(rr, lo.channel_heading_underline.p1, lo.channel_heading_underline.p2);

            draw_text_rect(lo.octkb_label, lo.octkb_label.text, Color_Palette::text_low_brightness);
            draw_text_rect(lo.instrument_label, lo.instrument_label.text, Color_Palette::text_low_brightness);
        }
    }

    // Draw MIDI channel info
    for (unsigned ch = 0; ch < 16; ++ch) {
        const Channel_State &cs = ps.kb.channel[ch];

        Piano piano(lo.channel_piano[ch]);
        piano.set_keys(cs.key);
        piano.paint(rr, paint);

        if (paint & Pt_Background) {
            SDLpp_SetRenderDrawColor(rr, Color_Palette::text_min_brightness);
            SDLpp_RenderDrawDottedHLine(rr, lo.channel_underline[ch].p1.x, lo.channel_underline[ch].p2.x, lo.channel_underline[ch].p1.y);
        }

        if (paint & Pt_Background) {
            Rect r = lo.channel_number_label[ch];
            Text_Painter tp;
            tp.rr = rr;
            tp.font = &font_fmdsp_small;

            tp.fg = Color_Palette::text_high_brightness;
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
            unsigned patch_spec = kMidiSpecGM1;

            if (paint & Pt_Foreground) {
                Rect r = lo.channel_midispec_label[ch];

                unsigned desired_spec = kMidiSpecGM1;
                switch (ps.kb.midispec) {
                default: break;
                case KMS_GeneralMidi2: desired_spec |= kMidiSpecGM2; break;
                case KMS_YamahaXG: desired_spec |= kMidiSpecXG; break;
                case KMS_RolandGS: desired_spec |= kMidiSpecGS|kMidiSpecSC; break;
                }

                if (cs.percussion) {
                    MidiProgramId bank_id(true, 0, cs.pgm, 0);
                    const MidiProgram *bnk = getMidiBank(bank_id, desired_spec, &patch_spec);
                    patch_name = bnk ? bnk->bankName : nullptr;
                }
                else {
                    MidiProgramId pgm_id(false, cs.ctl[0], cs.ctl[32], cs.pgm);
                    const MidiProgram *pgm = getMidiProgram(pgm_id, desired_spec, &patch_spec);
                    pgm = pgm ? pgm : getFallbackProgram(pgm_id, desired_spec, &patch_spec);
                    patch_name = pgm ? pgm->patchName : nullptr;
                }

                tp.fg = Color_Palette::text_high_brightness;
                tp.font = &font_fmdsp_small;
                tp.pos = r.origin();
                switch (patch_spec) {
                default:
                    tp.draw_char(U'G');
                    tp.pos.y += 6;
                    tp.draw_char(U'M');
                    break;
                case kMidiSpecXG:
                    tp.draw_char(U'X');
                    tp.pos.y += 6;
                    tp.draw_char(U'G');
                    break;
                case kMidiSpecGS:
                    tp.draw_char(U'G');
                    tp.pos.y += 6;
                    tp.draw_char(U'S');
                    break;
                case kMidiSpecSC:
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
        }
    }

    // Draw file info
    File_Browser &fb = *file_browser_;
    Metadata_Display &mdd = *metadata_display_;

    if (paint & Pt_Background)
        draw_text_rect(lo.file_label, lo.file_label.text, Color_Palette::text_low_brightness);

    if (paint & Pt_Foreground) {
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.file_dir_path.bounds);
        if (info_mode_ == Info_File)
            draw_text_rect(lo.file_dir_path, get_display_path(fb.cwd()), Color_Palette::text_high_brightness);
        else {
            gsl::cstring_span file_dir = path_directory(ps.file_path);
            draw_text_rect(lo.file_dir_path, get_display_path(file_dir), Color_Palette::text_high_brightness);
        }
        SDLpp_RestoreClipState(rr, clip);

        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &lo.file_num_label.bounds);
        draw_text_rect(lo.file_num_label, std::to_string(fb.current_index() + 1) + " / " + std::to_string(fb.current_count()), Color_Palette::text_high_brightness);
        SDLpp_RestoreClipState(rr, clip);
    }
    if (paint & Pt_Background) {
        SDLpp_SetRenderDrawColor(rr, Color_Palette::text_low_brightness);
        SDLpp_RenderDrawLine(rr, lo.info_underline.p1, lo.info_underline.p2);
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
    SDL_Texture *bg = cached_background_;

    if (!bg) {
        // clear font caches between paints on differents renderers
        Text_Painter::clear_font_caches();
        auto font_cache_cleanup = gsl::finally([] { Text_Painter::clear_font_caches(); });

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

        cached_background_ = bg;
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
        if (keymod == KMOD_NONE && event.keysym.scancode == SDL_SCANCODE_SLASH)
            play_random(fb.cwd(), fb.entry(fb.current_index()));
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
        else if (keymod & KMOD_SHIFT) {
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
        else if (keymod & KMOD_SHIFT) {
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
    case SDL_SCANCODE_ESCAPE:
        if (keymod == KMOD_NONE && !event.repeat) {
            engage_shutdown();
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

void Application::set_current_file(const std::string &path)
{
    file_browser_->set_current_file(path);
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

    return !memcmp(ext, "MID", 3) || !memcmp(ext, "KAR", 3);
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

void Application::engage_shutdown()
{
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

void Application::advance_shutdown()
{
    if (fadeout_engaged_)
        --fadeout_time_;
}

bool Application::should_quit() const
{
    return fadeout_engaged_ && fadeout_time_ < 0;
}

void Application::receive_state_in_other_thread(const Player_State &ps)
{
    std::unique_lock<std::mutex> lock(ps_mutex_, std::try_to_lock);
    if (lock.owns_lock())
        *ps_ = ps;
}
