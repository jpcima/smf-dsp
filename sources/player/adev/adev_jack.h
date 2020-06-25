//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(ADEV_JACK)
#include "adev.h"
#include <jack/jack.h>
#include <list>
#include <array>
#include <memory>

class Audio_Device_Jack : public Audio_Device {
public:
    static bool is_available();

    bool init(double desired_latency) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static int jack_audio_callback(jack_nframes_t nframes, void *user_data);
    void connect_physical_ports();

    typedef std::array<std::list<std::string>, 2> Connections;
    Connections identify_physical_ports();

private:
    struct jack_client_deleter {
        void operator()(jack_client_t *x) const noexcept { jack_client_close(x); }
    };
    typedef std::unique_ptr<jack_client_t, jack_client_deleter> jack_client_u;

private:
    std::array<jack_port_t *, 2> ports_ {};
    std::unique_ptr<float[]> buffer_;
    double audio_rate_ = 0.0;
    double audio_latency_ = 0.0;
    jack_client_u client_;
    bool active_ = false;
};
#endif
