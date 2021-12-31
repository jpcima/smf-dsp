//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "synth/synth.h"
#include <fmidi/fmidi.h>
#include <vector>

// Collects the MIDI instruments present in a file, for the needs of preloading.
// It computes a conservative estimate without trying to hard, that should
// match most synthesizers regardless of MIDI support.
std::vector<synth_midi_ins> collect_file_instruments(const fmidi_smf_t &smf);
