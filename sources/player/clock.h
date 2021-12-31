//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <functional>
#include <memory>
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_timer_s uv_timer_t;

class Player_Clock {
public:
    explicit Player_Clock(uv_loop_t *loop);
    ~Player_Clock();

    void start(uint64_t ms_interval);
    void stop();
    bool active() const noexcept { return active_; }

    std::function<void (uint64_t)> TimerCallback;

private:
    static void callback(uv_timer_t *t);

private:
    std::unique_ptr<uv_timer_t> timer_;
    uint64_t last_tick_ = ~(uint64_t)0;
    bool active_ = false;
};
