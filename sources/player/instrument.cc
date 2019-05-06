#include "instrument.h"
#include <RtMidi.h>
#include <cstdio>

void Midi_Instrument::send_message(const uint8_t *data, unsigned len)
{
    handle_send_message(data, len);
    kbs_.handle_message(data, len);
}

void Midi_Instrument::initialize()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg)); }
        // reset all controllers
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 121, 0 };
            send_message(msg, sizeof(msg)); }
        // bank select
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 0, 0 };
            send_message(msg, sizeof(msg)); }
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 32, 0 };
            send_message(msg, sizeof(msg)); }
        // program change
        { uint8_t msg[] { (uint8_t)((0b1100 << 4) | c), 0 };
            send_message(msg, sizeof(msg)); }
        // pitch bend change
        { uint8_t msg[] { (uint8_t)((0b1110 << 4) | c), 0, 0b1000000 };
            send_message(msg, sizeof(msg)); }
    }
}

void Midi_Instrument::all_sound_off()
{
    for (unsigned c = 0; c < 16; ++c) {
        // all sound off
        { uint8_t msg[] { (uint8_t)((0b1011 << 4) | c), 120, 0 };
            send_message(msg, sizeof(msg)); }
    }
}

//
void Dummy_Instrument::handle_send_message(const uint8_t *data, unsigned len)
{
    if (false) {
        for (unsigned i = 0; i < len; ++i)
            fprintf(stderr, "%s%02X", i ? " " : "", data[i]);
        fputc('\n', stderr);
    }
}

//
Midi_Port_Instrument::Midi_Port_Instrument()
{
    set_midi_output(gsl::cstring_span());
}

Midi_Port_Instrument::~Midi_Port_Instrument()
{
}

void Midi_Port_Instrument::handle_send_message(const uint8_t *data, unsigned len)
{
    RtMidiOut &out = *out_;
    out.sendMessage(data, len);
}

std::vector<std::string> Midi_Port_Instrument::get_midi_outputs()
{
    RtMidiOut &out = *out_;
    std::vector<std::string> ports;

    for (size_t i = 0, n = out.getPortCount(); i < n; ++i)
        ports.push_back(out.getPortName(i));

    return ports;
}

bool Midi_Port_Instrument::has_virtual_midi_output()
{
    RtMidiOut &out = *out_;
    return out.getCurrentApi() != RtMidi::WINDOWS_MM;
}

void Midi_Port_Instrument::set_midi_output(gsl::cstring_span name)
{
    RtMidi::Api api = out_ ? out_->getCurrentApi() : RtMidi::UNSPECIFIED;
    out_.reset();

    RtMidiOut *out = new RtMidiOut(api, "FMidiPlay");
    out_.reset(out);

    bool port_found = false;
    if (!name.empty()) {
        for (size_t i = 0, n = out->getPortCount(); i < n && !port_found; ++i) {
            if (out->getPortName(i) == name) {
                out->openPort(i, "FMidiPlay out");
                port_found = true;
            }
        }
    }

    if (!port_found) {
        if (has_virtual_midi_output())
            out->openVirtualPort("FMidiPlay out");
        else
            out->openPort(0, "FMidiPlay out");
    }
}

void Midi_Port_Instrument::set_midi_virtual_output()
{
    RtMidi::Api api = out_ ? out_->getCurrentApi() : RtMidi::UNSPECIFIED;
    out_.reset();

    RtMidiOut *out = new RtMidiOut(api, "FMidiPlay");
    out_.reset(out);
    out->openVirtualPort("FMidiPlay out");
}
