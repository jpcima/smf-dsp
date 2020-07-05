#pragma once
#include <xmmintrin.h>
#include <algorithm>
#include <cmath>

class lp_butter_3_sse {

private:

    float fConst0;
    __m128 fVec0[2];
    __m128 fRec1[2];
    __m128 fRec0[3];
    __m128 fControl[7];

    static __m128 mydsp_faustpower2_f(__m128 value)
    {
        return (value * value);
    }

public:

    void init(float sample_rate)
    {
        fConst0 = (3.14159274f / sample_rate);
        clear();
    }

    void cutoff(__m128 fc)
    {
        for (int i = 0; i < 4; ++i)
            fControl[0][i] = std::tan((fConst0 * float(fc[i])));
        fControl[1] = (1.0f / fControl[0]);
        fControl[2] = (1.0f / (((fControl[1] + 1.0f) / fControl[0]) + 1.0f));
        fControl[3] = (1.0f / (fControl[1] + 1.0f));
        fControl[4] = (1.0f - fControl[1]);
        fControl[5] = (((fControl[1] + -1.0f) / fControl[0]) + 1.0f);
        fControl[6] = (2.0f * (1.0f - (1.0f / mydsp_faustpower2_f(fControl[0]))));
    }

    void clear()
    {
        for (int l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
            fVec0[l0] = _mm_set1_ps(0.0f);
        }
        for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
            fRec1[l1] = _mm_set1_ps(0.0f);
        }
        for (int l2 = 0; (l2 < 3); l2 = (l2 + 1)) {
            fRec0[l2] = _mm_set1_ps(0.0f);
        }
    }

    __m128 compute(__m128 input)
    {
        __m128 fTemp0 = __m128(input);
        fVec0[0] = fTemp0;
        fRec1[0] = (0.0f - (fControl[3] * ((fControl[4] * fRec1[1]) - (fTemp0 + fVec0[1]))));
        fRec0[0] = (fRec1[0] - (fControl[2] * ((fControl[5] * fRec0[2]) + (fControl[6] * fRec0[1]))));
        __m128 output = __m128((fControl[2] * (fRec0[2] + (fRec0[0] + (2.0f * fRec0[1])))));
        fVec0[1] = fVec0[0];
        fRec1[1] = fRec1[0];
        fRec0[2] = fRec0[1];
        fRec0[1] = fRec0[0];
        return output;
    }

};
