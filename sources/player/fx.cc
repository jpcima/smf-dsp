#include "fx.h"
#include <cassert>

namespace Fx {

static const Parameter fx_parameters[] = {
    {"Humanize",        0, Boolean, Midi,  NoFlags},
    {"Human velocity", 50, Integer, Midi,  NoFlags},
    {"Bass Enhance",    0, Boolean, Synth, HasSeparator},
    {"Bass amount",    50, Integer, Synth, NoFlags},
    {"Bass tone",      50, Integer, Synth, NoFlags},
    {"EQ",              0, Boolean, Synth, HasSeparator},
    {"EQ Low",         50, Integer, Synth, NoFlags},
    {"EQ Mid-Low",     50, Integer, Synth, NoFlags},
    {"EQ Mid",         50, Integer, Synth, NoFlags},
    {"EQ Mid-High",    50, Integer, Synth, NoFlags},
    {"EQ High",        50, Integer, Synth, NoFlags},
    {"Reverb",          0, Boolean, Synth, HasSeparator},
    {"Reverb amount",  50, Integer, Synth, NoFlags},
    {"Reverb size",    10, Integer, Synth, NoFlags},
};

static constexpr size_t fx_parameter_count =
    sizeof(fx_parameters) / sizeof(fx_parameters[0]);

static_assert(
    fx_parameter_count == Parameter_Count,
    "The parameter count does not match");

gsl::span<const Parameter> parameters()
{
    return gsl::span<const Parameter>(fx_parameters, fx_parameter_count);
}

const Parameter &parameter(size_t index)
{
    assert(index < Parameter_Count);
    return fx_parameters[index];
}

} // namespace Fx
