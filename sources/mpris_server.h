//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(HAVE_MPRIS)
#include <memory>

class Application;
struct Player_State;

class Mpris_Server
{
public:
    explicit Mpris_Server(Application &app);
    ~Mpris_Server();
    void exec();
    void receive_new_player_state(const Player_State &ps);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // defined(HAVE_MPRIS)
