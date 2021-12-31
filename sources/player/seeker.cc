//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "seeker.h"

enum Seek_Control_Type {
    Control_CC = 1,
    Control_RPN_MSB,
    Control_RPN_LSB,
    Control_NRPN_MSB,
    Control_NRPN_LSB,
    Control_BEND,
    Control_PROGRAM,
};

union Seek_Control_Id {
    uint32_t raw = 0;
    struct {
        uint32_t type : 3;
        uint32_t channel : 4;
        uint32_t identifier : 14;
    };

    uint8_t cc() const noexcept { return identifier & 127; }
    uint8_t rpn_lsb() const noexcept { return identifier & 127; }
    uint8_t rpn_msb() const noexcept { return (identifier >> 7) & 127; }

    void set_cc(uint8_t cc) noexcept { identifier = cc & 127; }
    void set_rpn(uint8_t msb, uint8_t lsb) noexcept { identifier = ((msb & 127) << 7) | (lsb & 127); }
};

static constexpr size_t initial_capacity = 256;

Seek_State::Seek_State()
{
    storage_.reserve(initial_capacity);
    clear();
}

void Seek_State::clear()
{
    storage_.clear();
    for (unsigned ch = 0; ch < 16; ++ch) {
        is_nrpn_[ch] = -1;
        rpn_msb_[ch] = -1;
        rpn_lsb_[ch] = -1;
        reset_all_cc[ch] = false;
    }
}

void Seek_State::add_event(const uint8_t *msg, uint32_t len)
{
    if (len == 0)
        return;

    // note: flush at any message which may alter control state behind scenes,
    // or may require some delay after it (reset, system exclusive)

    unsigned status = msg[0];

    if (status == 0xf0) { // system-exclusive
        flush_state();
        emit_message(msg, len);
        return;
    }

    unsigned channel = status & 15;
    unsigned data1 = (len >= 2) ? (msg[1] & 127) : 0;
    unsigned data2 = (len >= 3) ? (msg[2] & 127) : 0;

    if (status == 0xff) { // reset
        flush_state();
        emit_message(msg, len);
        return;
    }

    Storage &stor = storage_;

    switch (status & 0xf0) {
    case 0xb0: // controller change
        switch (data1) {
        case 121: // reset all controllers
            add_reset_all_controllers(channel);
            break;
        case 6: // data entry MSB
        case 38: { // data entry LSB
            int is_nrpn = is_nrpn_[channel];
            unsigned msb = rpn_msb_[channel];
            unsigned lsb = rpn_lsb_[channel];
            if ((int)msb == -1 || (int)lsb == -1 || is_nrpn == -1)
                break;
            Seek_Control_Id id;
            id.type = (data1 == 6) ?
                (is_nrpn ? Control_NRPN_MSB : Control_RPN_MSB) :
                (is_nrpn ? Control_NRPN_LSB : Control_RPN_LSB);
            id.channel = channel;
            id.set_rpn(msb, lsb);
            stor.put(id.raw, data2);
            break;
        }
        case 98: // NRPN LSB
        case 100: // RPN LSB
            is_nrpn_[channel] = data1 == 98;
            rpn_lsb_[channel] = data2;
            break;
        case 99: // NRPN MSB
        case 101: // RPN MSB
            is_nrpn_[channel] = data1 == 99;
            rpn_msb_[channel] = data2;
            break;
        default: { // other
            if (data1 >= 120) { // channel mode message, not ordinary controller
                flush_state();
                emit_message(msg, len);
                return;
            }
            Seek_Control_Id id;
            id.type = Control_CC;
            id.channel = channel;
            id.set_cc(data1);
            stor.put(id.raw, data2);
            break;
        }
        }
        break;
    case 0xc0: { // program change
        Seek_Control_Id id;
        id.type = Control_PROGRAM;
        id.channel = channel;
        stor.put(id.raw, data1);
        break;
    }
    case 0xe0: { // pitch bend change
        Seek_Control_Id id;
        id.type = Control_BEND;
        id.channel = channel;
        stor.put(id.raw, data1 | (data2 << 7));
        break;
    }
    }
}

void Seek_State::add_reset_all_controllers(unsigned channel)
{
    Storage &stor = storage_;
    Storage temp;
    temp.reserve(initial_capacity);

    std::swap(stor.assoc, temp.assoc);
    std::swap(stor.index_of, temp.index_of);

    stor.clear();

    for (Storage::Assoc item : temp.assoc) {
        Seek_Control_Id id;
        id.raw = item.raw_id;

        // remove all changes in the channel which are affected by
        // the reset-all-controllers CC
        bool removed = false;

        if (channel == id.channel) {
            switch (id.type) {
            default:
                removed = true;
                break;
            case Control_PROGRAM:
                removed = false;
                break;
            case Control_CC: {
                // cf. GM Level 1 developer guidelines
                uint8_t cc = id.cc();
                removed = !(
                    cc == 0 || cc == 32 || // bank select
                    cc == 7 || cc == 10 || // volume, pan
                    (cc >= 91 && cc <= 95) || // not GM
                    (cc >= 70 && cc <= 79) || // not GM
                    cc == 120 || cc >= 122); // other
                break;
            }
            }
        }

        if (!removed)
            stor.put(id.raw, item.value);
    }

    reset_all_cc[channel] = true;
    is_nrpn_[channel] = -1;
    rpn_msb_[channel] = -1;
    rpn_lsb_[channel] = -1;
}

void Seek_State::flush_state()
{
    const Storage &stor = storage_;

    ///
    auto emit_cc = [this](unsigned ch, unsigned cc, unsigned val) {
        uint8_t msg[3] = {(uint8_t)(0xb0|ch), (uint8_t)cc, (uint8_t)val};
        emit_message(msg, sizeof(msg));
    };
    auto emit_bend = [this](unsigned ch, unsigned bend) {
        uint8_t msg[3] = {(uint8_t)(0xe0|ch), (uint8_t)(bend & 127), (uint8_t)(bend >> 7)};
        emit_message(msg, sizeof(msg));
    };
    auto emit_program = [this](unsigned ch, unsigned pgm) {
        uint8_t msg[2] = {(uint8_t)(0xc0|ch), (uint8_t)pgm};
        emit_message(msg, sizeof(msg));
    };

    ///
    for (unsigned ch = 0; ch < 16; ++ch) {
        if (reset_all_cc[ch])
            emit_cc(ch, 121, 0);
    }

    ///
    for (Storage::Assoc item : stor.assoc) {
        Seek_Control_Id id;
        id.raw = item.raw_id;

        uint32_t value = item.value;

        switch (id.type) {
        case Control_CC: {
            emit_cc(id.channel, id.cc(), value);
            break;
        }

        // RPN/NRPN note: if value is a MSB/LSB pair, send both of them along
        // and avoid repeating 2 CC messages.

        case Control_RPN_MSB: {
            emit_cc(id.channel, 101, id.rpn_msb());
            emit_cc(id.channel, 100, id.rpn_lsb());
            emit_cc(id.channel, 6, value);
            Seek_Control_Id id_lsb = id;
            id_lsb.type = Control_RPN_LSB;
            if (const uint32_t *value_lsb = stor.find(id_lsb.raw))
                emit_cc(id.channel, 38, *value_lsb);
            break;
        }
        case Control_RPN_LSB: {
            Seek_Control_Id id_msb = id;
            id_msb.type = Control_RPN_MSB;
            if (!stor.find(id_msb.raw)) {
                emit_cc(id.channel, 101, id.rpn_msb());
                emit_cc(id.channel, 100, id.rpn_lsb());
                emit_cc(id.channel, 38, value);
            }
            break;
        }

        case Control_NRPN_MSB: {
            emit_cc(id.channel, 99, id.rpn_msb());
            emit_cc(id.channel, 98, id.rpn_lsb());
            emit_cc(id.channel, 6, value);
            Seek_Control_Id id_lsb = id;
            id_lsb.type = Control_NRPN_LSB;
            if (const uint32_t *value_lsb = stor.find(id_lsb.raw))
                emit_cc(id.channel, 38, *value_lsb);
            break;
        }
        case Control_NRPN_LSB: {
            Seek_Control_Id id_msb = id;
            id_msb.type = Control_NRPN_MSB;
            if (!stor.find(id_msb.raw)) {
                emit_cc(id.channel, 99, id.rpn_msb());
                emit_cc(id.channel, 98, id.rpn_lsb());
                emit_cc(id.channel, 38, value);
            }
            break;
        }

        case Control_BEND: {
            emit_bend(id.channel, value);
            break;
        }
        case Control_PROGRAM: {
            emit_program(id.channel, value);
            break;
        }
        }
    }

    for (unsigned ch = 0; ch < 16; ++ch) {
        int is_nrpn = is_nrpn_[ch];
        if (is_nrpn == -1)
            continue;

        unsigned msb = rpn_msb_[ch];
        if ((int)msb != -1)
            emit_cc(ch, is_nrpn ? 99 : 101, msb);

        unsigned lsb = rpn_lsb_[ch];
        if ((int)lsb != -1)
            emit_cc(ch, is_nrpn ? 98 : 100, lsb);
    }

    clear();
}

void Seek_State::set_message_callback(Message_Callback *cb, void *cbdata)
{
    cb_ = cb;
    cbdata_ = cbdata;
}

void Seek_State::emit_message(const uint8_t *msg, uint32_t len)
{
    Message_Callback *cb = cb_;
    if (cb)
        cb(msg, len, cbdata_);
}

///
void Seek_State::Storage::clear()
{
    assoc.clear();
    index_of.clear();
}

void Seek_State::Storage::reserve(size_t capacity)
{
    assoc.reserve(capacity);
    index_of.reserve(capacity);
}

void Seek_State::Storage::put(uint32_t raw_id, uint32_t value)
{
    Assoc *entry = nullptr;

    auto it = index_of.find(raw_id);
    if (it != index_of.end())
        entry = &assoc[it->second];
    else {
        size_t index = assoc.size();
        assoc.emplace_back();
        index_of[raw_id] = index;
        entry = &assoc[index];
        entry->raw_id = raw_id;
    }

    entry->value = value;
}

uint32_t *Seek_State::Storage::find(uint32_t raw_id)
{
    return const_cast<uint32_t *>(
        const_cast<const Storage *>(this)->find(raw_id));
}

const uint32_t *Seek_State::Storage::find(uint32_t raw_id) const
{
    auto it = index_of.find(raw_id);
    return (it != index_of.end()) ? &assoc[it->second].value : nullptr;
}
