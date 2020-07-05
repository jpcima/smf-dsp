#pragma once
#include <array>
#include <memory>
class Eq_5band_dsp;

class Eq_5band {
public:
    Eq_5band();
    ~Eq_5band();

    void init(float sample_rate);
    void clear();
    void compute(float *ch1, float *ch2, unsigned nframes);

    float get_parameter(size_t index) const;
    void set_parameter(size_t index, float value);

private:
    std::unique_ptr<Eq_5band_dsp> dsp_;
    std::array<float *, 5> parameter_ {};
};
