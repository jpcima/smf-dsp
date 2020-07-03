//          Copyright Jean Pierre Cimalando 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "adev.h"
#include <SDL.h>
#include <memory>
struct SDL_AudioSpec;

class Audio_Device_SDL : public Audio_Device {
public:
    Audio_Device_SDL();
    ~Audio_Device_SDL();

    const char *audio_system_name() const noexcept override { return "SDL"; }

    bool init(double desired_sample_rate, double desired_latency) override;
    void shutdown() override;
    bool start() override;
    double latency() const override;
    double sample_rate() const override;

private:
    static void SDLCALL sdl_audio_callback(void *userdata, Uint8 *stream, int len);

private:
    class Handle {
    public:
        explicit Handle(SDL_AudioDeviceID id = 0) noexcept;
        Handle(const Handle &) = delete;
        Handle(Handle &&other);
        ~Handle() noexcept;

        Handle &operator=(const Handle &) = delete;
        Handle &operator=(Handle &&other) noexcept;

        explicit operator bool() const noexcept;

        SDL_AudioDeviceID get() const noexcept;
        void reset(SDL_AudioDeviceID id = 0) noexcept;

    private:
        SDL_AudioDeviceID id_ {};
    };

private:
    Handle handle_;
    bool active_ = false;
    double sample_rate_ {};
    double latency_ {};
};
