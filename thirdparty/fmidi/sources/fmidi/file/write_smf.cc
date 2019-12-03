//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi/fmidi.h"
#include "fmidi/fmidi_internal.h"
#include "fmidi/u_stdio.h"
#include <cstring>
#include <cassert>

static void write_vlq(uint32_t value, Writer &writer)
{
    unsigned shift = 28;
    unsigned mask = (1u << 7) - 1;
    while (shift > 0 && ((value >> shift) & mask) == 0)
        shift -= 7;
    while (shift > 0) {
        writer.put(((value >> shift) & mask) | (1u << 7));
        shift -= 7;
    }
    writer.put(value & mask);
}

static bool fmidi_smf_write(const fmidi_smf_t *smf, Writer &writer)
{
    writer.write("MThd", 4);

    const uint32_t header_size = 6;
    writer.writeBE(&header_size, 4);

    const fmidi_smf_info_t *info = fmidi_smf_get_info(smf);
    const uint16_t track_count = info->track_count;
    writer.writeBE(&info->format, 2);
    writer.writeBE(&track_count, 2);
    writer.writeBE(&info->delta_unit, 2);

    for (unsigned i = 0; i < track_count; ++i) {
        writer.write("MTrk", 4);

        off_t off_track_length = writer.tell();

        uint32_t track_length = 0;
        writer.writeBE(&track_length, 4);

        int running_status = -1;

        fmidi_track_iter_t iter;
        fmidi_smf_track_begin(&iter, i);

        const fmidi_event_t *event;
        while ((event = fmidi_smf_track_next(smf, &iter))) {
            switch (event->type) {
            case fmidi_event_meta:
                write_vlq(event->delta, writer);
                writer.put(0xff);
                writer.put(event->data[0]);
                write_vlq(event->datalen - 1, writer);
                writer.write(event->data + 1, event->datalen - 1);
                running_status = -1;
                break;
            case fmidi_event_message:
            {
                write_vlq(event->delta, writer);
                uint8_t status = event->data[0];
                if (status == 0xf0) {
                    writer.put(0xf0);
                    write_vlq(event->datalen - 1, writer);
                    writer.write(event->data + 1, event->datalen - 1);
                    running_status = -1;
                }
                else if ((int)status == running_status)
                    writer.write(event->data + 1, event->datalen - 1);
                else {
                    writer.write(event->data, event->datalen);
                    running_status = status;
                }
                break;
            }
            case fmidi_event_escape:
                write_vlq(event->delta, writer);
                writer.put(0xf7);
                write_vlq(event->datalen, writer);
                writer.write(event->data, event->datalen);
                running_status = -1;
                break;
            case fmidi_event_xmi_timbre:
            case fmidi_event_xmi_branch_point:
                break;
            }
        }

        off_t off_track_end = writer.tell();

        track_length =
            std::make_unsigned<off_t>::type(off_track_end) -
            std::make_unsigned<off_t>::type(off_track_length) - 4;

        writer.seek(off_track_length, SEEK_SET);
        writer.writeBE(&track_length, 4);
        writer.seek(off_track_end, SEEK_SET);
    }

    return true;
}

bool fmidi_smf_mem_write(const fmidi_smf_t *smf, uint8_t **data, size_t *length)
{
    std::vector<uint8_t> mem;
    mem.reserve(8192);

    Memory_Writer writer(mem);
    if (!fmidi_smf_write(smf, writer))
        return false;

    assert(data);
    assert(length);

    if (!(*data = (uint8_t *)malloc(mem.size())))
        throw std::bad_alloc();

    memcpy(*data, mem.data(), mem.size());
    *length = mem.size();

    return true;
}

bool fmidi_smf_file_write(const fmidi_smf_t *smf, const char *filename)
{
    unique_FILE fh(fopen(filename, "wb"));
    if (!fh)
        RET_FAIL(false, fmidi_err_output);

    return fmidi_smf_stream_write(smf, fh.get());
}

bool fmidi_smf_stream_write(const fmidi_smf_t *smf, FILE *stream)
{
    Stream_Writer writer(stream);
    if (!fmidi_smf_write(smf, writer))
        return false;

    if (fflush(stream) != 0)
        RET_FAIL(false, fmidi_err_output);

    return true;
}
