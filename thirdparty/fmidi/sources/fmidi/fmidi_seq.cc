//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi.h"
#include <memory>
#include <string.h>

struct fmidi_seq_timing {
    fmidi_smpte startoffset;
    uint32_t tempo;
};

struct fmidi_seq_pending_event {
    const fmidi_event_t *event;
    double delta;
};

struct fmidi_seq_track_info {
    double timepos;
    fmidi_track_iter_t iter;
    fmidi_seq_pending_event next;
    std::shared_ptr<fmidi_seq_timing> timing;
};

struct fmidi_seq {
    const fmidi_smf_t *smf;
    std::unique_ptr<fmidi_seq_track_info[]> track;
};

static const double fmidi_convert_delta(
    const fmidi_seq_t *seq, uint16_t trkno, double delta)
{
    uint16_t unit = fmidi_smf_get_info(seq->smf)->delta_unit;
    uint32_t tempo = seq->track[trkno].timing->tempo;
    return fmidi_delta_time(delta, unit, tempo);
}

fmidi_seq_t *fmidi_seq_new(const fmidi_smf_t *smf)
{
    std::unique_ptr<fmidi_seq_t> seq(new fmidi_seq_t);
    seq->smf = smf;

    const fmidi_smf_info_t *info = fmidi_smf_get_info(smf);
    uint16_t format = info->format;
    uint16_t ntracks = info->track_count;

    seq->track.reset(new fmidi_seq_track_info[ntracks]);

    for (unsigned i = 0; i < ntracks; ++i) {
        fmidi_seq_track_info &track = seq->track[i];

        std::shared_ptr<fmidi_seq_timing> timing;
        if (format == 2 || i == 0)
            timing.reset(new fmidi_seq_timing);
        else
            timing = seq->track[0].timing;
        track.timing = timing;
    }

    fmidi_seq_rewind(seq.get());
    return seq.release();
}

void fmidi_seq_free(fmidi_seq_t *seq)
{
    delete seq;
}

void fmidi_seq_rewind(fmidi_seq_t *seq)
{
    const fmidi_smf_t *smf = seq->smf;
    const fmidi_smf_info_t *info = fmidi_smf_get_info(smf);
    uint16_t ntracks = info->track_count;
    bool independent_multi_track =
        ntracks > 1 && seq->track[0].timing != seq->track[1].timing;

    for (unsigned i = 0; i < ntracks; ++i) {
        fmidi_seq_track_info &track = seq->track[i];
        std::shared_ptr<fmidi_seq_timing> timing = track.timing;
        fmidi_smpte &startoffset = timing->startoffset;
        fmidi_smf_track_begin(&track.iter, i);
        track.next.event = nullptr;
        memset(startoffset.code, 0, 5);
        timing->tempo = 500000;
        track.timepos = fmidi_smpte_time(&startoffset);
    }

    for (unsigned i = 0; i < ntracks; ++i) {
        fmidi_seq_track_info &track = seq->track[i];
        std::shared_ptr<fmidi_seq_timing> timing = track.timing;
        fmidi_smpte &startoffset = timing->startoffset;

        const fmidi_event_t *evt;
        fmidi_track_iter_t it;
        fmidi_smf_track_begin(&it, i);
        while ((evt = fmidi_smf_track_next(smf, &it)) &&
               evt->delta == 0 && evt->type == fmidi_event_meta) {
            uint8_t id = evt->data[0];
            if (id == 0x54 && evt->datalen == 6) {  // SMPTE offset
                // disregard SMPTE offset for format 1 MIDI and similar
                if (independent_multi_track)
                    memcpy(startoffset.code, &evt->data[1], 5);
            }
            if (id == 0x51 && evt->datalen == 4) {  // set tempo
                const uint8_t *d24 = &evt->data[1];
                timing->tempo = (d24[0] << 16) | (d24[1] << 8) | d24[2];
            }
        }
        track.timepos = fmidi_smpte_time(&startoffset);
    }
}

static fmidi_seq_pending_event *fmidi_seq_track_current_event(
    fmidi_seq_t *seq, uint16_t trkno)
{
    const fmidi_smf_t *smf = seq->smf;
    fmidi_seq_track_info &track = seq->track[trkno];

    fmidi_seq_pending_event *pending;
    if (track.next.event)
        return &track.next;

    const fmidi_event_t *evt = fmidi_smf_track_next(smf, &track.iter);
    if (!evt)
        return nullptr;

    if (evt->type == fmidi_event_meta) {
        uint8_t tag = evt->data[0];
        if (tag == 0x2f || tag == 0x3f)  // end of track
            return nullptr;  // stop now even if the final event has delta
    }

    pending = &track.next;
    pending->event = evt;
    pending->delta = evt->delta;
    return pending;
}

static int fmidi_seq_next_track(fmidi_seq_t *seq)
{
    const fmidi_smf_info_t *info = fmidi_smf_get_info(seq->smf);
    unsigned ntracks = info->track_count;

    unsigned trkno = 0;
    fmidi_seq_pending_event *pevt;

    pevt = fmidi_seq_track_current_event(seq, 0);
    while (!pevt && ++trkno < ntracks)
        pevt = fmidi_seq_track_current_event(seq, trkno);

    if (!pevt)
        return -1;

    double nearest = fmidi_convert_delta(seq, trkno, pevt->delta) +
        seq->track[trkno].timepos;
    for (unsigned i = trkno + 1; i < ntracks; ++i) {
        if ((pevt = fmidi_seq_track_current_event(seq, i))) {
            double time = fmidi_convert_delta(seq, i, pevt->delta) +
                seq->track[i].timepos;
            if (time < nearest) {
                trkno = i;
                nearest = time;
            }
        }
    }

    return trkno;
}

bool fmidi_seq_peek_event(fmidi_seq_t *seq, fmidi_seq_event_t *sqevt)
{
    unsigned trkno = fmidi_seq_next_track(seq);
    if ((int)trkno == -1)
        return false;

    fmidi_seq_track_info &nexttrk = seq->track[trkno];

    const fmidi_seq_pending_event *pevt =
        fmidi_seq_track_current_event(seq, trkno);
    if (!pevt)
        return false;

    if (sqevt) {
        sqevt->time = fmidi_convert_delta(seq, trkno, pevt->delta) + nexttrk.timepos;
        sqevt->track = trkno;
        sqevt->event = pevt->event;
    }

    return true;
}

static void fmidi_seq_track_advance_by(
    fmidi_seq_t *seq, unsigned trkno, double time)
{
    const fmidi_smf_t *smf = seq->smf;
    const fmidi_smf_info_t *info = fmidi_smf_get_info(smf);
    uint16_t unit = info->delta_unit;
    fmidi_seq_track_info &trk = seq->track[trkno];
    fmidi_seq_timing &tim = *trk.timing;

    fmidi_seq_pending_event *evt = fmidi_seq_track_current_event(seq, trkno);
    if (evt)
        evt->delta -= fmidi_time_delta(time, unit, tim.tempo);
    trk.timepos += time;
}

bool fmidi_seq_next_event(fmidi_seq_t *seq, fmidi_seq_event_t *sqevt)
{
    fmidi_seq_event_t pltmp;
    sqevt = sqevt ? sqevt : &pltmp;

    if (!fmidi_seq_peek_event(seq, sqevt))
        return false;

    double time = sqevt->time;
    unsigned trkno = sqevt->track;
    const fmidi_event_t *evt = sqevt->event;
    fmidi_seq_track_info &trk = seq->track[trkno];

    const fmidi_smf_t *smf = seq->smf;
    const fmidi_smf_info_t *info = fmidi_smf_get_info(smf);
    unsigned ntracks = info->track_count;
    double elapsed = time - trk.timepos;

    for (unsigned i = 0; i < ntracks; ++i)
        if (i != trkno)
            fmidi_seq_track_advance_by(seq, i, elapsed);

    if (evt->type == fmidi_event_meta) {
        if (evt->data[0] == 0x51 && evt->datalen == 4) {  // set tempo
            const uint8_t *d24 = &evt->data[1];
            trk.timing->tempo = (d24[0] << 16) | (d24[1] << 8) | d24[2];
        }
    }

    trk.timepos = time;
    trk.next.event = nullptr;
    return true;
}
