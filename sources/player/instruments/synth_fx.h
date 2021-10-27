#pragma once
#include <nonstd/span.hpp>
#include <memory>
class BassEnhance;
class Eq_5band;
class Reverb;

struct Fx_Parameter {
    const char *name;
    int default_value;
    enum Type { Integer, Boolean };
    Type type;
    enum Flag { HasSeparator = 1 };
    unsigned flags;
};

class Synth_Fx {
public:
    Synth_Fx();
    ~Synth_Fx();

    void init(float sample_rate);
    void clear();
    void compute(float data[], unsigned nframes);

    int get_parameter(size_t index) const;
    void set_parameter(size_t index, int value);

    static nonstd::span<const Fx_Parameter> parameters();

    enum {
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
        Parameter_Count,
    };

private:
    std::unique_ptr<BassEnhance> be_;
    std::unique_ptr<Eq_5band> eq_;
    std::unique_ptr<Reverb> rev_;
    bool be_enable_ = false;
    bool eq_enable_ = false;
    bool rev_enable_ = false;
};
