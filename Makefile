PREFIX ?= /usr/local
STATIC ?=
TARGET ?=

-include Makefile.ini

###
include build/base.mk

override PKG_CONFIG_FLAGS += $(if $(STATIC),-static)
override LDFLAGS += $(if $(STATIC),-static)

###
APP := fmidiplay$(APP_EXT)
SOURCES := \
  sources/configuration.cc \
  sources/fmidiplay.cc \
  sources/main.cc \
  sources/player/player.cc \
  sources/player/seeker.cc \
  sources/player/playlist.cc \
  sources/player/instrument.cc \
  sources/player/keystate.cc \
  sources/player/clock.cc \
  sources/player/smftext.cc \
  sources/player/adev/adev.cc \
  sources/player/adev/adev_rtaudio.cc \
  sources/player/adev/adev_haiku.cc \
  sources/player/adev/adev_jack.cc \
  sources/synth/synth_host.cc \
  sources/data/ins_names.cc \
  sources/ui/main_layout.cc \
  sources/ui/text.cc \
  sources/ui/fonts.cc \
  sources/ui/fonts_data.cc \
  sources/ui/paint.cc \
  sources/ui/color_palette.cc \
  sources/ui/piano.cc \
  sources/ui/file_browser.cc \
  sources/ui/file_browser_model.cc \
  sources/ui/file_entry.cc \
  sources/ui/metadata_display.cc \
  sources/ui/modal_box.cc \
  sources/utility/charset.cc \
  sources/utility/paths.cc \
  sources/utility/file_scan.cc \
  sources/utility/portfts.cc \
  sources/utility/uv++.cc \
  sources/utility/load_library.cc \
  thirdparty/fmidi/sources/fmidi/fmidi_mini.cc \
  thirdparty/simpleini/ConvertUTF.cpp \
  thirdparty/rtaudio/RtAudio.cpp \
  thirdparty/rtmidi/RtMidi.cpp \
  thirdparty/ring_buffer/ring_buffer.cpp \
  thirdparty/uchardet/src/CharDistribution.cpp \
  thirdparty/uchardet/src/JpCntx.cpp \
  thirdparty/uchardet/src/nsCharSetProber.cpp \
  thirdparty/uchardet/src/nsLatin1Prober.cpp \
  thirdparty/uchardet/src/nsMBCSSM.cpp \
  thirdparty/uchardet/src/nsSJISProber.cpp \
  thirdparty/uchardet/src/nsUTF8Prober.cpp

$(APP): CPPFLAGS += \
    -DPROGRAM_NAME='"fmidiplay"' \
    -DPROGRAM_DISPLAY_NAME='"FMidiPlay"' \
    -DPROGRAM_AUTHOR='"JPC"' \
    -DPROGRAM_VERSION='"0.10a"' \
    -Isources -Ifontdata -Iimagedata \
    -Ithirdparty/gsl-lite/include \
    -Ithirdparty/any-lite/include \
    -Ithirdparty/bst \
    -Ithirdparty/fmidi/sources -DFMIDI_STATIC=1 -DFMIDI_DISABLE_DESCRIBE_API=1 \
    -Ithirdparty/simpleini \
    -Ithirdparty/rtmidi \
    -Ithirdparty/rtaudio \
    -Ithirdparty/ring_buffer \
    -Ithirdparty/uchardet/src \
    -DSI_CONVERT_GENERIC \
    $(if $(LINUX),-D__LINUX_ALSA__=1 -D__LINUX_PULSE__=1) \
    $(if $(MINGW),-D__WINDOWS_MM__=1 -D__WINDOWS_DS__=1) \
    $(if $(APPLE),-D__MACOSX_CORE__=1) \
    $(if $(NOT_WASM),$(call pkg_config_cflags,sdl2 SDL2_image libuv)) \
    $(if $(NOT_WASM),$(call pkg_config_cflags,icu-uc icu-i18n)) \
    $(if $(WASM),-s USE_SDL=2 -s USE_SDL_IMAGE=2) \
    $(if $(WASM),-s USE_ICU=1) \
    $(if $(LINUX),$(call pkg_config_cflags,jack alsa libpulse-simple)) \
    $(if $(MINGW),-DWINICONV_CONST=) \
    $(if $(LINUX)$(MINGW)$(APPLE),-pthread)
$(APP): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX)$(MINGW)$(APPLE),-pthread)
$(APP): LIBS += \
    $(if $(NOT_WASM),$(call pkg_config_libs,sdl2 SDL2_image libuv)) \
    $(if $(NOT_WASM),$(call pkg_config_libs,icu-uc icu-i18n)) \
    $(if $(WASM),-s USE_SDL=2 -s USE_SDL_IMAGE=2) \
    $(if $(WASM),-s USE_ICU=1) \
    $(if $(LINUX),$(call pkg_config_libs,jack alsa libpulse-simple)) \
    $(if $(MINGW),-lwinmm -ldsound) \
    $(if $(APPLE),-framework CoreMIDI -framework CoreAudio -framework CoreFoundation) \
    $(if $(HAIKU),-lmedia) \
    $(if $(MINGW),-lboost_filesystem) \
    $(if $(MINGW)$(APPLE)$(HAIKU),-liconv) \
    $(if $(LINUX),-ldl)

include build/app.mk

###
S_FLUID_ENABLE := $(shell $(PKG_CONFIG) --exists fluidsynth && echo 1)
ifneq ($(S_FLUID_ENABLE),1)
$(call color_warning,Fluidsynth is missing; skipping plugin s_fluid)
endif

ifeq ($(S_FLUID_ENABLE),1)
PLUGIN := s_fluid$(LIB_EXT)
SOURCES := \
  sources/synth/plugins/fluid.cc \
  sources/synth/synth_utility.cc \
  sources/utility/paths.cc

include build/plugin.mk

$(PLUGIN): CPPFLAGS += \
    -Isources \
    -Ithirdparty/gsl-lite/include \
    $(shell $(PKG_CONFIG) $(if $(STATIC),-static) fluidsynth --cflags)
$(PLUGIN): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX),-Xlinker -no-undefined)
$(PLUGIN): LIBS := \
    $(shell $(PKG_CONFIG) $(if $(STATIC),-static) fluidsynth --libs)
endif

###
S_ADLMIDI_ENABLE := $(shell test -d thirdparty/libADLMIDI && echo 1)
ifneq ($(S_ADLMIDI_ENABLE),1)
$(call color_warning,libADLMIDI sources are missing from thirdparty/libADLMIDI; skipping plugin s_adlmidi)
endif

ifeq ($(S_ADLMIDI_ENABLE),1)
PLUGIN := s_adlmidi$(LIB_EXT)
SOURCES := \
  sources/synth/plugins/adlmidi.cc \
  sources/utility/paths.cc \
  $(wildcard thirdparty/libADLMIDI/src/*.c*) \
  $(wildcard thirdparty/libADLMIDI/src/chips/*.c*) \
  $(wildcard thirdparty/libADLMIDI/src/chips/*/*.c*) \
  $(wildcard thirdparty/libADLMIDI/src/wopl/*.c*)

include build/plugin.mk

$(PLUGIN): CPPFLAGS += \
    -Isources \
    -Ithirdparty/gsl-lite/include \
    -Ithirdparty/libADLMIDI/include \
    -Ithirdparty/libADLMIDI/src \
    -DADLMIDI_DISABLE_MIDI_SEQUENCER \
    -DADLMIDI_DISABLE_CPP_EXTRAS
$(PLUGIN): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX),-Xlinker -no-undefined)
endif

###
S_OPNMIDI_ENABLE := $(shell test -d thirdparty/libOPNMIDI && echo 1)
ifneq ($(S_OPNMIDI_ENABLE),1)
$(call color_warning,libOPNMIDI sources are missing from thirdparty/libOPNMIDI; skipping plugin s_opnmidi)
endif

ifeq ($(S_OPNMIDI_ENABLE),1)
PLUGIN := s_opnmidi$(LIB_EXT)
SOURCES := \
  sources/synth/plugins/opnmidi.cc \
  sources/utility/paths.cc \
  $(wildcard thirdparty/libOPNMIDI/src/*.c*) \
  $(wildcard thirdparty/libOPNMIDI/src/chips/*.c*) \
  $(wildcard thirdparty/libOPNMIDI/src/chips/*/*.c*) \
  $(wildcard thirdparty/libOPNMIDI/src/wopn/*.c*)

include build/plugin.mk

$(PLUGIN): CPPFLAGS += \
    -Isources \
    -Ithirdparty/gsl-lite/include \
    -Ithirdparty/libOPNMIDI/include \
    -Ithirdparty/libOPNMIDI/src \
    -DOPNMIDI_DISABLE_MIDI_SEQUENCER \
    -DOPNMIDI_DISABLE_CPP_EXTRAS
$(PLUGIN): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX),-Xlinker -no-undefined)
endif

###
S_SCC_ENABLE := $(shell test -d thirdparty/scc && echo 1)
ifneq ($(S_SCC_ENABLE),1)
$(call color_warning,scc sources are missing from thirdparty/scc; skipping plugin s_scc)
endif

ifeq ($(S_SCC_ENABLE),1)
PLUGIN := s_scc$(LIB_EXT)
SOURCES := \
  sources/synth/plugins/scc.cc \
  $(wildcard thirdparty/scc/emidi_alpha/C*.cpp) \
  $(wildcard thirdparty/scc/emidi_alpha/device/*.c)

include build/plugin.mk

$(PLUGIN): CPPFLAGS += \
    -Isources \
    -Ithirdparty/gsl-lite/include \
    -Ithirdparty/scc
$(PLUGIN): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX),-Xlinker -no-undefined)
endif

###
S_MT32EMU_ENABLE := $(shell test -d thirdparty/munt && echo 1)
ifneq ($(S_MT32EMU_ENABLE),1)
$(call color_warning,munt sources are missing from thirdparty/munt; skipping plugin s_mt32emu)
endif

ifeq ($(S_MT32EMU_ENABLE),1)
PLUGIN := s_mt32emu$(LIB_EXT)
SOURCES := \
  sources/synth/plugins/mt32emu.cc \
  sources/utility/paths.cc \
  $(wildcard thirdparty/munt/mt32emu/src/*.cpp) \
  $(wildcard thirdparty/munt/mt32emu/src/c_interface/*.cpp) \
  $(wildcard thirdparty/munt/mt32emu/src/sha1/*.cpp) \
  thirdparty/munt/mt32emu/src/srchelper/InternalResampler.cpp \
  $(wildcard thirdparty/munt/mt32emu/src/srchelper/srctools/src/*.cpp)

include build/plugin.mk

$(PLUGIN): CPPFLAGS += \
    -Isources \
    -Ithirdparty/gsl-lite/include \
    -Ithirdparty/munt/mt32emu/src \
    -Igen/s_mt32emu$(LIB_EXT) \
    -DMT32EMU_WITH_INTERNAL_RESAMPLER \
    -DMT32EMU_EXPORTS_TYPE=1 \
    -DMT32EMU_VERSION='"0.0.0"' \
    -DMT32EMU_VERSION_MAJOR=0 \
    -DMT32EMU_VERSION_MINOR=0 \
    -DMT32EMU_VERSION_PATCH=0
$(PLUGIN): LDFLAGS += \
    $(if $(STATIC),-static) \
    $(if $(LINUX),-Xlinker -no-undefined)

$(_OBJS): gen/s_mt32emu$(LIB_EXT)/config.h
gen/s_mt32emu$(LIB_EXT)/config.h:
	@mkdir -p $(dir $@)
	touch $@
endif

###
install: all
	install -D -m 755 fmidiplay$(APP_EXT) $(DESTDIR)$(PREFIX)/bin/fmidiplay$(APP_EXT)
ifeq ($(S_FLUID_ENABLE),1)
	install -D -m 755 s_fluid$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_fluid$(LIB_EXT)
endif
ifeq ($(S_ADLMIDI_ENABLE),1)
	install -D -m 755 s_adlmidi$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_adlmidi$(LIB_EXT)
endif
ifeq ($(S_OPNMIDI_ENABLE),1)
	install -D -m 755 s_opnmidi$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_opnmidi$(LIB_EXT)
endif
ifeq ($(S_SCC_ENABLE),1)
	install -D -m 755 s_scc$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_scc$(LIB_EXT)
endif
ifeq ($(S_MT32EMU_ENABLE),1)
	install -D -m 755 s_mt32emu$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_mt32emu$(LIB_EXT)
endif
.PHONY: install
