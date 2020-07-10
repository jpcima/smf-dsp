//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(ADEV_SOUNDIO)
#include "adev_soundio.h"
#include "utility/strings.h"
#include "utility/logs.h"
#include <gsl/gsl>
#include <cmath>
#include <cstring>

Audio_Device_Soundio::Audio_Device_Soundio()
{
    constexpr unsigned initial_temp_frames = 4096;
    temp_.reset(new float[2 * initial_temp_frames]);
    temp_frames_ = initial_temp_frames;
}

bool Audio_Device_Soundio::init(double desired_sample_rate, double desired_latency)
{
    shutdown();

    SoundIo_u client{soundio_create()};
    if (!client)
        throw std::bad_alloc();

    int err = soundio_connect(client.get());
    if (err) {
        Log::e("Cannot connect audio client: %s", soundio_strerror(err));
        return false;
    }

    Log::i("Audio interface: %s", soundio_backend_name(client->current_backend));

    soundio_flush_events(client.get());

    int device_index = soundio_default_output_device_index(client.get());
    if (device_index < 0) {
        Log::e("Cannot find an audio device for output");
        return false;
    }

    SoundIoDevice_u device{soundio_get_output_device(client.get(), device_index)};
    if (!device)
        throw std::bad_alloc();

    Log::i("Audio device: %s", device->name);

    if (device->probe_error) {
        Log::e("Cannot probe audio device: %s", soundio_strerror(device->probe_error));
        return false;
    }

    SoundIoOutStream_u outstream{soundio_outstream_create(device.get())};
    if (!outstream)
        throw std::bad_alloc();

    outstream->format = SoundIoFormatFloat32NE;
    outstream->sample_rate = static_cast<int>(desired_sample_rate);
    outstream->layout = *soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);
    outstream->software_latency = desired_latency;
    outstream->userdata = this;
    outstream->write_callback = &write_callback;
    outstream->name = PROGRAM_DISPLAY_NAME;

    err = soundio_outstream_open(outstream.get());
    if (err) {
        Log::e("Cannot open audio stream: %s", soundio_strerror(err));
        return false;
    }

    if (outstream->layout_error) {
        Log::e("Cannot set channel layout: %s", soundio_strerror(outstream->layout_error));
        return false;
    }

    audio_rate_ = outstream->sample_rate;
    audio_latency_ = outstream->software_latency;
    nominal_buffer_frames_ = static_cast<unsigned>(std::ceil(audio_latency_ * audio_rate_));

    Log::i("Software latency: %g s", audio_latency_);
    Log::i("Nominal buffer size: %u", nominal_buffer_frames_);

    client_ = std::move(client);
    outstream_ = std::move(outstream);

    return true;
}

void Audio_Device_Soundio::shutdown()
{
    outstream_.reset();
    client_.reset();
    active_ = false;
}

bool Audio_Device_Soundio::start()
{
    if (active_)
        return true;

    SoundIoOutStream *outstream = outstream_.get();
    if (!outstream)
        return false;

    int err = soundio_outstream_start(outstream);
    if (err) {
        Log::e("Cannot start audio stream: %s\n", soundio_strerror(err));
        return false;
    }

    active_ = true;
    return true;
}

double Audio_Device_Soundio::latency() const
{
    return audio_latency_;
}

double Audio_Device_Soundio::sample_rate() const
{
    return audio_rate_;
}

void Audio_Device_Soundio::write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
    Audio_Device_Soundio *self = reinterpret_cast<Audio_Device_Soundio *>(outstream->userdata);

    unsigned nframes = self->nominal_buffer_frames_;
    nframes = std::max<unsigned>(nframes, frame_count_min);
    nframes = std::min<unsigned>(nframes, frame_count_max);

    SoundIoChannelArea *areas_ptr;
    int err = soundio_outstream_begin_write(outstream, &areas_ptr, reinterpret_cast<int *>(&nframes));
    if (err) {
        Log::e("Unrecoverable audio stream error: %s", soundio_strerror(err));
        throw std::runtime_error("soundio_outstream_begin_write");
    }

    const std::array<SoundIoChannelArea, 2> areas{areas_ptr[0], areas_ptr[1]};

    bool is_interleaved = areas[0].ptr + sizeof(float) == areas[1].ptr &&
        areas[0].step == 2 * sizeof(float) && areas[1].step == 2 * sizeof(float);

    if (is_interleaved)
        self->process_cycle(reinterpret_cast<float *>(areas[0].ptr), nframes);
    else {
        float *temp = self->get_temp_buffer(nframes);
        self->process_cycle(temp, nframes);
        for (unsigned i = 0; i < nframes; ++i) {
            float *ch1 = reinterpret_cast<float *>(areas[0].ptr + i * areas[0].step);
            float *ch2 = reinterpret_cast<float *>(areas[1].ptr + i * areas[1].step);
            *ch1 = temp[2 * i];
            *ch2 = temp[2 * i + 1];
        }
    }

    err = soundio_outstream_end_write(outstream);
    if (err && err != SoundIoErrorUnderflow) {
        Log::e("Unrecoverable audio stream error: %s", soundio_strerror(err));
        throw std::runtime_error("soundio_outstream_begin_write");
    }
}

float *Audio_Device_Soundio::get_temp_buffer(unsigned nframes)
{
    unsigned temp_frames_current = temp_frames_;
    if (nframes <= temp_frames_current)
        return temp_.get();

    while (nframes > temp_frames_current)
        temp_frames_current *= 2;

    float *temp = new float[2 * temp_frames_current];
    temp_.reset(temp);
    return temp;
}
#endif
