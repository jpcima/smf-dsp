#include "synth_fx.h"
#include "audio/bass_enhance.h"
#include "audio/eq_5band.h"
#include "audio/reverb.h"

static const Fx_Parameter fx_parameters[] = {
    {"Bass Enhance", 0, Fx_Parameter::Boolean},
    {"Bass Amount", 50, Fx_Parameter::Integer},
    {"Bass Tone", 50, Fx_Parameter::Integer},
    {"EQ", 0, Fx_Parameter::Boolean, Fx_Parameter::HasSeparator},
    {"EQ Low", 50, Fx_Parameter::Integer},
    {"EQ Mid-Low", 50, Fx_Parameter::Integer},
    {"EQ Mid", 50, Fx_Parameter::Integer},
    {"EQ Mid-High", 50, Fx_Parameter::Integer},
    {"EQ High", 50, Fx_Parameter::Integer},
    {"Reverb", 0, Fx_Parameter::Boolean, Fx_Parameter::HasSeparator},
    {"Reverb amount", 50, Fx_Parameter::Integer},
    {"Reverb size", 10, Fx_Parameter::Integer},
};

static constexpr size_t fx_parameter_count =
    sizeof(fx_parameters) / sizeof(fx_parameters[0]);

static_assert(
    fx_parameter_count == Synth_Fx::Parameter_Count,
    "The parameter count does not match");

Synth_Fx::Synth_Fx()
    : be_(new BassEnhance), eq_(new Eq_5band), rev_(new Reverb)
{
    gsl::span<const Fx_Parameter> params(fx_parameters);

    for (size_t p = 0; p < params.size(); ++p)
        set_parameter(p, params[p].default_value);
}

Synth_Fx::~Synth_Fx()
{
}

void Synth_Fx::init(float sample_rate)
{
    be_->init(sample_rate);
    eq_->init(sample_rate);
    rev_->init(sample_rate);
}

void Synth_Fx::clear()
{
    be_->clear();
    eq_->clear();
    rev_->clear();
}

void Synth_Fx::compute(float data[], unsigned nframes)
{
    BassEnhance &be = *be_;
    Eq_5band &eq = *eq_;
    Reverb &rev = *rev_;
    unsigned index = 0;

    while (index < nframes) {
        constexpr unsigned maxframes = 512;

        unsigned cur = nframes - index;
        cur = (cur < maxframes) ? cur : maxframes;

        float ch1[maxframes];
        float ch2[maxframes];

        for (unsigned i = 0; i < cur; ++i) {
            const float *frame = data + 2 * (index + i);
            ch1[i] = frame[0];
            ch2[i] = frame[1];
        }

        if (be_enable_)
            be.compute(ch1, ch2, cur);

        if (eq_enable_)
            eq.compute(ch1, ch2, cur);

        if (rev_enable_)
            rev.compute(ch1, ch2, cur);

        for (unsigned i = 0; i < cur; ++i) {
            float *frame = data + 2 * (index + i);
            frame[0] = ch1[i];
            frame[1] = ch2[i];
        }

        index += cur;
    }
}

int Synth_Fx::get_parameter(size_t index) const
{
    const BassEnhance &be = *be_;
    const Eq_5band &eq = *eq_;
    const Reverb &rev = *rev_;

    switch (index) {
    case P_Bass_Enhance:
        return be_enable_ ? 100 : 0;
    case P_Bass_Amount:
        return static_cast<int>(be.get_parameter(0));
    case P_Bass_Tone:
        return static_cast<int>(be.get_parameter(1));
    case P_Eq_Enable:
        return eq_enable_ ? 100 : 0;
    case P_Eq_Low:
        return static_cast<int>(eq.get_parameter(0));
    case P_Eq_Mid_Low:
        return static_cast<int>(eq.get_parameter(1));
    case P_Eq_Mid:
        return static_cast<int>(eq.get_parameter(2));
    case P_Eq_Mid_High:
        return static_cast<int>(eq.get_parameter(3));
    case P_Eq_High:
        return static_cast<int>(eq.get_parameter(4));
    case P_Reverb_Enable:
        return rev_enable_ ? 100 : 0;
    case P_Reverb_Amount:
        return static_cast<int>(rev.get_parameter(1));
    case P_Reverb_Size:
        return static_cast<int>(rev.get_parameter(0));
    }
    return 0;
}

void Synth_Fx::set_parameter(size_t index, int value)
{
    BassEnhance &be = *be_;
    Eq_5band &eq = *eq_;
    Reverb &rev = *rev_;

    switch (index) {
    case P_Bass_Enhance: {
        bool en = value > 0;
        if (be_enable_ != en) {
            if (!en)
                be.clear();
            be_enable_ = en;
        }
        break;
    }
    case P_Bass_Amount:
        be.set_parameter(0, value);
        break;
    case P_Bass_Tone:
        be.set_parameter(1, value);
        break;
    case P_Eq_Enable: {
        bool en = value > 0;
        if (eq_enable_ != en) {
            if (!en)
                eq.clear();
            eq_enable_ = en;
        }
        break;
    }
    case P_Eq_Low:
        eq.set_parameter(0, value);
        break;
    case P_Eq_Mid_Low:
        eq.set_parameter(1, value);
        break;
    case P_Eq_Mid:
        eq.set_parameter(2, value);
        break;
    case P_Eq_Mid_High:
        eq.set_parameter(3, value);
        break;
    case P_Eq_High:
        eq.set_parameter(4, value);
        break;
    case P_Reverb_Enable: {
        bool en = value > 0;
        if (rev_enable_ != en) {
            if (!en)
                rev.clear();
            rev_enable_ = en;
        }
        break;
    }
    case P_Reverb_Amount:
        rev.set_parameter(1, value);
        break;
    case P_Reverb_Size:
        rev.set_parameter(0, value);
        break;
    }
}

gsl::span<const Fx_Parameter> Synth_Fx::parameters()
{
    return gsl::span<const Fx_Parameter>(fx_parameters, fx_parameter_count);
}
