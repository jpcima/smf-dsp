//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi/fmidi.h"
#include "fmidi/fmidi_util.h"
#include "fmidi/fmidi_internal.h"
#include "fmidi/u_memstream.h"
#include "fmidi/u_stdio.h"
#include <algorithm>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
# define fileno _fileno
#endif

#define FOURCC(x)                               \
    (((uint8_t)(x)[0] << 24) |                  \
     ((uint8_t)(x)[1] << 16) |                  \
     ((uint8_t)(x)[2] << 8) |                   \
     ((uint8_t)(x)[3]))

struct fmidi_xmi_timb {
    uint32_t patch;
    uint32_t bank;
};

struct fmidi_xmi_rbrn {
    uint32_t id;
    uint32_t dest;
};

struct fmidi_xmi_note {
    uint32_t delta;
    uint8_t channel;
    uint8_t note;
    uint8_t velo;
};

static bool operator<(const fmidi_xmi_note &a, const fmidi_xmi_note &b)
{
    return a.delta < b.delta;
}

static void fmidi_xmi_emit_noteoffs(
    uint32_t *pdelta, std::vector<fmidi_xmi_note> &noteoffs,
    std::vector<uint8_t> &evbuf)
{
    uint32_t delta = *pdelta;

    std::sort(noteoffs.begin(), noteoffs.end());

    size_t i = 0;
    size_t n = noteoffs.size();

    for (; i < n; ++i) {
        fmidi_xmi_note xn = noteoffs[i];
        if (delta < xn.delta)
            break;

        fmidi_event_t *event = fmidi_event_alloc(evbuf, 3);
        event->type = fmidi_event_message;
        event->delta = xn.delta;
        event->datalen = 3;

        uint8_t *data = event->data;
        data[0] = 0x80 | xn.channel;
        data[1] = xn.note;
        data[2] = xn.velo;

        delta -= xn.delta;
        for (size_t k = i + 1; k < n; ++k)
            noteoffs[k].delta -= xn.delta;
    }

    size_t j = 0;
    for (; i < n; ++i) {
        fmidi_xmi_note xn = noteoffs[i];
        noteoffs[j++] = xn;
    }
    noteoffs.resize(j);

    *pdelta = delta;
}

static bool fmidi_xmi_read_events(
    memstream &mb, fmidi_raw_track &track,
    const fmidi_xmi_timb *timb, uint32_t timb_count,
    const fmidi_xmi_rbrn *rbrn, uint32_t rbrn_count)
{
    memstream_status ms;
    std::vector<uint8_t> evbuf;
    evbuf.reserve(8192);

    std::vector<fmidi_xmi_note> noteoffs;
    noteoffs.reserve(128);

    for (uint32_t i = 0; i < timb_count; ++i) {
        fmidi_event_t *event = fmidi_event_alloc(evbuf, 2);
        event->type = fmidi_event_xmi_timbre;
        event->delta = 0;
        event->datalen = 2;
        uint8_t * data = event->data;
        data[0] = timb[i].patch;
        data[1] = timb[i].bank;
    }

    bool eot = false;
    while (!eot) {
        uint32_t delta = 0;
        unsigned status = 0;

        size_t branch = ~(size_t)0;
        for (uint32_t i = 0; i < rbrn_count && branch == ~(size_t)0; ++i) {
            if (rbrn[i].dest == mb.getpos())
                branch = i;
        }

        while (!(status & 128)) {
            if ((ms = mb.readbyte(&status)))
                RET_FAIL(false, (fmidi_status)ms);
            delta += (status & 128) ? 0 : status;
        }

        if (branch != ~(size_t)0) {
            fmidi_event_t *event = fmidi_event_alloc(evbuf, 1);
            event->type = fmidi_event_xmi_branch_point;
            event->delta = delta;
            event->datalen = 1;
            event->data[0] = rbrn[branch].id;
            delta = 0;
        }

        fmidi_xmi_emit_noteoffs(&delta, noteoffs, evbuf);

        if (status == 0xff) {
            unsigned type;
            uint32_t length;
            if ((ms = mb.readbyte(&type)) ||
                (ms = mb.readvlq(&length)))
                RET_FAIL(false, (fmidi_status)ms);

            const uint8_t *data = mb.read(length);
            if (!data)
                RET_FAIL(false, fmidi_err_eof);

            eot = type == 0x2F;

            if (eot) {
                // emit later
            }
            else if (type == 0x51) {
                // don't emit tempo change
            }
            else {
                fmidi_event_t *event = fmidi_event_alloc(evbuf, length + 1);
                event->type = fmidi_event_meta;
                event->delta = delta;
                event->datalen = length + 1;
                event->data[0] = type;
                memcpy(event->data + 1, data, length);
            }
        }
        else if (status == 0xf0) {
            uint32_t length;
            if ((ms = mb.readvlq(&length)))
                RET_FAIL(false, (fmidi_status)ms);

            const uint8_t *data = mb.read(length);
            if (!data)
                RET_FAIL(false, fmidi_err_eof);

            fmidi_event_t *event = fmidi_event_alloc(evbuf, length + 1);
            event->type = fmidi_event_message;
            event->delta = delta;
            event->datalen = length + 1;
            event->data[0] = 0xf0;
            memcpy(event->data + 1, data, length);
        }
        else if (status == 0xf7) {
            RET_FAIL(false, fmidi_err_format);
        }
        else if ((status & 0xf0) == 0x90) {
            mb.setpos(mb.getpos() - 1);
            const uint8_t *data = mb.read(3);
            if (!data)
                RET_FAIL(false, fmidi_err_eof);

            uint32_t interval;
            if ((ms = mb.readvlq(&interval)))
                RET_FAIL(false, (fmidi_status)ms);

            fmidi_event_t *event = fmidi_event_alloc(evbuf, 3);
            event->type = fmidi_event_message;
            event->delta = delta;
            event->datalen = 3;
            memcpy(event->data, data, 3);

            fmidi_xmi_note noteoff;
            noteoff.delta = interval;
            noteoff.channel = data[0] & 15;
            noteoff.note = data[1];
            noteoff.velo = data[2];
            noteoffs.push_back(noteoff);
        }
        else {
            unsigned length = fmidi_message_sizeof(status);
            mb.setpos(mb.getpos() - 1);
            const uint8_t *data = mb.read(length);
            if (!data)
                RET_FAIL(false, fmidi_err_eof);

            fmidi_event_t *event = fmidi_event_alloc(evbuf, length);
            event->type = fmidi_event_message;
            event->delta = delta;
            event->datalen = length;
            memcpy(event->data, data, length);
        }
    }

    {
        uint32_t delta = UINT32_MAX;
        fmidi_xmi_emit_noteoffs(&delta, noteoffs, evbuf);
    }

    {
        fmidi_event_t *event = fmidi_event_alloc(evbuf, 1);
        event->type = fmidi_event_meta;
        event->delta = 0;
        event->datalen = 1;
        event->data[0] = 0x2F;
    }

    uint32_t evdatalen = track.length = evbuf.size();
    uint8_t *evdata = new uint8_t[evdatalen];
    track.data.reset(evdata);
    memcpy(evdata, evbuf.data(), evdatalen);

    return true;
}

static bool fmidi_xmi_read_track(memstream &mb, fmidi_raw_track &track)
{
    memstream_status ms;

    const uint8_t *fourcc;
    if (!(fourcc = mb.read(4)))
        RET_FAIL(false, fmidi_err_eof);
    if (memcmp(fourcc, "FORM", 4))
        RET_FAIL(false, fmidi_err_format);

    uint32_t formsize;
    if ((ms = mb.readintBE(&formsize, 4)))
        RET_FAIL(false, (fmidi_status)ms);

    const uint8_t *formdata = mb.read(formsize);
    if (!formdata)
        RET_FAIL(false, fmidi_err_eof);
    memstream mbform(formdata, formsize);

    if (!(fourcc = mbform.read(4)))
        RET_FAIL(false, fmidi_err_eof);
    if (memcmp(fourcc, "XMID", 4))
        RET_FAIL(false, fmidi_err_format);

    std::unique_ptr<fmidi_xmi_timb[]> timb;
    uint32_t timb_count = 0;
    std::unique_ptr<fmidi_xmi_rbrn[]> rbrn;
    uint32_t rbrn_count = 0;

    while (mbform.getpos() < mbform.endpos()) {
        if (!(fourcc = mbform.read(4)))
            RET_FAIL(false, fmidi_err_eof);

        uint32_t chunksize;
        if ((ms = mbform.readintBE(&chunksize, 4)))
            RET_FAIL(false, (fmidi_status)ms);

        const uint8_t *chunkdata = mbform.read(chunksize);
        if (!chunkdata)
            RET_FAIL(false, fmidi_err_eof);
        memstream mbchunk(chunkdata, chunksize);

        switch (FOURCC(fourcc)) {
            case FOURCC("TIMB"): {
                if ((ms = mbchunk.readintLE(&timb_count, 2)))
                    RET_FAIL(false, (fmidi_status)ms);

                timb.reset(new fmidi_xmi_timb[timb_count]);
                for (uint32_t i = 0; i < timb_count; ++i) {
                    if ((ms = mbchunk.readintLE(&timb[i].patch, 1)) ||
                        (ms = mbchunk.readintLE(&timb[i].bank, 1)))
                        RET_FAIL(false, (fmidi_status)ms);
                }

                break;
            }
            case FOURCC("RBRN"): {
                if ((ms = mbchunk.readintLE(&rbrn_count, 2)))
                    RET_FAIL(false, (fmidi_status)ms);

                rbrn.reset(new fmidi_xmi_rbrn[rbrn_count]);
                for (uint32_t i = 0; i < rbrn_count; ++i) {
                    if ((ms = mbchunk.readintLE(&rbrn[i].id, 2)) ||
                        (ms = mbchunk.readintLE(&rbrn[i].dest, 4)))
                        RET_FAIL(false, (fmidi_status)ms);
                    if (rbrn[i].id >= 128)
                        RET_FAIL(false, fmidi_err_format);
                }

                break;
            }
            case FOURCC("EVNT"):
                if (!fmidi_xmi_read_events(
                        mbchunk, track,
                        timb.get(), timb_count, rbrn.get(), rbrn_count))
                    return false;

                break;
        }

        if (mb.getpos() & 1) {
            if ((ms = mb.skip(1)))
                RET_FAIL(false, (fmidi_status)ms);
        }
    }

    return true;
}

uint32_t fmidi_xmi_update_unit(fmidi_smf_t *smf)
{
    uint32_t res = 1;
    const fmidi_event_t *evt;
    fmidi_track_iter_t it;
    fmidi_smf_track_begin(&it, 0);
    bool found = false;
    while (!found && (evt = fmidi_smf_track_next(smf, &it))) {
        if (evt->type == fmidi_event_meta) {
            uint8_t id = evt->data[0];
            if (id == 0x51 && evt->datalen == 4) {  // set tempo
                const uint8_t *d24 = &evt->data[1];
                uint32_t tempo = (d24[0] << 16) | (d24[1] << 8) | d24[2];
                res = 3;
                smf->info.delta_unit = tempo * res * 120 / 1000000;
                found = true;
            }
        }
    }
    return res;
}

fmidi_smf_t *fmidi_xmi_mem_read(const uint8_t *data, size_t length)
{
    const uint8_t header[] = {
        'F', 'O', 'R', 'M', 0, 0, 0, 14,
        'X', 'D', 'I', 'R', 'I', 'N', 'F', 'O', 0, 0, 0, 2
    };

    const uint8_t *start = std::search(
        data, data + length, header, header + sizeof(header));
    if (start == data + length)
        RET_FAIL(nullptr, fmidi_err_format);

    length = length - (start - data);
    data = start;

    // ensure padding to even size (The Lost Vikings)
    std::unique_ptr<uint8_t[]> padded;
    if (length & 1) {
        padded.reset(new uint8_t[length + 1]);
        memcpy(padded.get(), data, length);
        padded[length] = 0;
        data = padded.get();
        length = length + 1;
    }

    memstream mb(data + sizeof(header), length - sizeof(header));
    memstream_status ms;

    uint32_t ntracks;
    if ((ms = mb.readintLE(&ntracks, 2)))
        RET_FAIL(nullptr, (fmidi_status)ms);
    if (ntracks < 1)
        RET_FAIL(nullptr, fmidi_err_format);

    const uint8_t *fourcc;
    if (!(fourcc = mb.read(4)))
        RET_FAIL(nullptr, fmidi_err_eof);
    if (memcmp(fourcc, "CAT ", 4))
        RET_FAIL(nullptr, fmidi_err_format);

    uint32_t catsize;
    if ((ms = mb.readintBE(&catsize, 4)))
        RET_FAIL(nullptr, (fmidi_status)ms);
    if (mb.endpos() - mb.getpos() < catsize)
        RET_FAIL(nullptr, fmidi_err_eof);

    if (!(fourcc = mb.read(4)))
        RET_FAIL(nullptr, fmidi_err_eof);
    if (memcmp(fourcc, "XMID", 4))
        RET_FAIL(nullptr, fmidi_err_format);

    fmidi_smf_u smf(new fmidi_smf);
    smf->info.format = (ntracks > 1) ? 2 : 0;
    smf->info.track_count = ntracks;
    smf->info.delta_unit = 60;
    smf->track.reset(new fmidi_raw_track[ntracks]);

    for (uint32_t i = 0; i < ntracks; ++i) {
        if (!fmidi_xmi_read_track(mb, smf->track[i]))
            return nullptr;
        if (mb.getpos() & 1) {
            if ((ms = mb.skip(1)))
                RET_FAIL(nullptr, (fmidi_status)ms);
        }
    }

    uint32_t res = fmidi_xmi_update_unit(smf.get());
    if (res == 0)
        return nullptr;

    for (uint32_t i = 0; i < ntracks; ++i) {
        fmidi_track_iter_t it;
        fmidi_smf_track_begin(&it, i);
        fmidi_event_t *event;
        while ((event = const_cast<fmidi_event_t *>(
                    fmidi_smf_track_next(smf.get(), &it)))) {
            event->delta *= res;
        }
    }

    return smf.release();
}

fmidi_smf_t *fmidi_xmi_file_read(const char *filename)
{
    unique_FILE fh(fopen(filename, "rb"));
    if (!fh)
        RET_FAIL(nullptr, fmidi_err_input);

    fmidi_smf_t *smf = fmidi_xmi_stream_read(fh.get());
    return smf;
}

fmidi_smf_t *fmidi_xmi_stream_read(FILE *stream)
{
    struct stat st;
    size_t length;

    rewind(stream);

    if (fstat(fileno(stream), &st) != 0)
        RET_FAIL(nullptr, fmidi_err_input);

    length = st.st_size;
    if (length > fmidi_file_size_limit)
        RET_FAIL(nullptr, fmidi_err_largefile);

    bool pad = length & 1;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[length + pad]);
    if (!fread(buf.get(), length, 1, stream))
        RET_FAIL(nullptr, fmidi_err_input);
    if (pad)
        buf[length] = 0;

    fmidi_smf_t *smf = fmidi_xmi_mem_read(buf.get(), length + pad);
    return smf;
}
