#include "midi_fx.h"
#include "../fx.h"
#include "utility/logs.h"

void Midi_Fx::init(output_callback *cb, void *cbdata)
{
    cb_ = cb;
    cbdata_ = cbdata;
}

void Midi_Fx::send_message(const uint8_t *msg, uint32_t len, double ts, int flags)
{
    if (!cb_)
        return;

    uint8_t hum_buf[3];
    if (hum_enable_) {
        if (len >= 3 && (msg[0] & 0xf0) == 0x90 && msg[2] > 0) {
            int dev = hum_velocity_ * 32 / 100;
            int diff = std::uniform_int_distribution<>(-dev, +dev)(prng_) +
                std::uniform_int_distribution<>(-dev, +dev)(prng_);
            int old_vel = msg[2];
            int new_vel = std::max(1, std::min(127, old_vel + diff));
            //Log::i("Velocity %02x -> %02x (%+d)", old_vel, new_vel, new_vel - old_vel);
            hum_buf[0] = msg[0];
            hum_buf[1] = msg[1];
            hum_buf[2] = (uint8_t)new_vel;
            msg = hum_buf;
        }
    }

    cb_(cbdata_, msg, len, ts, flags);
}

int Midi_Fx::get_parameter(size_t index) const
{
    switch (index) {
    case Fx::P_Humanize:
        return hum_enable_ ? 100 : 0;
    case Fx::P_Humanize_Velocity:
        return hum_velocity_;
    }
    return 0;
}

void Midi_Fx::set_parameter(size_t index, int value)
{
    switch (index) {
    case Fx::P_Humanize:
        hum_enable_ = value > 0;
        break;
    case Fx::P_Humanize_Velocity:
        hum_velocity_ = value;
        break;
    }
}
