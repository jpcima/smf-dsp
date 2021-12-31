//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "player/instruments/dummy.h"
#include <cstdio>

void Dummy_Instrument::handle_send_message(const uint8_t *data, unsigned len, double ts, uint8_t flags)
{
    (void)ts;
    (void)flags;

    if (false) {
        for (unsigned i = 0; i < len; ++i)
            fprintf(stderr, "%s%02X", i ? " " : "", data[i]);
        fputc('\n', stderr);
    }
}
