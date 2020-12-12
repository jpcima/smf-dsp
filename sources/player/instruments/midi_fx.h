#pragma once
#include <random>
#include <cstdint>
#include <cstddef>

class Midi_Fx {
public:
    typedef void (output_callback)(void *cbdata, const uint8_t *msg, uint32_t len, double ts, int flags);

    void init(output_callback *cb, void *cbdata);
    void send_message(const uint8_t *msg, uint32_t len, double ts, int flags);

    int get_parameter(size_t index) const;
    void set_parameter(size_t index, int value);

private:
    output_callback *cb_ = nullptr;
    void *cbdata_ = nullptr;

    bool hum_enable_ = false;
    int hum_velocity_ = 0;

    typedef std::minstd_rand Prng;
    Prng prng_;
};
