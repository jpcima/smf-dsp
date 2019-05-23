#pragma once
#include "keystate.h"
#include <RtMidi.h>
#include <RtAudio.h>
#include <ring_buffer.h>
#include <gsl.hpp>
#include <vector>
#include <memory>
#include <cstdint>
class Synth_Host;

struct Midi_Output {
    std::string id;
    std::string name;
};

class Midi_Instrument {
public:
    Midi_Instrument();
    virtual ~Midi_Instrument() {}
    void send_message(const uint8_t *data, unsigned len);
    void initialize();
    void all_sound_off();

    const Keyboard_State &keyboard_state() const noexcept { return kbs_; }

    virtual void open_midi_output(gsl::cstring_span id) = 0;
    virtual void close_midi_output() = 0;

protected:
    virtual void handle_send_message(const uint8_t *data, unsigned len) = 0;

private:
    Keyboard_State kbs_;
};

///
class Dummy_Instrument : public Midi_Instrument {
public:
    void open_midi_output(gsl::cstring_span id) override {}
    void close_midi_output() override {}

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;
};

///
class Midi_Port_Instrument : public Midi_Instrument {
public:
    Midi_Port_Instrument();
    ~Midi_Port_Instrument();

    static std::vector<Midi_Output> get_midi_outputs();
    void open_midi_output(gsl::cstring_span id) override;
    void close_midi_output() override;

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;

private:
    std::unique_ptr<RtMidiOut> out_;
};

///
class Midi_Synth_Instrument : public Midi_Instrument {
public:
    Midi_Synth_Instrument();
    ~Midi_Synth_Instrument();

    void open_midi_output(gsl::cstring_span id) override;
    void close_midi_output() override;

protected:
    void handle_send_message(const uint8_t *data, unsigned len) override;

private:
    static int audio_callback(void *output_buffer, void *, unsigned int nframes, double, RtAudioStreamStatus, void *user_data);
    void process_midi();

private:
    std::unique_ptr<Synth_Host> host_;
    std::unique_ptr<Ring_Buffer> midibuf_;
    std::unique_ptr<RtAudio> audio_;

private:
    static constexpr unsigned midi_buffer_size = 8192;
    static constexpr unsigned midi_message_max = 256;
};
