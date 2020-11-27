#include "bass_enhance.h"
#include "wrap/faust.h"
#include "gen/bass_enhance.cxx"

BassEnhance::BassEnhance()
    : dsp_(new BassEnhance_dsp)
{
    BassEnhance_dsp &dsp = *dsp_;
    dsp.instanceResetUserInterface();

    UI ui;
    dsp.buildUserInterface(&ui);

    for (unsigned p = 0; p < parameter_.size(); ++p)
        parameter_[p] = ui.getActiveById(p);
}

BassEnhance::~BassEnhance()
{
}

void BassEnhance::init(float sample_rate)
{
    BassEnhance_dsp &dsp = *dsp_;
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void BassEnhance::clear()
{
    BassEnhance_dsp &dsp = *dsp_;
    dsp.instanceClear();
}

void BassEnhance::compute(float *ch1, float *ch2, unsigned nframes)
{
    BassEnhance_dsp &dsp = *dsp_;
    float *chs[] = {ch1, ch2};
    dsp.compute(static_cast<int>(nframes), chs, chs);
}

float BassEnhance::get_parameter(size_t index) const
{
    if (index >= parameter_.size())
        return 0;
    return *parameter_[index];
}

void BassEnhance::set_parameter(size_t index, float value)
{
    if (index >= parameter_.size())
        return;
    *parameter_[index] = value;
}
