//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <map>
#include <cstdint>

class Seek_State {
public:
    Seek_State();
    void clear();

    void add_event(const uint8_t *msg, uint32_t len);
    void add_reset_all_controllers(unsigned channel);

    void flush_state();

    typedef void (Message_Callback)(const uint8_t *msg, uint32_t len, void *cbdata);
    void set_message_callback(Message_Callback *cb, void *cbdata);

private:
    typedef std::map<uint32_t, uint32_t> storage_t;

    void emit_message(const uint8_t *msg, uint32_t len);

private:
    storage_t storage_;
    int is_nrpn_[16] = {};
    int rpn_msb_[16] = {};
    int rpn_lsb_[16] = {};
    bool reset_all_cc[16] = {};

    Message_Callback *cb_ = nullptr;
    void *cbdata_ = nullptr;
};
