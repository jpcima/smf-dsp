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
class Midi_Instrument;
class Midi_Port_Instrument;
class Midi_Synth_Instrument;
enum Repeat_Mode : unsigned;
class Synth_Host;
class Basic_Sequencer;
class Sequencer_Message_Event;
class Sequencer_Meta_Event;
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
    void reset_current_playback();

    void resume_play_list();

    void tick(uint64_t elapsed);
    void play_message(const Sequencer_Message_Event &event);
    void play_meta(const Sequencer_Meta_Event &event);
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
    std::unique_ptr<Basic_Sequencer> seq_;
    double smf_duration_ = 0;
    Player_Song_Metadata smf_md_;
    double current_tempo_ = 0;
    unsigned current_speed_ = 100;

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
