//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player.h"
#include "playlist.h"
#include "seeker.h"
#include "instrument.h"
#include "command.h"
#include "clock.h"
#include "smftext.h"
#include "configuration.h"
#include "adev/adev.h"
#include "instruments/port.h"
#include "instruments/synth.h"
#include "synth/synth_host.h"
#include "utility/charset.h"
#include "utility/uv++.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <stdexcept>
#include <cstdio>
#include <cstring>

Player::Player()
    : quit_(false),
      play_list_(new Linear_Play_List),
      seek_state_(new Seek_State),
      midiport_ins_(new Midi_Port_Instrument)
{
    // scan and initialize plugins
    Synth_Host::plugins();

    // create audio device
    if (Audio_Device *adev = init_audio_device()) {
        synth_ins_.reset(new Midi_Synth_Instrument);
        adev->set_callback(&audio_callback, this);
        analyzer_10band &an = level_analyzer_;
        an.init(adev->sample_rate());
        an.setup(1.0, 10e3, 100e-3);
        adev->start();
    }

    // initialize seeker
    seek_state_->set_message_callback(+[](const uint8_t *msg, uint32_t len, void *ptr) {
        reinterpret_cast<Player *>(ptr)->seeker_play_message(msg, len);
    }, this);

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
                    start_ticking();
                else {
                    for (Midi_Instrument *ins : instruments())
                        ins->all_sound_off();
                    stop_ticking();
                }
            }
            break;
        case PC_Rewind:
            rewind();
            break;
        case PC_Seek_End:
            goto_time(smf_duration_);
            break;
        case PC_Seek_Cur: {
            double o = static_cast<Pcmd_Seek_Cur &>(*cmd).time_offset;
            goto_relative_time(o);
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
            const std::string &id = static_cast<Pcmd_Set_Midi_Output &>(*cmd).midi_output_id;
            Log::i("Change MIDI output: %s", id.c_str());
            bool active = stop_ticking();
            ins.open_midi_output(id);
            if (active) start_ticking();
            break;
        }
        case PC_Set_Synth: {
            Midi_Synth_Instrument *ins = synth_ins_.get();
            if (!ins)
                break;

            const std::string &id = static_cast<Pcmd_Set_Synth &>(*cmd).synth_plugin_id;
            Log::i("Change synthesizer: %s", id.c_str());

            bool active = stop_ticking();
            Audio_Device *adev = adev_.get();

            const double audio_rate = adev->sample_rate();
            const double audio_latency = adev->latency();
            ins->configure_audio(audio_rate, audio_latency);
            Log::i("Audio rate: %f Hz", audio_rate);
            Log::i("Audio latency: %f ms", 1e3 * audio_latency);

            ins->open_midi_output(id);

            if (active) start_ticking();
            break;
        }
        case PC_Shutdown: {
            std::mutex *wait_mutex = static_cast<Pcmd_Shutdown &>(*cmd).wait_mutex;
            std::condition_variable *wait_cond = static_cast<Pcmd_Shutdown &>(*cmd).wait_cond;

            reset_current_playback();
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
    fmidi_player_t *pl = pl_.get();
    if (!pl)
        return;

    fmidi_player_rewind(pl);

    for (Midi_Instrument *ins : instruments()) {
        ins->initialize();
        ins->flush_events();
    }
}

void Player::goto_time(double t)
{
    fmidi_player_t *pl = pl_.get();
    if (!pl)
        return;

    begin_seeking();
    fmidi_player_goto_time(pl, t);
    end_seeking();
}

void Player::goto_relative_time(double o)
{
    fmidi_player_t *pl = pl_.get();
    if (!pl)
        return;

    double t = fmidi_player_current_time(pl) + o;
    t = std::max(t, 0.0);
    t = std::min(t, smf_duration_);

    begin_seeking();
    fmidi_player_goto_time(pl, t);
    end_seeking();
}

void Player::reset_current_playback()
{
    pl_.reset();
    smf_.reset();

    for (Midi_Instrument *ins : instruments()) {
        ins->initialize();
        ins->flush_events();
    }
}

void Player::begin_seeking()
{
    // trust the sequencer to send initialization events, don't do it ourselves
    for (Midi_Instrument *ins : instruments())
        ins->flush_events();

    Seek_State &sks = *seek_state_;
    sks.clear();

    seeking_ = true;
}

void Player::end_seeking()
{
    Seek_State &sks = *seek_state_;
    sks.flush_state();

    seeking_ = false;
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
        fmidi_player_t *pl = fmidi_player_new(smf.get());
        pl_.reset(pl);

        fmidi_player_set_speed(pl, current_speed_ * 0.01);
        fmidi_player_event_callback(pl, [](const fmidi_event_t *ev, void *ud) { static_cast<Player *>(ud)->on_sequence_event(*ev); }, this);
        fmidi_player_finish_callback(pl, [](void *ud) { static_cast<Player *>(ud)->file_finished(); }, this);

        smf_ = std::move(smf);
        extract_smf_metadata();
        start_ticking();
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

void Player::on_sequence_event(const fmidi_event_t &event)
{
    switch (event.type) {
    case fmidi_event_message:
        if (!seeking_)
            play_message(event.data, event.datalen);
        else {
            Seek_State &sks = *seek_state_;
            sks.add_event(event.data, event.datalen);
        }
        break;
    case fmidi_event_meta: {
        play_meta(event.data[0], event.data + 1, event.datalen - 1);
        break;
    }
    default:
        break;
    }
}

void Player::play_message(const uint8_t *msg, uint32_t len)
{
    uint64_t now = uv_hrtime();
    double ts = 0;
    int flags = 0;
    if (ts_started_)
        ts = 1e-9 * (now - ts_last_);
    else {
        flags |= Midi_Message_Is_First;
        ts_started_ = true;
    }
    for (Midi_Instrument *ins : instruments())
        ins->send_message(msg, len, ts, flags);
    ts_last_ = now;
}

void Player::play_meta(uint8_t type, const uint8_t *msg, uint32_t len)
{
    if (type == 0x51 && len == 3) {
        uint16_t unit = fmidi_smf_get_info(smf_.get())->delta_unit;
        if (unit & (1 << 15))
            current_tempo_ = 0; // not tempo-based file
        else {
            unsigned midi_tempo = (msg[0] << 16) | (msg[1] << 8) | msg[2];
            current_tempo_ = 60e6 / midi_tempo;
        }
    }
}

///
static bool is_midi_reset_message(const uint8_t *msg, uint32_t len)
{
    if (len >= 1 && msg[0] == 0xff) // GM system reset
        return true;

    if (len >= 4 && msg[0] == 0xf0 && msg[len - 1] == 0xf7) { // sysex resets
        uint8_t manufacturer = msg[1];
        uint8_t device_id = msg[2];
        const uint8_t *payload = msg + 3;
        uint8_t paysize = len - 4;

        switch (manufacturer) {
        case 0x7e: { // GM system messages
            const uint8_t gm_on[] = {0x09, 0x01};
            if (paysize >= 2 && !memcmp(gm_on, payload, 2))
                return true;
            break;
        }
        case 0x43: { // Yamaha XG
            const uint8_t xg_on[] = {0x4c, 0x00, 0x00, 0x7e};
            const uint8_t all_reset[] = {0x4c, 0x00, 0x00, 0x7f};
            if ((device_id & 0xf0) == 0x10 && paysize >= 4 &&
                (!memcmp(xg_on, payload, 4) || !memcmp(all_reset, payload, 4)))
                return true;
            break;
        }
        case 0x41: { // Roland GS / Roland MT-32
            const uint8_t gs_on[] = {0x42, 0x12, 0x40, 0x00, 0x7f};
            const uint8_t mt32_reset[] = {0x16, 0x12, 0x7f};
            if ((device_id & 0xf0) == 0x10 &&
                ((paysize >= 5 && !memcmp(gs_on, payload, 5)) ||
                 (paysize >= 3 && !memcmp(mt32_reset, payload, 3))))
                return true;
            break;
        }
        }
    }

    return false;
}

///
void Player::seeker_play_message(const uint8_t *msg, uint32_t len)
{
    play_message(msg, len);

    // add a delay if the message may reset the device
    if (is_midi_reset_message(msg, len))
        uv_sleep(50);
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
    ps.kb = instruments().front()->keyboard_state();
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

    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(current_levels_mutex_));
        std::memcpy(ps.audio_levels, current_levels_, 10 * sizeof(float));
    }

    return ps;
}

std::vector<Midi_Instrument *> Player::instruments() const
{
    std::vector<Midi_Instrument *> ins;
    ins.push_back(midiport_ins_.get());
    if (synth_ins_)
        ins.push_back(synth_ins_.get());
    return ins;
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

    if (!clock.active())
        return false;

    for (Midi_Instrument *ins : instruments())
        ins->flush_events();

    clock.stop();
    return true;
}

Audio_Device *Player::init_audio_device()
{
    Audio_Device *adev = adev_.get();

    adev = Audio_Device::create_best_for_system();
    if (!adev) {
        Log::e("Cannot create an audio device for this system");
        return nullptr;
    }
    adev_.reset(adev);

    std::unique_ptr<CSimpleIniA> ini = load_global_configuration();
    if (!ini)
        ini = create_configuration();

    double desired_sample_rate = ini->GetDoubleValue("", "synth-sample-rate", 44100);
    desired_sample_rate = std::max(22050.0, std::min(192000.0, desired_sample_rate));

    double desired_latency = ini->GetDoubleValue("", "synth-audio-latency", 50);
    desired_latency = 1e-3 * std::max(1.0, std::min(500.0, desired_latency));

    if (!adev->init(desired_sample_rate, desired_latency)) {
        Log::e("Cannot initialize the audio device");
        adev_.reset();
        return nullptr;
    }

    return adev;
}

void Player::audio_callback(float *output, unsigned nframes, void *user_data)
{
    Player *self = reinterpret_cast<Player *>(user_data);
    self->synth_ins_->generate_audio(output, nframes);

    const float *levels = self->level_analyzer_.compute(output, nframes);
    std::unique_lock<std::mutex> levels_lock(self->current_levels_mutex_, std::try_to_lock);
    if (levels_lock.owns_lock())
        std::memcpy(self->current_levels_, levels, 10 * sizeof(float));
}
