//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player/instruments/port.h"
#include "utility/logs.h"
#include <RtMidi.h>
#include <algorithm>
#include <cstring>

Midi_Port_Instrument::Midi_Port_Instrument()
{
    open_midi_output(gsl::cstring_span());
}

Midi_Port_Instrument::~Midi_Port_Instrument()
{
}

void Midi_Port_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    (void)ts;
    (void)flags;

    RtMidiOut &out = *out_;
    out.sendMessage(data, len);
}

void Midi_Port_Instrument::handle_midi_error(int type, const std::string &text)
{
    Log::e("%s", text.c_str());
    midi_error_status_ = (type < RtMidiError::UNSPECIFIED) ? 0 : type;
}

std::vector<Midi_Output> Midi_Port_Instrument::get_midi_outputs()
{
    std::vector<Midi_Output> ports;

    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);

    for (RtMidi::Api api : apis) {
        if (api == RtMidi::RTMIDI_DUMMY)
            continue;

        std::string api_name = RtMidi::getApiName(api);
        bool has_virtual_port = api != RtMidi::WINDOWS_MM;

        RtMidiOut out(api);

        if (has_virtual_port) {
            Midi_Output port;
            port.id = api_name + "://vport";
            port.name = api_name + ": Virtual port";
            ports.push_back(std::move(port));
        }

        for (size_t i = 0, n = out.getPortCount(); i < n; ++i) {
            std::string port_name = out.getPortName(i);
            if (port_name.empty())
                continue;
            Midi_Output port;
            port.id = api_name + "://port/" + port_name;
            port.name = api_name + ": " + port_name;
            ports.push_back(std::move(port));
        }
    }

    return ports;
}

void Midi_Port_Instrument::open_midi_output(gsl::cstring_span id)
{
    RtMidiOut *out = out_.get();
    if (out) {
        initialize();
        out_.reset();
    }

    // extract API name
    RtMidi::Api api = RtMidi::UNSPECIFIED;
    auto pos = std::find(id.begin(), id.end(), ':');
    if (pos != id.end()) {
        std::string api_name(id.begin(), pos);
        api = RtMidi::getCompiledApiByName(api_name);
        id = gsl::cstring_span(pos + 1, id.end());
    }

    // open client
    midi_error_status_ = 0;
    out = new RtMidiOut(api, PROGRAM_DISPLAY_NAME);
    out_.reset(out);

    out->setErrorCallback(
        +[](RtMidiError::Type type, const std::string &error, void *data) {
             static_cast<Midi_Port_Instrument *>(data)->handle_midi_error(type, error);
         }, this);

    api = out->getCurrentApi();

    if (id.empty())
        return;

    // export port
    if (id.size() > 7 && !memcmp(id.data(), "//port/", 7)) {
        gsl::cstring_span port_id(id.begin() + 7, id.end());
        bool port_found = false;
        for (size_t i = 0, n = out->getPortCount(); i < n && !port_found; ++i) {
            if ((port_found = out->getPortName(i) == port_id))
                out->openPort(i, PROGRAM_DISPLAY_NAME " out");
        }
    }
    else if (id.size() == 7 && !memcmp(id.data(), "//vport", 7))
        out->openVirtualPort(PROGRAM_DISPLAY_NAME " out");
}

void Midi_Port_Instrument::close_midi_output()
{
    RtMidiOut &out = *out_;
    out.closePort();
}
