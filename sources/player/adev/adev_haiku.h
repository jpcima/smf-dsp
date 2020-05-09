//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(__HAIKU__)
#include "adev.h"
#include <MediaKit.h>
#include <memory>

class Audio_Device_Haiku : public Audio_Device {
public:
    bool init(double desired_latency, audio_callback_t *cb, void *cbdata) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static void haiku_audio_callback(void *user_data, void *output_buffer, size_t size, const media_raw_audio_format &)

private:
    std::unique_ptr<BSoundPlayer> audio_;
    audio_callback_t *cb_ = nullptr;
    void *cbdata_ = nullptr;
    double audio_rate_ = 0.0;
    double audio_latency_ = 0.0;
};
#endif
