#pragma once
#include "keystate.h"
#include <gsl.hpp>
#include <vector>
#include <memory>
#include <cstdint>
class RtMidiOut;

struct Midi_Output {
    std::string id;
    std::string name;
};

class Midi_Instrument {
public:
    virtual ~Midi_Instrument() {}
    void send_message(const uint8_t *data, unsigned len);
    void initialize();
    void all_sound_off();

    const Keyboard_State &keyboard_state() const noexcept { return kbs_; }

    virtual std::vector<Midi_Output> get_midi_outputs() = 0;
    virtual void set_midi_output(gsl::cstring_span id) = 0;

protected:
    virtual void handle_send_message(const uint8_t *data, unsigned len) = 0;

private:
    Keyboard_State kbs_;
};

///
class Dummy_Instrument : public Midi_Instrument {
public:
    std::vector<Midi_Output> get_midi_outputs() override
        { return std::vector<Midi_Output>(); }

    void set_midi_output(gsl::cstring_span id) override { return; }

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;
};

///
class Midi_Port_Instrument : public Midi_Instrument {
public:
    Midi_Port_Instrument();
    ~Midi_Port_Instrument();

    std::vector<Midi_Output> get_midi_outputs() override;
    void set_midi_output(gsl::cstring_span id) override;

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;

private:
    std::unique_ptr<RtMidiOut> out_;
};
