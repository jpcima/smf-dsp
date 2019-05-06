#pragma once
#include "keystate.h"
#include <gsl.hpp>
#include <vector>
#include <memory>
#include <cstdint>
class RtMidiOut;

class Midi_Instrument {
public:
    virtual ~Midi_Instrument() {}
    void send_message(const uint8_t *data, unsigned len);
    void initialize();
    void all_sound_off();

    const Keyboard_State &keyboard_state() const noexcept { return kbs_; }

    virtual std::vector<std::string> get_midi_outputs() = 0;
    virtual bool has_virtual_midi_output() = 0;
    virtual void set_midi_output(gsl::cstring_span name) = 0;
    virtual void set_midi_virtual_output() = 0;

protected:
    virtual void handle_send_message(const uint8_t *data, unsigned len) = 0;

private:
    Keyboard_State kbs_;
};

///
class Dummy_Instrument : public Midi_Instrument {
public:
    std::vector<std::string> get_midi_outputs() override
        { return std::vector<std::string>(); }

    bool has_virtual_midi_output() override { return false; }
    void set_midi_output(gsl::cstring_span name) override { return; }
    void set_midi_virtual_output() override { return; }

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;
};

///
class Midi_Port_Instrument : public Midi_Instrument {
public:
    Midi_Port_Instrument();
    ~Midi_Port_Instrument();

    std::vector<std::string> get_midi_outputs() override;
    bool has_virtual_midi_output() override;
    void set_midi_output(gsl::cstring_span name) override;
    void set_midi_virtual_output() override;

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;

private:
    std::unique_ptr<RtMidiOut> out_;
};
