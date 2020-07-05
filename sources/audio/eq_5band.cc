#include "eq_5band.h"
#include "wrap/faust.h"
#include "gen/eq_5band.cxx"

Eq_5band::Eq_5band()
    : dsp_(new Eq_5band_dsp)
{
    Eq_5band_dsp &dsp = *dsp_;
    dsp.instanceResetUserInterface();

    UI ui;
    dsp.buildUserInterface(&ui);

    for (unsigned p = 0; p < parameter_.size(); ++p)
        parameter_[p] = ui.getActiveById(p);
}

Eq_5band::~Eq_5band()
{
}

void Eq_5band::init(float sample_rate)
{
    Eq_5band_dsp &dsp = *dsp_;
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void Eq_5band::clear()
{
    Eq_5band_dsp &dsp = *dsp_;
    dsp.instanceClear();
}

void Eq_5band::compute(float *ch1, float *ch2, unsigned nframes)
{
    Eq_5band_dsp &dsp = *dsp_;
    float *chs[] = {ch1, ch2};
    dsp.compute(static_cast<int>(nframes), chs, chs);
}

float Eq_5band::get_parameter(size_t index) const
{
    if (index >= parameter_.size())
        return 0;
    return *parameter_[index];
}

void Eq_5band::set_parameter(size_t index, float value)
{
    if (index >= parameter_.size())
        return;
    *parameter_[index] = value;
}
