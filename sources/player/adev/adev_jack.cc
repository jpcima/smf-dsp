//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(ADEV_JACK)
#include "adev_jack.h"
#include "utility/strings.h"
#include <gsl.hpp>
#include <cmath>
#include <cstring>

bool Audio_Device_Jack::is_available()
{
    jack_client_u client(jack_client_open(PROGRAM_DISPLAY_NAME, JackNoStartServer, nullptr));
    return client != nullptr;
}

bool Audio_Device_Jack::init(double desired_latency)
{
    shutdown();

    jack_client_u client(jack_client_open(PROGRAM_DISPLAY_NAME, JackNoStartServer, nullptr));
    if (!client)
        return false;

    ports_[0] = jack_port_register(client.get(), "output_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!ports_[0])
        return false;

    ports_[1] = jack_port_register(client.get(), "output_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!ports_[1])
        return false;

    jack_set_process_callback(client.get(), &jack_audio_callback, this);

    double audio_rate = jack_get_sample_rate(client.get());
    jack_nframes_t audio_buffer_size = jack_get_buffer_size(client.get());

    buffer_.reset(new float[2 * audio_buffer_size]);

    client_ = std::move(client);
    audio_rate_ = audio_rate;
    audio_latency_ = audio_buffer_size / audio_rate;

    return true;
}

void Audio_Device_Jack::shutdown()
{
    client_.reset();
}

bool Audio_Device_Jack::start()
{
    jack_client_t *client = client_.get();

    if (jack_activate(client) != 0)
        return false;

    connect_physical_ports();

    return true;
}

double Audio_Device_Jack::latency() const
{
    return audio_latency_;
}

double Audio_Device_Jack::sample_rate() const
{
    return audio_rate_;
}

int Audio_Device_Jack::jack_audio_callback(jack_nframes_t nframes, void *user_data)
{
    Audio_Device_Jack *self = reinterpret_cast<Audio_Device_Jack *>(user_data);

    float *buffer = self->buffer_.get();
    self->process_cycle(buffer, nframes);

    float *out1 = reinterpret_cast<float *>(jack_port_get_buffer(self->ports_[0], nframes));
    float *out2 = reinterpret_cast<float *>(jack_port_get_buffer(self->ports_[1], nframes));
    for (jack_nframes_t i = 0; i < nframes; ++i) {
        out1[i] = buffer[2 * i];
        out2[i] = buffer[2 * i + 1];
    }

    return 0;
}

void Audio_Device_Jack::connect_physical_ports()
{
    jack_client_t *client = client_.get();

    const char **ports = jack_get_ports(
        client, nullptr, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput|JackPortIsPhysical);

    if (!ports)
        return;

    gsl::cstring_span prefix;
    if (const char *name = ports[0]) {
        const char *pos = std::strchr(name, ':');
        if (pos)
            prefix = gsl::cstring_span(name, pos + 1 - name);
    }

    if (!prefix.empty()) {
        jack_connect(client, jack_port_name(ports_[0]), ports[0]);
        for (unsigned i = 1; i < 2 && ports[i]; ++i) {
            if (string_starts_with<char>(ports[i], prefix))
                jack_connect(client, jack_port_name(ports_[i]), ports[i]);
        }
    }

    jack_free(ports);
}
#endif
