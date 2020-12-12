#pragma once
#include "../fx.h"
#include <memory>
class BassEnhance;
class Eq_5band;
class Reverb;

class Synth_Fx {
public:
    Synth_Fx();
    ~Synth_Fx();

    void init(float sample_rate);
    void clear();
    void compute(float data[], unsigned nframes);

    int get_parameter(size_t index) const;
    void set_parameter(size_t index, int value);

private:
    std::unique_ptr<BassEnhance> be_;
    std::unique_ptr<Eq_5band> eq_;
    std::unique_ptr<Reverb> rev_;
    bool be_enable_ = false;
    bool eq_enable_ = false;
    bool rev_enable_ = false;
};
