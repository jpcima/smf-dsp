//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "state.h"
#include <fmidi/fmidi.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>
struct Player_Command;
class Player_Clock;
class Play_List;
class Seek_State;
class Midi_Instrument;
class Midi_Port_Instrument;
class Midi_Synth_Instrument;
enum Repeat_Mode : unsigned;
class Synth_Host;
typedef struct uv_async_s uv_async_t;

class Player {
public:
    Player();
    ~Player();

    void push_command(std::unique_ptr<Player_Command> cmd);

    std::function<void (const Player_State &)> StateCallback;

private:
    void thread_exec();
    void process_command_queue();

    void rewind();
    void goto_time(double t);
    void goto_relative_time(double o);
    void reset_current_playback();

    void begin_seeking();
    void end_seeking();

    void resume_play_list();

    void tick(uint64_t elapsed);
    void play_event(const fmidi_event_t &event);
    void seeker_play_message(const uint8_t *msg, uint32_t len);
    void file_finished();

    void extract_smf_metadata();

    Player_State make_state() const;

    void switch_instrument(Midi_Instrument &ins);

    bool start_ticking();
    bool stop_ticking();

private:
    std::thread thread_;
    std::atomic_bool quit_;
    uv_async_t *async_ = nullptr;
    Player_Clock *clock_ = nullptr;
    std::unique_ptr<Play_List> play_list_;
    Repeat_Mode repeat_mode_ = Repeat_Mode(0);
    mutable std::mutex cmd_mutex_;
    std::queue<std::unique_ptr<Player_Command>> cmd_queue_;

    // current playback
    fmidi_smf_u smf_;
    fmidi_player_u pl_;
    double smf_duration_ = 0;
    Player_Song_Metadata smf_md_;
    double current_tempo_ = 0;
    unsigned current_speed_ = 100;

    // seek state
    bool seeking_ = false;
    std::unique_ptr<Seek_State> seek_state_;

    // instrument
    Midi_Instrument *ins_ = nullptr;
    std::unique_ptr<Midi_Port_Instrument> midiport_ins_;
    std::unique_ptr<Midi_Synth_Instrument> synth_ins_;

    // timestamping
    bool ts_started_ = false;
    uint64_t ts_last_ = 0;

    // startup and shutdown synchronization
    std::condition_variable ready_cv_;
    std::mutex ready_mutex_;
};
