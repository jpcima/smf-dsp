#pragma once
#include "playlist.h"
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

enum {
    PC_Play,
    PC_Next,
    PC_Pause,
    PC_Rewind,
    PC_Seek_Cur,
    PC_Seek_End,
    PC_Speed,
    PC_Repeat_Mode,
    PC_Request_State,
    PC_Shutdown,
};

struct Player_Command {
    virtual ~Player_Command() {}
    virtual int type() const noexcept = 0;
};

struct Pcmd_Play : Player_Command {
    int type() const noexcept override { return PC_Play; }
    std::unique_ptr<Play_List> play_list;
    size_t play_index = 0;
};

struct Pcmd_Next : Player_Command {
    int type() const noexcept override { return PC_Next; }
    int play_offset = 0;
};

struct Pcmd_Pause : Player_Command {
    int type() const noexcept override { return PC_Pause; }
};

struct Pcmd_Rewind : Player_Command {
    int type() const noexcept override { return PC_Rewind; }
};

struct Pcmd_Seek_Cur : Player_Command {
    int type() const noexcept override { return PC_Seek_Cur; }
    double time_offset = 0;
};

struct Pcmd_Seek_End : Player_Command {
    int type() const noexcept override { return PC_Seek_End; }
};

struct Pcmd_Speed : Player_Command {
    int type() const noexcept override { return PC_Speed; }
    int increment = 0;
};

struct Pcmd_Repeat_Mode : Player_Command {
    int type() const noexcept override { return PC_Repeat_Mode; }
};

struct Pcmd_Request_State : Player_Command {
    int type() const noexcept override { return PC_Request_State; }
};

struct Pcmd_Shutdown : Player_Command {
    int type() const noexcept override { return PC_Shutdown; }
    std::mutex *wait_mutex = nullptr;
    std::condition_variable *wait_cond = nullptr;
};
