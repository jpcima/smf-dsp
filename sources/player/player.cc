#include "player.h"
#include "playlist.h"
#include "instrument.h"
#include "command.h"
#include "clock.h"
#include "synth/synth_host.h"
#include "utility/charset.h"
#include <gsl.hpp>
#include <stdexcept>
#include <cstdio>

Player::Player()
    : quit_(false),
      play_list_(new Linear_Play_List),
      midiport_ins_(new Midi_Port_Instrument),
      synth_ins_(new Midi_Synth_Instrument)
{
    ins_ = midiport_ins_.get();

    // scan and initialize plugins
    Synth_Host::plugins();

    std::unique_lock<std::mutex> ready_lock(ready_mutex_);
    thread_ = std::thread([this] { thread_exec(); });
    ready_cv_.wait(ready_lock);
}

Player::~Player()
{
    std::unique_lock<std::mutex> lock(ready_mutex_);
    quit_.store(true);
    uv_async_send(async_);
    ready_cv_.wait(lock);
    thread_.join();
}

void Player::push_command(std::unique_ptr<Player_Command> cmd)
{
    std::unique_lock<std::mutex> lock(cmd_mutex_);
    cmd_queue_.push(std::move(cmd));
    lock.unlock();
    uv_async_send(async_);
}

void Player::thread_exec()
{
#if UV_VERSION_MAJOR >= 1
    uv_loop_t loop_buf;
    uv_loop_t *loop = &loop_buf;
    if (uv_loop_init(loop) != 0)
        throw std::runtime_error("uv_loop_init");
    auto loop_cleanup = gsl::finally([loop] { uv_loop_close(loop); });
#else
    uv_loop_t *loop = uv_loop_new();
    if (!loop)
        throw std::runtime_error("uv_loop_new");
    auto loop_cleanup = gsl::finally([loop] { uv_loop_delete(loop); });
#endif

    uv_async_t async;
#if UV_VERSION_MAJOR >= 1
    uv_async_init(loop, &async, nullptr);
#else
    uv_async_init(loop, &async, +[](uv_async_t *, int) {});
#endif
    async_ = &async;

    Player_Clock clock(loop);
    clock.TimerCallback = [this](uint64_t elapsed) { tick(elapsed); };
    clock_ = &clock;

    {
        std::lock_guard<std::mutex> lock(ready_mutex_);
        ready_cv_.notify_one();
    }

    while (!quit_.load()) {
        process_command_queue();
        uv_run(loop, UV_RUN_ONCE);
    }

    std::lock_guard<std::mutex> lock(ready_mutex_);
    async_ = nullptr;
    clock_ = nullptr;
    ready_cv_.notify_one();
}

void Player::process_command_queue()
{
    std::unique_lock<std::mutex> lock(cmd_mutex_, std::defer_lock);

    while (!cmd_queue_.empty()) {
        lock.lock();
        std::unique_ptr<Player_Command> cmd = std::move(cmd_queue_.front());
        cmd_queue_.pop();
        lock.unlock();

        if (quit_.load())
            return;

        switch (cmd->type()) {
        case PC_Play: {
            Play_List *pll = static_cast<Pcmd_Play &>(*cmd).play_list.release();
            size_t index = static_cast<Pcmd_Play &>(*cmd).play_index;
            play_list_.reset(pll);

            pll->start();
            for (size_t i = 0; i < index; ++i)
                pll->go_next();

            resume_play_list();
            break;
        }
        case PC_Next: {
            Play_List &pll = *play_list_;
            bool m = false;
            int n = static_cast<Pcmd_Next &>(*cmd).play_offset;
            for (; n > 0 && pll.go_next(); --n) m = true;
            for (; n < 0 && pll.go_previous(); ++n) m = true;
            if (m)
                resume_play_list();
            break;
        }
        case PC_Pause:
            if (pl_) {
                Player_Clock &c = *clock_;
                if (!c.active())
                    c.start(1);
                else {
                    c.stop();
                    ins_->all_sound_off();
                }
            }
            break;
        case PC_Rewind:
            rewind();
            break;
        case PC_Seek_End: {
            fmidi_player_t *pl = pl_.get();
            if (pl)
                fmidi_player_goto_time(pl, smf_duration_);
            break;
        }
        case PC_Seek_Cur: {
            fmidi_player_t *pl = pl_.get();
            if (pl) {
                double o = static_cast<Pcmd_Seek_Cur &>(*cmd).time_offset;
                double t = fmidi_player_current_time(pl) + o;
                t = std::max(t, 0.0);
                t = std::min(t, smf_duration_);
                fmidi_player_goto_time(pl, t);
            }
            break;
        }
        case PC_Speed: {
            fmidi_player_t *pl = pl_.get();
            if (pl) {
                unsigned cur = (unsigned)(0.5 + fmidi_player_current_speed(pl) * 100);
                int speed = static_cast<int>(cur) + static_cast<Pcmd_Speed &>(*cmd).increment;
                speed = std::max(speed, 1);
                speed = std::min(speed, 500);
                fmidi_player_set_speed(pl, speed * 0.01);
                current_speed_ = speed;
            }
            break;
        }
        case PC_Repeat_Mode:
            repeat_mode_ = Repeat_Mode((repeat_mode_ + 1) % (Repeat_Mode_Max + 1));
            break;
        case PC_Request_State:
            if (StateCallback)
                StateCallback(make_state());
            break;
        case PC_Get_Midi_Outputs: {
            std::mutex *wait_mutex = static_cast<Pcmd_Get_Midi_Outputs &>(*cmd).wait_mutex;
            std::condition_variable *wait_cond = static_cast<Pcmd_Get_Midi_Outputs &>(*cmd).wait_cond;
            *static_cast<Pcmd_Get_Midi_Outputs &>(*cmd).midi_outputs = Midi_Port_Instrument::get_midi_outputs();
            std::unique_lock<std::mutex> lock(*wait_mutex);
            wait_cond->notify_one();
            break;
        }
        case PC_Set_Midi_Output: {
            Midi_Port_Instrument &ins = *midiport_ins_;
            switch_instrument(ins);
            ins.open_midi_output(static_cast<Pcmd_Set_Midi_Output &>(*cmd).midi_output_id);
            break;
        }
        case PC_Set_Synth: {
            Midi_Synth_Instrument &ins = *synth_ins_;
            switch_instrument(ins);
            ins.open_midi_output(static_cast<Pcmd_Set_Synth &>(*cmd).synth_plugin_id);
            break;
        }
        case PC_Shutdown: {
            std::mutex *wait_mutex = static_cast<Pcmd_Shutdown &>(*cmd).wait_mutex;
            std::condition_variable *wait_cond = static_cast<Pcmd_Shutdown &>(*cmd).wait_cond;

            reset_current_playback();
            clock_->stop();
            ins_->initialize();

            std::unique_lock<std::mutex> lock(*wait_mutex);
            wait_cond->notify_one();
            break;
        }
        }
    }
}

void Player::rewind()
{
    fmidi_player_t *pl = pl_.get();
    if (!pl)
        return;

    fmidi_player_rewind(pl);
    ins_->initialize();
}

void Player::reset_current_playback()
{
    pl_.reset();
    smf_.reset();
    ins_->initialize();
}

void Player::resume_play_list()
{
    reset_current_playback();

    Play_List &pll = *play_list_;

    fmidi_smf_u smf;
    while (!smf && !pll.at_end()) {
        const std::string &current = pll.current();
        smf.reset(fmidi_smf_file_read(current.c_str()));
        if (!smf)
            pll.go_next();
        else {
            smf_duration_ = fmidi_smf_compute_duration(smf.get());
            uint16_t unit = fmidi_smf_get_info(smf.get())->delta_unit;
            current_tempo_ = (unit & (1 << 15)) ? 0.0 : 120.0;
        }
    }

    if (smf) {
        fmidi_player_t *pl = fmidi_player_new(smf.get());
        pl_.reset(pl);

        fmidi_player_set_speed(pl, current_speed_ * 0.01);
        fmidi_player_event_callback(pl, [](const fmidi_event_t *ev, void *ud) { static_cast<Player *>(ud)->play_event(*ev); }, this);
        fmidi_player_finish_callback(pl, [](void *ud) { static_cast<Player *>(ud)->file_finished(); }, this);

        smf_ = std::move(smf);
        extract_smf_metadata();
        clock_->start(1);
    }
}

void Player::tick(uint64_t elapsed)
{
    fmidi_player_t *pl = pl_.get();
    if (!pl)
        return;

    double delta = elapsed * 1e-9;
    fmidi_player_tick(pl, delta);
}

void Player::play_event(const fmidi_event_t &event)
{
    switch (event.type) {
    case fmidi_event_message: {
        ins_->send_message(event.data, event.datalen);
        break;
    }
    case fmidi_event_meta: {
        uint8_t type = event.data[0];
        if (type == 0x51 && event.datalen - 1 == 3) {
            uint16_t unit = fmidi_smf_get_info(smf_.get())->delta_unit;
            if (unit & (1 << 15))
                current_tempo_ = 0; // not tempo-based file
            else {
                unsigned midi_tempo = (event.data[1] << 16) | (event.data[2] << 8) | event.data[3];
                current_tempo_ = 60e6 / midi_tempo;
            }
        }
        break;
    }
    default:
        break;
    }
}

void Player::file_finished()
{
    Play_List &pll = *play_list_;
    Repeat_Mode rm = repeat_mode_;
    bool must_stop = false;

    if ((rm & (Repeat_Multi|Repeat_Single)) == Repeat_Single) {
        must_stop = (rm & (Repeat_On|Repeat_Off)) != Repeat_On;
        if (!must_stop)
            rewind();
    }
    else if (!pll.go_next())
        must_stop = true;
    else if (!pll.at_end())
        resume_play_list();
    else {
        must_stop = (rm & (Repeat_On|Repeat_Off)) != Repeat_On;
        if (!must_stop) {
            pll.start();
            resume_play_list();
        }
    }

    if (must_stop) {
        reset_current_playback();
        clock_->stop();
    }
}

void Player::extract_smf_metadata()
{
    const fmidi_smf_t &smf = *smf_;
    const fmidi_event_t *ev;
    fmidi_track_iter_t it;

    Player_Song_Metadata &md = smf_md_;
    md = Player_Song_Metadata();

    //
    const fmidi_smf_info_t *info = fmidi_smf_get_info(&smf);
    sprintf(md.format, "SMF type %u", info->format);
    md.track_count = info->track_count;

    //
    Encoding_Detector ed;
    ed.start();

    //
    fmidi_smf_track_begin(&it, 0);
    while ((ev = fmidi_smf_track_next(&smf, &it)) && ev->type == fmidi_event_meta) {
        uint8_t type = ev->data[0];
        if (type >= 0x01 && type <= 0x05 && ev->datalen - 1 > 0) {
            gsl::cstring_span text(reinterpret_cast<const char *>(ev->data + 1), ev->datalen - 1);
            ed.feed(text);
        }
    }

    //
    ed.finish();
    const char *enc = ed.detected_encoding();
    //fprintf(stderr, "Detected encoding: %s\n", enc);

    //
    fmidi_smf_track_begin(&it, 0);
    while ((ev = fmidi_smf_track_next(&smf, &it)) && ev->type == fmidi_event_meta) {
        uint8_t type = ev->data[0];
        gsl::cstring_span text(reinterpret_cast<const char *>(ev->data + 1), ev->datalen - 1);
        std::string *dst = nullptr;

        switch (type) {
        case 0x01: // Text
            md.text.emplace_back();
            dst = &md.text.back();
            break;
        case 0x02: // Copyright
            if (md.author.empty())
                dst = &md.author;
            break;
        case 0x03: // Track name
            if (md.name.empty())
                dst = &md.name;
            break;
        }

        if (dst) {
            if (enc)
                to_utf8(text, *dst, enc, true);
            else
                dst->assign(text.data(), text.size());
        }
    }
}

Player_State Player::make_state() const
{
    Player_State ps;
    ps.kb = ins_->keyboard_state();
    ps.repeat_mode = repeat_mode_;

    fmidi_player_t *pl = pl_.get();
    if (pl) {
        ps.time_position = fmidi_player_current_time(pl);
        ps.duration = smf_duration_;
        ps.tempo = current_tempo_;
        ps.speed = current_speed_;
        ps.song_metadata = smf_md_;
    }

    Play_List &pll = *play_list_;
    if (!pll.at_end())
        ps.file_path = pll.current();

    return ps;
}

void Player::switch_instrument(Midi_Instrument &ins)
{
    Midi_Instrument &old = *ins_;
    if (&ins != &old) {
        old.initialize();
        old.close_midi_output();
        ins_ = &ins;
    }
}
