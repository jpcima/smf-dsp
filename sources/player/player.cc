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
#include "instruments/synth_fx.h"
#include "synth/synth_host.h"
#include "player/smfutil.h"
#include "data/ins_names.h"
#include "utility/charset.h"
#include "utility/uv++.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <array>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cassert>

Player::Player()
    : quit_(false),
      play_list_(new Linear_Play_List),
      seek_state_(new Seek_State),
      midiport_ins_(new Midi_Port_Instrument)
{
    // scan and initialize plugins
    Synth_Host::plugins();

    // create audio device
    Synth_Fx *fx = new Synth_Fx;
    fx_.reset(fx);
    if (Audio_Device *adev = init_audio_device()) {
        float sample_rate = adev->sample_rate();
        synth_ins_.reset(new Midi_Synth_Instrument);
        adev->set_callback(&audio_callback, this);
        analyzer_10band &an = level_analyzer_;
        an.init(sample_rate);
        an.setup(1.0, 16e3, 100e-3);
        fx->init(sample_rate);
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

        if (quit_.load()) {
            reset_current_playback();
            return;
        }

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
        case PC_Channel_Enable: {
            unsigned channel = static_cast<Pcmd_Channel_Enable &>(*cmd).channel;
            bool enable = static_cast<Pcmd_Channel_Enable &>(*cmd).enable;
            set_channel_enabled(channel, enable);
            break;
        }
        case PC_Channel_Toggle: {
            unsigned channel = static_cast<Pcmd_Channel_Toggle &>(*cmd).channel;
            toggle_channel_enabled(channel);
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
            const std::string &id = static_cast<Pcmd_Set_Midi_Output &>(*cmd).midi_output_id;

            // reinitialize the old device
            ins.initialize();

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

            fx_enable_request_.store(id.empty() ? 0 : 1);

            ins->open_midi_output(id);

            if (active) start_ticking();
            break;
        }
        case PC_Set_Fx_Parameter: {
            const size_t index = static_cast<Pcmd_Set_Fx_Parameter &>(*cmd).index;
            const int value = static_cast<Pcmd_Set_Fx_Parameter &>(*cmd).value;
            fx_->set_parameter(index, value);
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

void Player::set_channel_enabled(unsigned ch, bool en)
{
    assert(!seeking_);

    if (ch >= 16)
        return;

    if (channel_enabled_[ch] == en)
        return;

    if (!en) {
        uint8_t all_sound_off[3];
        all_sound_off[0] = 0xb0 | ch;
        all_sound_off[1] = 120;
        all_sound_off[2] = 0;
        play_message(all_sound_off, sizeof(all_sound_off));
    }

    channel_enabled_[ch] = en;
}

void Player::toggle_channel_enabled(unsigned ch)
{
    if (ch >= 16)
        return;

    set_channel_enabled(ch, !channel_enabled_[ch]);
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
            smf.reset(fmidi_auto_stream_read(fh));
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
        if (Midi_Synth_Instrument *synth_ins = synth_ins_.get()) {
            synth_ins->flush_events();
            synth_ins->preload(*smf);
        }

        fmidi_player_t *pl = fmidi_player_new(smf.get());
        pl_.reset(pl);

        fmidi_player_set_speed(pl, current_speed_ * 0.01);
        fmidi_player_event_callback(pl, [](const fmidi_event_t *ev, void *ud) { static_cast<Player *>(ud)->on_sequence_event(*ev); }, this);
        fmidi_player_finish_callback(pl, [](void *ud) { static_cast<Player *>(ud)->file_finished(); }, this);

        smf_ = std::move(smf);
        extract_smf_metadata();
        send_reset_if_smf_needs();

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
    case fmidi_event_message: {
        uint8_t status = event.data[0];
        bool is_note = (status & 0xf0) == 0x80 || (status & 0xf0) == 0x90;
        if (is_note && !channel_enabled_[status & 0x0f])
            break;

        if (!seeking_)
            play_message(event.data, event.datalen);
        else {
            Seek_State &sks = *seek_state_;
            sks.add_event(event.data, event.datalen);
        }
        break;
    }
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
void Player::seeker_play_message(const uint8_t *msg, uint32_t len)
{
    play_message(msg, len);

    // add a delay if the message may reset the device
    if (identify_reset_message(msg, len))
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

void Player::send_reset_if_smf_needs()
{
    const fmidi_smf_t &smf = *smf_;
    fmidi_seq_u seq{fmidi_seq_new(&smf)};

    bool have_reset = false;

    // search for a reset in the starting sysex sequence
    fmidi_seq_event_t evt;
    while (!have_reset && fmidi_seq_next_event(seq.get(), &evt)) {
        if (evt.event->type != fmidi_event_message)
            continue;

        const uint8_t *msg = evt.event->data;
        uint32_t len = evt.event->datalen;

        bool is_sysex = len >= 2 && msg[0] == 0xf0 && msg[len - 1] == 0xf7;
        if (!is_sysex)
            break;

        have_reset = identify_reset_message(msg, len);
    }

    if (have_reset)
        return;

    // the prologue did not contain a reset sequence
    // search for a matching MIDI spec according to the programs used

    std::vector<synth_midi_ins> instruments = collect_file_instruments(smf);

    struct Contestant {
        uint32_t spec;
        uint32_t flags;
        uint32_t score;
    };

    Contestant gm1{KMS_GeneralMidi, Midi_Spec_GM1, 0};
    Contestant gm2{KMS_GeneralMidi2, Midi_Spec_GM2|Midi_Spec_GM1, 0};
    Contestant xg{KMS_YamahaXG, Midi_Spec_XG|Midi_Spec_GM1, 0};
    Contestant gs{KMS_RolandGS, Midi_Spec_GS|Midi_Spec_SC|Midi_Spec_GM1, 0};

    std::array<Contestant *, 4> contestants {{&gm1, &gm2, &xg, &gs}};

    for (synth_midi_ins ins : instruments) {
        Midi_Program_Id id {ins.percussive != 0, ins.bank_msb, ins.bank_lsb, ins.program};

        for (Contestant *con : contestants) {
            if (Midi_Data::get_program(id, con->flags))
                con->score += 2;
            else if (Midi_Data::get_fallback_program(id, con->flags))
                con->score += 1;
        }
    }

    //Log::d("Score GM1=%u GM2=%u XG=%u GS=%u", gm1.score, gm2.score, xg.score, gs.score);

    uint32_t winnerIndex = 0;
    for (uint32_t i = 1; i < contestants.size(); ++i) {
        if (contestants[i]->score > contestants[winnerIndex]->score)
            winnerIndex = i;
    }

    Contestant &winner = *contestants[winnerIndex];

    // send a reset which matches the deduced specification

    const uint8_t *msg = nullptr;
    uint32_t len = 0;

    switch (winner.spec) {
    case KMS_GeneralMidi: {
        static constexpr uint8_t sys_gm_reset[] =
            {0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
        msg = sys_gm_reset;
        len = sizeof(sys_gm_reset);
        Log::i("Sending the GM reset");
        break;
    }
    case KMS_GeneralMidi2: {
        static constexpr uint8_t sys_gm2_reset[] =
            {0xf0, 0x7e, 0x7f, 0x09, 0x03, 0xf7};
        msg = sys_gm2_reset;
        len = sizeof(sys_gm2_reset);
        Log::i("Sending the GM2 reset");
        break;
    }
    case KMS_YamahaXG: {
        static constexpr uint8_t sys_xg_reset[] =
            {0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7};
        msg = sys_xg_reset;
        len = sizeof(sys_xg_reset);
        Log::i("Sending the XG reset");
        break;
    }
    case KMS_RolandGS: {
        static constexpr uint8_t sys_gs_reset[] =
            {0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};
        msg = sys_gs_reset;
        len = sizeof(sys_gs_reset);
        Log::i("Sending the GS reset");
        break;
    }
    }

    if (len > 0) {
        play_message(msg, len);
        uv_sleep(50);
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

    for (unsigned ch = 0; ch < 16; ++ch)
        ps.channel_enabled[ch] = channel_enabled_[ch];

    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(current_levels_mutex_));
        std::memcpy(ps.audio_levels, current_levels_, 10 * sizeof(float));
    }

    Synth_Fx &fx = *fx_;
    for (size_t p = 0; p < Synth_Fx::Parameter_Count; ++p)
        ps.fx_parameters[p] = fx.get_parameter(p);

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

    Log::s("Initialized audio device: %s", adev->audio_system_name());

    return adev;
}

void Player::audio_callback(float *output, unsigned nframes, void *user_data)
{
    Player *self = reinterpret_cast<Player *>(user_data);
    self->synth_ins_->generate_audio(output, nframes);

    ///
    Synth_Fx &fx = *self->fx_;
    bool fx_enabled;
    switch (self->fx_enable_request_.exchange(-1)) {
    case 0: fx_enabled = false; break;
    case 1: fx_enabled = true; break;
    default: fx_enabled = self->fx_enabled_; break;
    }
    if (self->fx_enabled_ != fx_enabled) {
        if (fx_enabled)
            fx.clear();
        self->fx_enabled_ = fx_enabled;
    }
    if (fx_enabled)
        fx.compute(output, nframes);

    ///
    const float *levels = self->level_analyzer_.compute_stereo(output, nframes);
    std::unique_lock<std::mutex> levels_lock(self->current_levels_mutex_, std::try_to_lock);
    if (levels_lock.owns_lock())
        std::memcpy(self->current_levels_, levels, 10 * sizeof(float));
}
