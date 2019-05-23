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
  sources/fmidiplay.cc \
  sources/main.cc \
  sources/player/player.cc \
  sources/player/playlist.cc \
  sources/player/instrument.cc \
  sources/player/keystate.cc \
  sources/player/clock.cc \
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
  sources/ui/file_entry.cc \
  sources/ui/metadata_display.cc \
  sources/ui/modal_box.cc \
  sources/utility/charset.cc \
  sources/utility/paths.cc \
  sources/utility/file_scan.cc \
  sources/utility/portfts.cc \
  sources/utility/dynamic_icu.cc \
  thirdparty/fmidi/sources/fmidi/fmidi_util.cc \
  thirdparty/fmidi/sources/fmidi/fmidi_player.cc \
  thirdparty/fmidi/sources/fmidi/fmidi_internal.cc \
  thirdparty/fmidi/sources/fmidi/fmidi_seq.cc \
  thirdparty/fmidi/sources/fmidi/file/read_smf.cc \
  thirdparty/fmidi/sources/fmidi/file/identify.cc \
  thirdparty/fmidi/sources/fmidi/file/read_xmi.cc \
  thirdparty/fmidi/sources/fmidi/file/write_smf.cc \
  thirdparty/fmidi/sources/fmidi/u_memstream.cc \
  thirdparty/fmidi/sources/fmidi/u_stdio.cc \
  thirdparty/simpleini/ConvertUTF.cpp \
  thirdparty/rtaudio/RtAudio.cpp \
  thirdparty/rtmidi/RtMidi.cpp \
  thirdparty/ring_buffer/ring_buffer.cpp

$(APP): CPPFLAGS += \
    -DPROGRAM_NAME='"fmidiplay"' \
    -DPROGRAM_DISPLAY_NAME='"FMidiPlay"' \
    -Isources -Ifontdata \
    -Ithirdparty/gsl-lite/include -Ithirdparty/bst \
    -Ithirdparty/fmidi/sources -DFMIDI_STATIC=1 -DFMIDI_DISABLE_DESCRIBE_API=1 \
    -Ithirdparty/simpleini \
    -Ithirdparty/rtmidi \
    -Ithirdparty/rtaudio \
    -Ithirdparty/ring_buffer \
    $(if $(LINUX),-D__LINUX_ALSA__=1 -D__UNIX_JACK__=1 -DJACK_HAS_PORT_RENAME=1 -DHAVE_SEMAPHORE=1) \
    $(if $(MINGW),-D__WINDOWS_MM__=1 -D__WINDOWS_DS__=1) \
    $(if $(APPLE),-D__MACOSX_CORE__=1) \
    $(call pkg_config_cflags,sdl2 libuv uchardet) \
    $(if $(LINUX),$(call pkg_config_cflags,jack alsa)) \
    $(if $(MINGW),-DWINICONV_CONST=) \
    -pthread
$(APP): LDFLAGS += \
    $(if $(STATIC),-static) \
    -pthread
$(APP): LIBS += \
    $(call pkg_config_libs,sdl2 libuv uchardet) \
    $(if $(LINUX),$(call pkg_config_libs,jack alsa)) \
    $(if $(MINGW),-lwinmm -ldsound) \
    $(if $(APPLE),-framework CoreMIDI -framework CoreAudio -framework CoreFoundation) \
    $(if $(MINGW),-lboost_filesystem) \
    $(if $(MINGW)$(APPLE),-liconv) \
    $(if $(LINUX),-ldl) \

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
  sources/synth/plugins/synth_utility.cc \
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
install: all
	install -D -m 755 fmidiplay$(APP_EXT) $(DESTDIR)$(PREFIX)/bin/fmidiplay$(APP_EXT)
ifeq ($(S_FLUID_ENABLE),1)
	install -D -m 755 s_fluid$(LIB_EXT) $(DESTDIR)$(PREFIX)/lib/fmidiplay/s_fluid$(LIB_EXT)
endif
.PHONY: install
