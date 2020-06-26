//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "adev_sdl.h"
#include "utility/logs.h"
#include <algorithm>
#include <cstring>

Audio_Device_SDL::Audio_Device_SDL()
{
}

Audio_Device_SDL::~Audio_Device_SDL()
{
}

template <class I>
static I highest_bit_only(I x)
{
    I bit = 0;
    for (unsigned i = 0; i < 8 * sizeof(x); ++i)
        bit = (x & (I(1) << i)) ? (I(1) << i) : bit;
    return bit;
}

bool Audio_Device_SDL::init(double desired_sample_rate, double desired_latency)
{
    shutdown();

    Log::i("Audio interface: SDL");

    SDL_AudioSpec spec;
    std::memset(&spec, 0, sizeof(spec));
    spec.format = AUDIO_F32;
    spec.channels = 2;
    spec.callback = &sdl_audio_callback;
    spec.userdata = this;

    unsigned audio_rate = (unsigned)desired_sample_rate;
    unsigned audio_buffer_size = (unsigned)std::ceil(desired_latency * audio_rate);

    audio_buffer_size = highest_bit_only(audio_buffer_size);
    audio_buffer_size = std::max(16u, std::min(32768u, audio_buffer_size));

    spec.freq = audio_rate;
    spec.samples = audio_buffer_size;

    SDL_AudioDeviceID id = SDL_OpenAudioDevice(nullptr, false, &spec, nullptr, 0);
    if (id <= 0) {
        Log::e("%s", SDL_GetError());
        return false;
    }

    handle_.reset(id);

    sample_rate_ = spec.freq;
    latency_ = (double)spec.samples / (double)spec.freq;

    return true;
}

void Audio_Device_SDL::shutdown()
{
    handle_.reset();
    active_ = false;
}

bool Audio_Device_SDL::start()
{
    if (active_)
        return true;

    if (!handle_)
        return false;

    SDL_PauseAudioDevice(handle_.get(), 0);

    active_ = true;
    return true;
}

double Audio_Device_SDL::latency() const
{
    return latency_;
}

double Audio_Device_SDL::sample_rate() const
{
    return sample_rate_;
}

void SDLCALL Audio_Device_SDL::sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
    Audio_Device_SDL *self = (Audio_Device_SDL *)userdata;
    int nframes = len / (2 * sizeof(float));
    self->process_cycle(reinterpret_cast<float *>(stream), nframes);
}

///
Audio_Device_SDL::Handle::Handle(SDL_AudioDeviceID id) noexcept
    : id_(id)
{
}

Audio_Device_SDL::Handle::~Handle() noexcept
{
    reset();
}

Audio_Device_SDL::Handle::operator bool() const noexcept
{
    return id_ > 0;
}

Audio_Device_SDL::Handle::Handle(Handle &&other)
{
    id_ = other.id_, other.id_ = 0;
}

auto Audio_Device_SDL::Handle::operator=(Handle &&other) noexcept -> Handle &
{
    return reset(other.id_), other.id_ = 0, *this;
}

SDL_AudioDeviceID Audio_Device_SDL::Handle::get() const noexcept
{
    return id_;
}

void Audio_Device_SDL::Handle::reset(SDL_AudioDeviceID id) noexcept
{
    if (id_ == id) return;
    if (id_ > 0) SDL_CloseAudioDevice(id_);
    id_ = id;
}
