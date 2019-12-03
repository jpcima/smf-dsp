//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "sequencer.h"

extern Basic_Sequencer *create_sequencer_wholstand(fmidi_smf_t &smf);
extern Basic_Sequencer *create_sequencer_jpc(fmidi_smf_t &smf);

Basic_Sequencer *Basic_Sequencer::create_sequencer(Sequencer_Type type, fmidi_smf_t &smf)
{
    switch (type) {
    default:
    case Sequencer_Type::Model_W:
        return create_sequencer_wholstand(smf);
    case Sequencer_Type::Model_J:
        return create_sequencer_jpc(smf);
    }
}

void Basic_Sequencer::set_message_callback(Sequencer_Message_Callback *cb, void *ud)
{
    message_callback_ = cb;
    message_userdata_ = ud;
}

void Basic_Sequencer::set_meta_callback(Sequencer_Meta_Callback *cb, void *ud)
{
    meta_callback_ = cb;
    meta_userdata_ = ud;
}

void Basic_Sequencer::set_finished_callback(Sequencer_Finished_Callback *cb, void *ud)
{
    finished_callback_ = cb;
    finished_userdata_ = ud;
}

void Basic_Sequencer::receive_message(const Sequencer_Message_Event &event)
{
    if (message_callback_)
        message_callback_(event, message_userdata_);
}

void Basic_Sequencer::receive_meta(const Sequencer_Meta_Event &event)
{
    if (meta_callback_)
        meta_callback_(event, meta_userdata_);
}

void Basic_Sequencer::signal_finished()
{
    if (finished_callback_)
        finished_callback_(finished_userdata_);
}
