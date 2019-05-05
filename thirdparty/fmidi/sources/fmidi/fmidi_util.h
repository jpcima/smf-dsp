//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi.h"
#include <vector>

struct fmidi_raw_track {
    std::unique_ptr<uint8_t[]> data;
    uint32_t length;
};

struct fmidi_smf {
    fmidi_smf_info_t info;
    std::unique_ptr<fmidi_raw_track[]> track;
};

//------------------------------------------------------------------------------
uintptr_t fmidi_event_pad(uintptr_t size);
fmidi_event_t *fmidi_event_alloc(std::vector<uint8_t> &buf, uint32_t datalen);
unsigned fmidi_message_sizeof(uint8_t id);

//------------------------------------------------------------------------------
inline uintptr_t fmidi_event_pad(uintptr_t size)
{
    uintptr_t nb = size % alignof(fmidi_event_t);
    return nb ? (size + alignof(fmidi_event_t) - nb) : size;
}
