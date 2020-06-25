//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "keystate.h"
#include <gsl.hpp>
#include <string>
#include <cstdint>

struct Midi_Output {
    std::string id;
    std::string name;
};

enum Midi_Message_Flag {
    Midi_Message_Is_First = 1,
};

///
class Midi_Instrument {
public:
    Midi_Instrument();
    virtual ~Midi_Instrument() {}
    void send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags);
    void initialize();
    void all_sound_off();

    const Keyboard_State &keyboard_state() const noexcept { return kbs_; }

    virtual void flush_events() {}

    virtual void open_midi_output(gsl::cstring_span id) = 0;
    virtual void close_midi_output() = 0;

    virtual bool is_synth() const { return false; }

protected:
    virtual void handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags) = 0;

private:
    Keyboard_State kbs_;
};
