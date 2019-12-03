//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "sequencer.h"
#include <midi_sequencer_impl.hpp>
#include <fmidi/fmidi.h>
#include <gsl.hpp>

class Sequencer_W final : public Basic_Sequencer {
public:
    Sequencer_W(fmidi_smf_t &smf);

    Sequencer_Type type() override { return Sequencer_Type::Model_W; }

    double get_current_speed() override;
    void set_speed(double speed) override;
    double get_current_time() const override;
    void goto_time(double time) override;
    void tick(double delta) override;
    void rewind() override;

private:
    std::unique_ptr<BW_MidiSequencer> seq_;
    BW_MidiRtInterface intf_ = {};
    double speed_ = 1.0;
};

///
static constexpr double tick_granularity = 1e-3;

///
Sequencer_W::Sequencer_W(fmidi_smf_t &smf)
    : seq_(new BW_MidiSequencer)
{
    BW_MidiSequencer &seq = *seq_;
    BW_MidiRtInterface &intf = intf_;

    // fill the callback interface
    intf.rtUserData = this;

    intf.rt_noteOn = +[](void *ud, uint8_t ch, uint8_t nn, uint8_t vl)
    {
        uint8_t msg[] = { (uint8_t)(0x90|ch), nn, vl };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_noteOffVel = +[](void *ud, uint8_t ch, uint8_t nn, uint8_t vl)
    {
        uint8_t msg[] = { (uint8_t)(0x80|ch), nn, vl };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_noteAfterTouch = +[](void *ud, uint8_t ch, uint8_t nn, uint8_t vl)
    {
        uint8_t msg[] = { (uint8_t)(0xa0|ch), nn, vl };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_channelAfterTouch = +[](void *ud, uint8_t ch, uint8_t vl)
    {
        uint8_t msg[] = { (uint8_t)(0xd0|ch), vl };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_controllerChange = +[](void *ud, uint8_t ch, uint8_t ty, uint8_t vl)
    {
        uint8_t msg[] = { (uint8_t)(0xb0|ch), ty, vl };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_patchChange = +[](void *ud, uint8_t ch, uint8_t pn)
    {
        uint8_t msg[] = { (uint8_t)(0xc0|ch), pn };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_pitchBend = +[](void *ud, uint8_t ch, uint8_t msb, uint8_t lsb)
    {
        uint8_t msg[] = { (uint8_t)(0xe0|ch), lsb, msb };
        Sequencer_Message_Event ev;
        ev.data = msg;
        ev.length = (uint32_t)sizeof(msg);
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_systemExclusive = +[](void *ud, const uint8_t *sdata, size_t slength)
    {
        Sequencer_Message_Event ev;
        ev.data = sdata;
        ev.length = (uint32_t)slength;
        return reinterpret_cast<Sequencer_W *>(ud)->receive_message(ev);
    };
    intf.rt_metaEvent = +[](void *ud, uint8_t type, const uint8_t *mdata, size_t mlength)
    {
        Sequencer_Meta_Event ev;
        ev.type = type;
        ev.data = mdata;
        ev.length = (uint32_t)mlength;
        return reinterpret_cast<Sequencer_W *>(ud)->receive_meta(ev);
    };

    seq.setInterface(&intf);

    // output cleaned up SMF to feed into the BW sequencer
    uint8_t *smf_data;
    size_t smf_length;
    if (fmidi_smf_mem_write(&smf, &smf_data, &smf_length)) {
        auto cleanup = gsl::finally([smf_data]() { free(smf_data); });
        seq.loadMIDI(smf_data, smf_length);
    }
}

double Sequencer_W::get_current_speed()
{
    return speed_;
}

void Sequencer_W::set_speed(double speed)
{
    speed_ = speed;
}

double Sequencer_W::get_current_time() const
{
    BW_MidiSequencer &seq = *seq_;
    return seq.tell();
}

void Sequencer_W::goto_time(double time)
{
    BW_MidiSequencer &seq = *seq_;
    seq.seek(time, tick_granularity);
}

void Sequencer_W::tick(double delta)
{
    BW_MidiSequencer &seq = *seq_;
    double speed = speed_;
    seq.Tick(speed * delta, tick_granularity);
    if (seq.positionAtEnd())
        signal_finished();
}

void Sequencer_W::rewind()
{
    BW_MidiSequencer &seq = *seq_;
    seq.rewind();
}

///
Basic_Sequencer *create_sequencer_wholstand(fmidi_smf_t &smf)
{
    return new Sequencer_W(smf);
}
