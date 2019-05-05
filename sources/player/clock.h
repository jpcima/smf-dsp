#pragma once
#include <uv.h>
#include <functional>

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
    uv_timer_t timer_;
    uint64_t last_tick_ = ~(uint64_t)0;
    bool active_ = false;
};
