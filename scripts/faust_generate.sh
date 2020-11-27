#!/bin/bash
set -e
set -x
faust -inpl -cn Eq_5band_dsp -a sources/audio/wrap/faust_architecture.cxx sources/audio/eq_5band.dsp > sources/audio/gen/eq_5band.cxx
faust -inpl -cn Reverb_dsp -a sources/audio/wrap/faust_architecture.cxx sources/audio/reverb.dsp > sources/audio/gen/reverb.cxx
faust -inpl -cn BassEnhance_dsp -a sources/audio/wrap/faust_architecture.cxx sources/audio/bass_enhance.dsp > sources/audio/gen/bass_enhance.cxx
