#pragma once
#include <cmath>

class lp_1 {
public:
    void init(float sample_rate)
    {
        inv_sample_rate_ = 1.0f / sample_rate;
        clear();
    }

    void clear()
    {
        mem_ = 0.0f;
    }

    void cutoff(float fc)
    {
        pole_ = std::exp(float(-2.0 * M_PI) * fc * inv_sample_rate_);
    }

    void tau(float t60)
    {
        pole_ = std::exp(-inv_sample_rate_ / t60);
    }

    float compute(float input)
    {
        mem_ = input * (1 - pole_) + mem_ * pole_;
        return mem_;
    }

private:
    float mem_;
    float pole_;
    float inv_sample_rate_;
};
