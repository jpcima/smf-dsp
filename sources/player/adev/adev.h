//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <mutex>

class Audio_Device {
public:
    virtual ~Audio_Device() {}

    static Audio_Device *create_best_for_system();

    typedef void (audio_callback_t)(float *output, unsigned nframes, void *user_data);

    virtual bool init(double desired_latency) = 0;
    virtual void shutdown() = 0;
    void set_callback(audio_callback_t *cb, void *cbdata);
    virtual bool start() = 0;
    virtual double latency() const = 0;
    virtual double sample_rate() const = 0;

protected:
    void process_cycle(float *output, unsigned nframes);
    std::mutex cbmutex_;

private:
    audio_callback_t *cb_ = nullptr;
    void *cbdata_ = nullptr;
};
