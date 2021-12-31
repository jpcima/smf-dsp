//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>

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
    struct Storage {
        void clear();
        void reserve(size_t capacity);
        void put(uint32_t raw_id, uint32_t value);
        uint32_t *find(uint32_t raw_id);
        const uint32_t *find(uint32_t raw_id) const;

        struct Assoc {
            uint32_t raw_id;
            uint32_t value;
        };

        std::vector<Assoc> assoc;
        std::unordered_map<uint32_t /*raw_id*/, size_t /*index*/> index_of;
    };

    void emit_message(const uint8_t *msg, uint32_t len);

private:
    Storage storage_;
    int is_nrpn_[16] = {};
    int rpn_msb_[16] = {};
    int rpn_lsb_[16] = {};
    bool reset_all_cc[16] = {};

    Message_Callback *cb_ = nullptr;
    void *cbdata_ = nullptr;
};
