//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>

typedef struct fmidi_smf fmidi_smf_t;

enum class Sequencer_Type {
    Model_W = 'w', // ADLMIDI sequencer by Wohlstand and Bisqwit
    Model_J = 'j', // FMIDI sequencer by JPC
};

struct Sequencer_Message_Event {
    const uint8_t *data = nullptr;
    uint32_t length = 0;
};

struct Sequencer_Meta_Event {
    uint8_t type;
    const uint8_t *data = nullptr;
    uint32_t length = 0;
};

typedef void (Sequencer_Message_Callback)(const Sequencer_Message_Event &, void *);
typedef void (Sequencer_Meta_Callback)(const Sequencer_Meta_Event &, void *);
typedef void (Sequencer_Finished_Callback)(void *);

class Basic_Sequencer {
public:
    static Basic_Sequencer *create_sequencer(Sequencer_Type type, fmidi_smf_t &smf);

    virtual ~Basic_Sequencer() {}

    virtual Sequencer_Type type() = 0;

    virtual double get_current_speed() = 0;
    virtual void set_speed(double speed) = 0;
    virtual double get_current_time() const = 0;
    virtual void goto_time(double time) = 0;
    virtual void tick(double delta) = 0;
    virtual void rewind() = 0;
    virtual void set_loop_enabled(bool loop) = 0;

    void set_message_callback(Sequencer_Message_Callback *cb, void *ud);
    void set_meta_callback(Sequencer_Meta_Callback *cb, void *ud);
    void set_finished_callback(Sequencer_Finished_Callback *cb, void *ud);

protected:
    void receive_message(const Sequencer_Message_Event &event);
    void receive_meta(const Sequencer_Meta_Event &event);
    void signal_finished();

private:
    Sequencer_Message_Callback *message_callback_ = nullptr;
    void *message_userdata_ = nullptr;
    Sequencer_Meta_Callback *meta_callback_ = nullptr;
    void *meta_userdata_ = nullptr;
    Sequencer_Finished_Callback *finished_callback_ = nullptr;
    void *finished_userdata_ = nullptr;
};
