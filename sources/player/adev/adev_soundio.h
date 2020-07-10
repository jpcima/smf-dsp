//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(ADEV_SOUNDIO)
#include "adev.h"
#include <soundio/soundio.h>
#include <list>
#include <array>
#include <memory>

class Audio_Device_Soundio : public Audio_Device {
public:
    Audio_Device_Soundio();

    const char *audio_system_name() const noexcept override { return "SoundIo"; }

    bool init(double desired_sample_rate, double desired_latency) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
    float *get_temp_buffer(unsigned nframes);

private:
    struct SoundIo_deleter {
        void operator()(SoundIo *x) const noexcept { soundio_destroy(x); }
    };
    typedef std::unique_ptr<SoundIo, SoundIo_deleter> SoundIo_u;

    struct SoundIoDevice_deleter {
        void operator()(SoundIoDevice *x) const noexcept { soundio_device_unref(x); }
    };
    typedef std::unique_ptr<SoundIoDevice, SoundIoDevice_deleter> SoundIoDevice_u;

    struct SoundIoOutStream_deleter {
        void operator()(SoundIoOutStream *x) const noexcept { soundio_outstream_destroy(x); }
    };
    typedef std::unique_ptr<SoundIoOutStream, SoundIoOutStream_deleter> SoundIoOutStream_u;

private:
    double audio_rate_ = 0.0;
    double audio_latency_ = 0.0;
    unsigned nominal_buffer_frames_ = 0;

    std::unique_ptr<float[]> temp_;
    unsigned temp_frames_ = 0;

    SoundIo_u client_;
    SoundIoOutStream_u outstream_;
    bool active_ = false;
};
#endif
