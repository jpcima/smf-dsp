#pragma once
#include <memory>
class reverb_dsp;

class reverb {
public:
    reverb();
    ~reverb();

    void init(float sample_rate);
    void clear();
    void compute(float *ch1, float *ch2, unsigned nframes);

private:
    std::unique_ptr<reverb_dsp> dsp_;
    float *size_ = nullptr;
    float *amount_ = nullptr;
};
