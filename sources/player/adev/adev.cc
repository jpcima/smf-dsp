//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "adev.h"
#include "adev_sdl.h"
#include "adev_haiku.h"
#include "adev_jack.h"
#include <memory>
#include <cstring>

Audio_Device *Audio_Device::create_best_for_system()
{
    std::unique_ptr<Audio_Device> adev;

#if !defined(__HAIKU__)
#if defined(ADEV_JACK)
    if (!adev && Audio_Device_Jack::is_available())
        adev.reset(new Audio_Device_Jack);
#endif
    if (!adev)
        adev.reset(new Audio_Device_SDL);
#else
    if (!adev)
        adev.reset(new Audio_Device_Haiku);
#endif

    return adev.release();
}

void Audio_Device::set_callback(audio_callback_t *cb, void *cbdata)
{
    std::lock_guard<std::mutex> lock(cbmutex_);

    cb_ = cb;
    cbdata_ = cbdata;
}

void Audio_Device::process_cycle(float *output, unsigned nframes)
{
    std::unique_lock<std::mutex> lock(cbmutex_, std::try_to_lock);

    if (lock.owns_lock() && cb_)
        cb_(reinterpret_cast<float *>(output), nframes, cbdata_);
    else
        std::memset(output, 0, 2 * nframes * sizeof(float));
}
