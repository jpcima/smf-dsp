#pragma once
#include <xmmintrin.h>
#include <cmath>

class lp_1_sse {
public:
    void init(float sample_rate)
    {
        inv_sample_rate_ = 1.0f / sample_rate;
        clear();
    }

    void clear()
    {
        mem_ = _mm_set1_ps(0.0f);
    }

    void cutoff(__m128 fc)
    {
        for (int i = 0; i < 4; ++i)
            pole_[i] = std::exp(float(-2.0 * M_PI) * fc[i] * inv_sample_rate_);
    }

    void tau(__m128 t60)
    {
        for (int i = 0; i < 4; ++i)
            pole_[i] = std::exp(-inv_sample_rate_ / t60[i]);
    }

    __m128 compute(__m128 input)
    {
        mem_ = input * (1 - pole_) + mem_ * pole_;
        return mem_;
    }

private:
    __m128 mem_;
    __m128 pole_;
    float inv_sample_rate_;
};
