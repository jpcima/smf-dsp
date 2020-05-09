//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "adev.h"
#include "adev_rtaudio.h"
#include "adev_haiku.h"
#include <memory>

Audio_Device *Audio_Device::create_best_for_system()
{
    std::unique_ptr<Audio_Device> adev;

#if !defined(__HAIKU__)
    adev.reset(new Audio_Device_Rt);
#else
    adev.reset(new Audio_Device_Haiku);
#endif

    return adev.release();
}
