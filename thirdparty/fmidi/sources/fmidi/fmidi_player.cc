//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi.h"
#include <memory>
#include <algorithm>
#include <assert.h>

struct fmidi_player_context {
    fmidi_player_t *plr;
    fmidi_seq_u seq;
    double timepos;
    double speed;
    bool have_event;
    fmidi_seq_event_t sqevt;
    void (*cbfn)(const fmidi_event_t *, void *);
    void *cbdata;
    void (*finifn)(void *);
    void *finidata;
};

struct fmidi_player {
    bool running;
    fmidi_player_context ctx;
};

fmidi_player_t *fmidi_player_new(fmidi_smf_t *smf)
{
    fmidi_player_u plr(new fmidi_player_t);
    plr->running = false;

    fmidi_player_context &ctx = plr->ctx;
    ctx.plr = plr.get();
    ctx.seq.reset(fmidi_seq_new(smf));
    ctx.timepos = 0;
    ctx.speed = 1;
    ctx.have_event = false;
    ctx.cbfn = nullptr;
    ctx.cbdata = nullptr;
    ctx.finifn = nullptr;
    ctx.finidata = nullptr;

    return plr.release();
}

void fmidi_player_tick(fmidi_player_t *plr, double delta)
{
    fmidi_player_context &ctx = plr->ctx;
    fmidi_seq_t &seq = *ctx.seq;
    void (*cbfn)(const fmidi_event_t *, void *) = ctx.cbfn;
    void *cbdata = ctx.cbdata;

    double timepos = ctx.timepos;
    bool have_event = ctx.have_event;
    fmidi_seq_event_t &sqevt = ctx.sqevt;

    timepos += ctx.speed * delta;

    bool more = have_event || fmidi_seq_next_event(&seq, &sqevt);
    if (more) {
        have_event = true;
        while (more && timepos > sqevt.time) {
            const fmidi_event_t &event = *sqevt.event;
            if (cbfn)
                cbfn(&event, cbdata);
            have_event = more = fmidi_seq_next_event(&seq, &sqevt);
        }
    }

    ctx.have_event = have_event;
    ctx.timepos = timepos;

    if (!more) {
        plr->running = false;
        if (ctx.finifn)
            ctx.finifn(ctx.finidata);
    }
}

void fmidi_player_free(fmidi_player_t *plr)
{
    delete plr;
}

void fmidi_player_start(fmidi_player_t *plr)
{
    plr->running = true;
}

void fmidi_player_stop(fmidi_player_t *plr)
{
    plr->running = false;
}

void fmidi_player_rewind(fmidi_player_t *plr)
{
    fmidi_player_context &ctx = plr->ctx;
    fmidi_seq_rewind(ctx.seq.get());
    ctx.timepos = 0;
    ctx.have_event = false;
}

bool fmidi_player_running(const fmidi_player_t *plr)
{
    return plr->running;
}

double fmidi_player_current_time(const fmidi_player_t *plr)
{
    return plr->ctx.timepos;
}

void fmidi_player_goto_time(fmidi_player_t *plr, double time)
{
    fmidi_player_context &ctx = plr->ctx;
    fmidi_seq_t &seq = *ctx.seq;

    fmidi_player_rewind(plr);

    if (ctx.cbfn) {
        alignas(fmidi_event_t) uint8_t evtbuf[fmidi_event_sizeof(3)];
        fmidi_event_t *evt = (fmidi_event_t *)evtbuf;
        evt->type = fmidi_event_message;
        evt->delta = 0;
        for (unsigned c = 0; c < 16; ++c) {
            // all sound off
            evt->datalen = 3;
            evt->data[0] = (0b1011 << 4) | c;
            evt->data[1] = 120;
            evt->data[2] = 0;
            ctx.cbfn(evt, ctx.cbdata);
            // reset all controllers
            evt->datalen = 3;
            evt->data[0] = (0b1011 << 4) | c;
            evt->data[1] = 121;
            evt->data[2] = 0;
            ctx.cbfn(evt, ctx.cbdata);
            // volume
            evt->datalen = 3;
            evt->data[0] = (0b1011 << 4) | c;
            evt->data[1] = 7;
            evt->data[2] = 100;
            ctx.cbfn(evt, ctx.cbdata);
            // pan
            evt->datalen = 3;
            evt->data[0] = (0b1011 << 4) | c;
            evt->data[1] = 10;
            evt->data[2] = 64;
            ctx.cbfn(evt, ctx.cbdata);
            // bank select
            evt->datalen = 3;
            evt->data[0] = (0b1011 << 4) | c;
            evt->data[1] = 0;
            evt->data[2] = 0;
            ctx.cbfn(evt, ctx.cbdata);
            evt->data[1] = 32;
            ctx.cbfn(evt, ctx.cbdata);
            // program change
            evt->datalen = 2;
            evt->data[0] = (0b1100 << 4) | c;
            evt->data[1] = 0;
            ctx.cbfn(evt, ctx.cbdata);
        }
    }

    for (fmidi_seq_event_t sqevt;
         fmidi_seq_peek_event(&seq, &sqevt) && sqevt.time < time;) {
        const fmidi_event_t &evt = *sqevt.event;
        if (ctx.cbfn)
            ctx.cbfn(&evt, ctx.cbdata);
        fmidi_seq_next_event(&seq, nullptr);
    }

    ctx.timepos = time;
}

double fmidi_player_current_speed(const fmidi_player_t *plr)
{
    return plr->ctx.speed;
}

void fmidi_player_set_speed(fmidi_player_t *plr, double speed)
{
    plr->ctx.speed = speed;
}

void fmidi_player_event_callback(
    fmidi_player_t *plr, void (*cbfn)(const fmidi_event_t *, void *), void *cbdata)
{
    fmidi_player_context &ctx = plr->ctx;
    ctx.cbfn = cbfn;
    ctx.cbdata = cbdata;
}

void fmidi_player_finish_callback(
    fmidi_player_t *plr, void (*cbfn)(void *), void *cbdata)
{
    fmidi_player_context &ctx = plr->ctx;
    ctx.finifn = cbfn;
    ctx.finidata = cbdata;
}
