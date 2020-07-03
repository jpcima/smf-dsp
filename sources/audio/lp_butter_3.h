#pragma once
#include <algorithm>
#include <cmath>

class lp_butter_3 {

private:

    float fConst0;
    float fVec0[2];
    float fRec1[2];
    float fRec0[3];
    float fControl[7];

    static float mydsp_faustpower2_f(float value)
    {
        return (value * value);
    }

public:

    void init(float sample_rate)
    {
        fConst0 = (3.14159274f / sample_rate);
        clear();
    }

    void cutoff(float fc)
    {
        fControl[0] = std::tan((fConst0 * float(fc)));
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
            fVec0[l0] = 0.0f;
        }
        for (int l1 = 0; (l1 < 2); l1 = (l1 + 1)) {
            fRec1[l1] = 0.0f;
        }
        for (int l2 = 0; (l2 < 3); l2 = (l2 + 1)) {
            fRec0[l2] = 0.0f;
        }
    }

    float compute(float input)
    {
        float fTemp0 = float(input);
        fVec0[0] = fTemp0;
        fRec1[0] = (0.0f - (fControl[3] * ((fControl[4] * fRec1[1]) - (fTemp0 + fVec0[1]))));
        fRec0[0] = (fRec1[0] - (fControl[2] * ((fControl[5] * fRec0[2]) + (fControl[6] * fRec0[1]))));
        float output = float((fControl[2] * (fRec0[2] + (fRec0[0] + (2.0f * fRec0[1])))));
        fVec0[1] = fVec0[0];
        fRec1[1] = fRec1[0];
        fRec0[2] = fRec0[1];
        fRec0[1] = fRec0[0];
        return output;
    }

};
