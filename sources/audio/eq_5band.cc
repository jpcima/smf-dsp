#include "eq_5band.h"
#include "wrap/faust.h"
#include "gen/eq_5band.cxx"

eq_5band::eq_5band()
    : dsp_(new eq_5band_dsp)
{
    eq_5band_dsp &dsp = *dsp_;
    dsp.instanceResetUserInterface();

    UI ui;
    dsp.buildUserInterface(&ui);

    for (unsigned i = 0; i < 5; ++i)
        band_[i] = ui.getActiveById(i);
}

eq_5band::~eq_5band()
{
}

void eq_5band::init(float sample_rate)
{
    eq_5band_dsp &dsp = *dsp_;
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void eq_5band::clear()
{
    eq_5band_dsp &dsp = *dsp_;
    dsp.instanceClear();
}

void eq_5band::compute(float *ch1, float *ch2, unsigned nframes)
{
    eq_5band_dsp &dsp = *dsp_;
    float *chs[] = {ch1, ch2};
    dsp.compute(static_cast<int>(nframes), chs, chs);
}
