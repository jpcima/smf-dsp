#!/bin/bash
set -e
set -x
faust -inpl -cn eq_5band_dsp -a sources/audio/wrap/faust_architecture.cxx sources/audio/eq_5band.dsp > sources/audio/gen/eq_5band.cxx
faust -inpl -cn reverb_dsp -a sources/audio/wrap/faust_architecture.cxx sources/audio/reverb.dsp > sources/audio/gen/reverb.cxx
