//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(ADEV_HAIKU)
#include "adev.h"
#include <MediaKit.h>
#include <memory>

class Audio_Device_Haiku : public Audio_Device {
public:
    const char *audio_system_name() const noexcept override { return "MediaKit"; }

    bool init(double desired_sample_rate, double desired_latency) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static void haiku_audio_callback(void *user_data, void *output_buffer, size_t size, const media_raw_audio_format &)

private:
    std::unique_ptr<BSoundPlayer> audio_;
    double audio_rate_ = 0.0;
    double audio_latency_ = 0.0;
    bool active_ = false;
};
#endif
