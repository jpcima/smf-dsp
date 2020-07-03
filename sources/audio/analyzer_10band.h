#pragma once
#include "lp_butter_3.h"
#include "hp_butter_3.h"
#include "lp_1.h"
#if defined(__SSE__)
#include "lp_butter_3_sse.h"
#include "hp_butter_3_sse.h"
#include "lp_1_sse.h"
#endif

class analyzer_10band {
public:
    void init(float sample_rate);
    void setup(float M, float ftop, float t60); // M=1, ftop=10e3, t60=100e-3
    void clear();
    float *compute(const float inputs[], size_t count);

private:
    enum { N = 10 };

private:
    lp_butter_3 f_lo_band_;
    lp_1 f_lo_smooth_;
#if defined(__SSE__)
    struct bandpass_filter_sse { lp_butter_3_sse lp, hp; };
    bandpass_filter_sse f_mid_band_sse_[(N - 2) / 4];
    lp_1_sse f_mid_smooth_sse_[(N - 2) / 4];
#else
    struct bandpass_filter { lp_butter_3 lp, hp; };
    bandpass_filter f_mid_band_[N - 2];
    lp_1 f_mid_smooth_[N - 2];
#endif
    hp_butter_3 f_hi_band_;
    lp_1 f_hi_smooth_;
    float outputs_[N] {};
};
