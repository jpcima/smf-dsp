//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "clock.h"
#include "utility/uv++.h"
#include <stdexcept>
#include <cstdio>

Player_Clock::Player_Clock(uv_loop_t *loop)
    : timer_(new uv_timer_t)
{
    if (uv_timer_init(loop, timer_.get()) != 0)
        throw std::runtime_error("uv_timer_init");
    timer_->data = this;
}

Player_Clock::~Player_Clock()
{
    stop();
}

void Player_Clock::start(uint64_t ms_interval)
{
    last_tick_ = ~(uint64_t)0;
 #if UV_VERSION_MAJOR >= 1
    uv_timer_start(timer_.get(), &callback, 0, ms_interval);
#else
    uv_timer_start(timer_.get(), +[](uv_timer_t *t, int) { callback(t); }, 0, ms_interval);
#endif
    active_ = true;
}

void Player_Clock::stop()
{
    uv_timer_stop(timer_.get());
    active_ = false;
}

void Player_Clock::callback(uv_timer_t *t)
{
    Player_Clock *self = static_cast<Player_Clock *>(t->data);
    uint64_t now = uv_hrtime();
    uint64_t then = self->last_tick_;
    self->last_tick_ = now;
    uint64_t elapsed = 0;
    if (then != ~(uint64_t)0)
        elapsed = now - then;
    if (self->TimerCallback)
        self->TimerCallback(elapsed);
}
