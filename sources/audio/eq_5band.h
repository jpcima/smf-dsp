#pragma once
#include <memory>
class eq_5band_dsp;

class eq_5band {
public:
    eq_5band();
    ~eq_5band();

    void init(float sample_rate);
    void clear();
    void compute(float *ch1, float *ch2, unsigned nframes);

private:
    std::unique_ptr<eq_5band_dsp> dsp_;
    float *band_[5] = {};
};
