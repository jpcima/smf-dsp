//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

class Audio_Device {
public:
    virtual ~Audio_Device() {}

    typedef void (audio_callback_t)(float *output, unsigned nframes, void *user_data);

    virtual bool init(double desired_latency, audio_callback_t *cb, void *cbdata) = 0;
    virtual void shutdown() = 0;
    virtual bool start() = 0;
    virtual double latency() const = 0;
    virtual double sample_rate() const = 0;
};
