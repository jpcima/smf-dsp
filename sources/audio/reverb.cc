#include "reverb.h"
#include "wrap/faust.h"
#include "gen/reverb.cxx"

Reverb::Reverb()
    : dsp_(new Reverb_dsp)
{
    Reverb_dsp &dsp = *dsp_;
    dsp.instanceResetUserInterface();

    UI ui;
    dsp.buildUserInterface(&ui);

    for (unsigned p = 0; p < parameter_.size(); ++p)
        parameter_[p] = ui.getActiveById(p);
}

Reverb::~Reverb()
{
}

void Reverb::init(float sample_rate)
{
    Reverb_dsp &dsp = *dsp_;
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void Reverb::clear()
{
    Reverb_dsp &dsp = *dsp_;
    dsp.instanceClear();
}

void Reverb::compute(float *ch1, float *ch2, unsigned nframes)
{
    Reverb_dsp &dsp = *dsp_;
    float *chs[] = {ch1, ch2};
    dsp.compute(static_cast<int>(nframes), chs, chs);
}

float Reverb::get_parameter(size_t index) const
{
    if (index >= parameter_.size())
        return 0;
    return *parameter_[index];
}

void Reverb::set_parameter(size_t index, float value)
{
    if (index >= parameter_.size())
        return;
    *parameter_[index] = value;
}
