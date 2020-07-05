#include "reverb.h"
#include "wrap/faust.h"
#include "gen/reverb.cxx"

reverb::reverb()
    : dsp_(new reverb_dsp)
{
    reverb_dsp &dsp = *dsp_;
    dsp.instanceResetUserInterface();

    UI ui;
    dsp.buildUserInterface(&ui);

    size_ = ui.getActiveById(0);
    amount_ = ui.getActiveById(1);
}

reverb::~reverb()
{
}

void reverb::init(float sample_rate)
{
    reverb_dsp &dsp = *dsp_;
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void reverb::clear()
{
    reverb_dsp &dsp = *dsp_;
    dsp.instanceClear();
}

void reverb::compute(float *ch1, float *ch2, unsigned nframes)
{
    reverb_dsp &dsp = *dsp_;
    float *chs[] = {ch1, ch2};
    dsp.compute(static_cast<int>(nframes), chs, chs);
}
