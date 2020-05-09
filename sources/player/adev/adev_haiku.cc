//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(__HAIKU__)
#include "adev_haiku.h"
#include <cmath>
#include <cstring>

bool Audio_Device_Haiku::init(double desired_latency, audio_callback_t *cb, void *cbdata)
{
    shutdown();

    cb_ = cb;
    cbdata_ = cbdata;

    media_raw_audio_format format = media_raw_audio_format::wildcard;
    format.channel_count = 2;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_HOST_ENDIAN;
    //NOTE: latency setting not used

    std::unique_ptr<BSoundPlayer> audio(
        new BSoundPlayer(&format, PROGRAM_DISPLAY_NAME, &audio_callback, nullptr, this));

    if (audio->InitCheck() != B_OK)
        return false;

    format = audio->Format();

    audio_ = std::move(audio);
    audio_rate_ = format.frame_rate;
    audio_latency_ = format.buffer_size / format.frame_rate;

    return true;
}

void Audio_Device_Haiku::shutdown()
{
    audio_.reset();
}

bool Audio_Device_Haiku::start()
{
    if (audio_->Start() != B_OK)
        return false;

    audio_->SetHasData(true);
    return true;
}

double Audio_Device_Haiku::latency() const
{
    return audio_latency_;
}

double Audio_Device_Haiku::sample_rate() const
{
    return audio_rate_;
}

void Audio_Device_Haiku::haiku_audio_callback(void *user_data, void *output_buffer, size_t size, const media_raw_audio_format &)
{
    const unsigned nframes = size / (2 * sizeof(float));

    Audio_Device_Haiku *self = reinterpret_cast<Audio_Device_Haiku *>(user_data);
    if (self->cb_)
        self->cb_(reinterpret_cast<float *>(output_buffer), nframes, self->cbdata_);
    else
        std::memset(output_buffer, 0, 2 * nframes * sizeof(float));
}
#endif
