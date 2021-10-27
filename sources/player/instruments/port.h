//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "player/instrument.h"
#include <vector>
#include <memory>
class RtMidiOut;

class Midi_Port_Instrument : public Midi_Instrument {
public:
    Midi_Port_Instrument();
    ~Midi_Port_Instrument();

    static std::vector<Midi_Output> get_midi_outputs();
    void open_midi_output(nonstd::string_view id) override;
    void close_midi_output() override;

protected:
    void handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags) override;

private:
    void handle_midi_error(int type, const std::string &text);

private:
    std::unique_ptr<RtMidiOut> out_;
    int midi_error_status_ = 0;
};
