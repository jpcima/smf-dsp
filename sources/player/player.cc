//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player.h"
#include "playlist.h"
#include "instrument.h"
#include "command.h"
#include "clock.h"
#include "smftext.h"
#include "synth/synth_host.h"
#include "utility/charset.h"
#include <uv.h>
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
            if (seq_) {
                Player_Clock &c = *clock_;
                Midi_Instrument &ins = *ins_;
                if (!c.active())
                    start_ticking();
                else {
                    ins.all_sound_off();
                    stop_ticking();
                }
            }
            break;
        case PC_Rewind:
            rewind();
            break;
        case PC_Seek_End: {
            Basic_Sequencer *seq = seq_.get();
            if (seq)
                seq->goto_time(smf_duration_);
            break;
        }
        case PC_Seek_Cur: {
            Basic_Sequencer *seq = seq_.get();
            if (seq) {
                double o = static_cast<Pcmd_Seek_Cur &>(*cmd).time_offset;
                double t = seq->get_current_time() + o;
                t = std::max(t, 0.0);
                t = std::min(t, smf_duration_);
                seq->goto_time(t);
            }
            break;
        }
        case PC_Speed: {
            Basic_Sequencer *seq = seq_.get();
            if (seq) {
                unsigned cur = (unsigned)(0.5 + seq->get_current_speed() * 100);
                int speed = static_cast<int>(cur) + static_cast<Pcmd_Speed &>(*cmd).increment;
                speed = std::max(speed, 1);
                speed = std::min(speed, 500);
                seq->set_speed(speed * 0.01);
                current_speed_ = speed;
            }
            break;
        }
        case PC_Repeat_Mode: {
            Repeat_Mode rm = Repeat_Mode((repeat_mode_ + 1) % (Repeat_Mode_Max + 1));
            repeat_mode_ = rm;
            update_sequencer_loop_mode();
            break;
        }
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
            bool active = stop_ticking();
            switch_instrument(ins);
            ins.open_midi_output(static_cast<Pcmd_Set_Midi_Output &>(*cmd).midi_output_id);
            if (active) start_ticking();
            break;
        }
        case PC_Set_Synth: {
            Midi_Synth_Instrument &ins = *synth_ins_;
            bool active = stop_ticking();
            switch_instrument(ins);
            ins.open_midi_output(static_cast<Pcmd_Set_Synth &>(*cmd).synth_plugin_id);
            if (active) start_ticking();
            break;
        }
        case PC_Shutdown: {
            Midi_Instrument &ins = *ins_;
            std::mutex *wait_mutex = static_cast<Pcmd_Shutdown &>(*cmd).wait_mutex;
            std::condition_variable *wait_cond = static_cast<Pcmd_Shutdown &>(*cmd).wait_cond;

            reset_current_playback();
            ins.initialize();
            stop_ticking();

            std::unique_lock<std::mutex> lock(*wait_mutex);
            wait_cond->notify_one();
            break;
        }
        }
    }
}

void Player::rewind()
{
    Basic_Sequencer *seq = seq_.get();
    if (!seq)
        return;

    seq->rewind();

    Midi_Instrument &ins = *ins_;
    ins.initialize();
    ins.flush_events();
}

void Player::reset_current_playback()
{
    seq_.reset();
    smf_.reset();

    Midi_Instrument &ins = *ins_;
    ins.initialize();
    ins.flush_events();
}

void Player::resume_play_list()
{
    reset_current_playback();

    Play_List &pll = *play_list_;

    fmidi_smf_u smf;
    while (!smf && !pll.at_end()) {
        const std::string &current = pll.current();

        if (FILE *fh = fopen_utf8(current.c_str(), "rb")) {
            auto fh_cleanup = gsl::finally([fh] { fclose(fh); });
            smf.reset(fmidi_smf_stream_read(fh));
        }

        if (!smf)
            pll.go_next();
        else {
            smf_duration_ = fmidi_smf_compute_duration(smf.get());
            uint16_t unit = fmidi_smf_get_info(smf.get())->delta_unit;
            current_tempo_ = (unit & (1 << 15)) ? 0.0 : 120.0;
        }
    }

    if (smf) {
        Basic_Sequencer *seq = Basic_Sequencer::create_sequencer(seq_type_, *smf);
        seq_.reset(seq);

        update_sequencer_loop_mode();

        seq->set_speed(current_speed_ * 0.01);

        seq->set_message_callback([](const Sequencer_Message_Event &ev, void *ud) { static_cast<Player *>(ud)->play_message(ev); }, this);
        seq->set_meta_callback([](const Sequencer_Meta_Event &ev, void *ud) { static_cast<Player *>(ud)->play_meta(ev); }, this);

        seq->set_finished_callback([](void *ud) { static_cast<Player *>(ud)->file_finished(); }, this);

        smf_ = std::move(smf);
        extract_smf_metadata();
        start_ticking();
    }
}

void Player::update_sequencer_loop_mode()
{
    Basic_Sequencer *seq = seq_.get();
    if (!seq)
        return;

    Repeat_Mode rm = repeat_mode_;

    // enable MIDI file loops when on single track repeat, otherwise disable
    seq->set_loop_enabled(
        ((rm & (Repeat_On|Repeat_Off)) == Repeat_On) &&
        ((rm & (Repeat_Multi|Repeat_Single)) == Repeat_Single));
}

void Player::tick(uint64_t elapsed)
{
    Basic_Sequencer *seq = seq_.get();
    if (!seq)
        return;

    double delta = elapsed * 1e-9;
    seq->tick(delta);
}

void Player::play_message(const Sequencer_Message_Event &event)
{
    Midi_Instrument &ins = *ins_;
    uint64_t now = uv_hrtime();
    double ts = 0;
    int flags = 0;
    if (ts_started_)
        ts = 1e-9 * (now - ts_last_);
    else {
        flags |= Midi_Message_Is_First;
        ts_started_ = true;
    }
    ins.send_message(event.data, event.length, ts, flags);
    ts_last_ = now;
}

void Player::play_meta(const Sequencer_Meta_Event &event)
{
    uint8_t type = event.type;
    if (type == 0x51 && event.length == 3) {
        uint16_t unit = fmidi_smf_get_info(smf_.get())->delta_unit;
        if (unit & (1 << 15))
            current_tempo_ = 0; // not tempo-based file
        else {
            unsigned midi_tempo = (event.data[0] << 16) | (event.data[1] << 8) | event.data[2];
            current_tempo_ = 60e6 / midi_tempo;
        }
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
        stop_ticking();
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
    SMF_Encoding_Detector det;
    det.scan(smf);

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

        if (dst)
            *dst = det.decode_to_utf8(text);
    }
}

Player_State Player::make_state() const
{
    Player_State ps;
    Midi_Instrument &ins = *ins_;
    ps.kb = ins.keyboard_state();
    ps.repeat_mode = repeat_mode_;

    Basic_Sequencer *seq = seq_.get();
    if (seq) {
        ps.time_position = seq->get_current_time();
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
    Midi_Instrument &old_ins = *ins_;

    if (&ins == &old_ins)
        return;

    old_ins.initialize();
    old_ins.flush_events();
    old_ins.close_midi_output();

    ins_ = &ins;
}

bool Player::start_ticking()
{
    Player_Clock &clock = *clock_;

    if (clock.active())
        return false;

    ts_started_ = false;
    clock.start(1);
    return true;
}

bool Player::stop_ticking()
{
    Player_Clock &clock = *clock_;
    Midi_Instrument &ins = *ins_;

    if (!clock.active())
        return false;

    ins.flush_events();
    clock.stop();
    return true;
}
