#pragma once
#include "state.h"
#include <fmidi/fmidi.h>
#include <uv.h>
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
enum Repeat_Mode : unsigned;

class Player {
public:
    Player();
    ~Player();

    void push_command(std::unique_ptr<Player_Command> cmd);

    std::function<void (const Player_State &)> StateCallback;

private:
    void thread_exec(std::condition_variable &ready_cv, std::mutex &ready_mutex);
    void process_command_queue();

    void rewind();
    void reset_current_playback();

    void resume_play_list();

    void tick(uint64_t elapsed);
    void play_event(const fmidi_event_t &event);
    void file_finished();

    void extract_smf_metadata();

    Player_State make_state() const;

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

    // instrument
    std::unique_ptr<Midi_Instrument> ins_;
};
