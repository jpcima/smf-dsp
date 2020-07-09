//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(ADEV_RTAUDIO)
#include "adev.h"
#include <RtAudio.h>
#include <memory>

class Audio_Device_Rt : public Audio_Device {
public:
    const char *audio_system_name() const noexcept override { return "RtAudio"; }

    bool init(double desired_sample_rate, double desired_latency) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static int rtaudio_callback(void *output_buffer, void *, unsigned nframes, double, RtAudioStreamStatus, void *user_data);

private:
    std::unique_ptr<RtAudio> audio_;
    double audio_rate_ = 0.0;
    double audio_latency_ = 0.0;
    bool active_ = false;
};
#endif
