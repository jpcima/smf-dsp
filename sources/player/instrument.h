#pragma once
#include "keystate.h"
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

protected:
    virtual void handle_send_message(const uint8_t *data, unsigned len) = 0;

private:
    Keyboard_State kbs_;
};

class Dummy_Instrument : public Midi_Instrument {
protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;
};

class Midi_Port_Instrument : public Midi_Instrument {
public:
    Midi_Port_Instrument();
    ~Midi_Port_Instrument();

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;

private:
    std::unique_ptr<RtMidiOut> out_;
};
