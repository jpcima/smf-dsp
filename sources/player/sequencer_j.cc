//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "sequencer.h"
#include <fmidi/fmidi.h>

class Sequencer_J final : public Basic_Sequencer {
public:
    Sequencer_J(fmidi_smf_t &smf);

    Sequencer_Type type() override { return Sequencer_Type::Model_J; }

    double get_current_speed() override;
    void set_speed(double speed) override;
    double get_current_time() const override;
    void goto_time(double time) override;
    void tick(double delta) override;
    void rewind() override;
    void set_loop_enabled(bool loop) override;

private:
    static void on_event(const fmidi_event_t *event, void *userdata);
    static void on_finished(void *userdata);

private:
    fmidi_player_u pl_;
};

Sequencer_J::Sequencer_J(fmidi_smf_t &smf)
    : pl_(fmidi_player_new(&smf))
{
    fmidi_player_t *pl = pl_.get();
    fmidi_player_event_callback(pl, &on_event, this);
    fmidi_player_finish_callback(pl, &on_finished, this);
}

double Sequencer_J::get_current_speed()
{
    fmidi_player_t *pl = pl_.get();
    return fmidi_player_current_speed(pl);
}

void Sequencer_J::set_speed(double speed)
{
    fmidi_player_t *pl = pl_.get();
    fmidi_player_set_speed(pl, speed);
}

double Sequencer_J::get_current_time() const
{
    fmidi_player_t *pl = pl_.get();
    return fmidi_player_current_time(pl);
}

void Sequencer_J::goto_time(double time)
{
    fmidi_player_t *pl = pl_.get();
    return fmidi_player_goto_time(pl, time);
}

void Sequencer_J::tick(double delta)
{
    fmidi_player_t *pl = pl_.get();
    return fmidi_player_tick(pl, delta);
}

void Sequencer_J::rewind()
{
    fmidi_player_t *pl = pl_.get();
    return fmidi_player_rewind(pl);
}

void Sequencer_J::set_loop_enabled(bool loop)
{
    // not supported
}

void Sequencer_J::on_event(const fmidi_event_t *event, void *userdata)
{
    Sequencer_J *self = reinterpret_cast<Sequencer_J *>(userdata);

    switch (event->type) {
    case fmidi_event_message: {
        Sequencer_Message_Event m;
        m.data = event->data;
        m.length = event->datalen;
        self->receive_message(m);
        break;
    }
    case fmidi_event_meta: {
        Sequencer_Meta_Event m;
        m.type = event->data[0];
        m.data = event->data + 1;
        m.length = event->datalen - 1;
        self->receive_meta(m);
        break;
    }
    default:
        break;
    }
}

void Sequencer_J::on_finished(void *userdata)
{
    Sequencer_J *self = reinterpret_cast<Sequencer_J *>(userdata);
    self->signal_finished();
}

///
Basic_Sequencer *create_sequencer_jpc(fmidi_smf_t &smf)
{
    return new Sequencer_J(smf);
}
