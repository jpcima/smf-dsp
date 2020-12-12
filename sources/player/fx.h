#pragma once
#include <gsl/gsl>

namespace Fx {
    enum Category { Midi, Synth };
    enum Type { Integer, Boolean };
    enum Flag { NoFlags = 0, HasSeparator = 1 };

    struct Parameter {
        const char *name;
        int default_value;
        Fx::Type type;
        Fx::Category category;
        unsigned flags;
    };

    enum {
        P_Humanize,
        P_Humanize_Velocity,
        //
        P_Bass_Enhance,
        P_Bass_Amount,
        P_Bass_Tone,
        P_Eq_Enable,
        P_Eq_Low,
        P_Eq_Mid_Low,
        P_Eq_Mid,
        P_Eq_Mid_High,
        P_Eq_High,
        P_Reverb_Enable,
        P_Reverb_Amount,
        P_Reverb_Size,
        //
        Parameter_Count,
    };

    gsl::span<const Parameter> parameters();
    const Parameter &parameter(size_t index);
} // namespace Fx
