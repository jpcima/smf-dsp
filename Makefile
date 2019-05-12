PREFIX ?= /usr/local
STATIC ?=
TARGET ?=

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
  thirdparty/rtmidi/RtMidi.cpp

$(APP): CPPFLAGS += \
    -Isources -Ifontdata \
    -Ithirdparty/gsl-lite/include -Ithirdparty/bst \
    -Ithirdparty/fmidi/sources -DFMIDI_STATIC=1 -DFMIDI_DISABLE_DESCRIBE_API=1 \
    -Ithirdparty/rtmidi \
    $(if $(LINUX),-D__LINUX_ALSA__=1 -D__UNIX_JACK__=1 -DJACK_HAS_PORT_RENAME=1 -DHAVE_SEMAPHORE=1) \
    $(if $(MINGW),-D__WINDOWS_MM__=1) \
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
    $(if $(MINGW),-lwinmm) \
    $(if $(APPLE),-framework CoreMIDI -framework CoreAudio -framework CoreFoundation) \
    $(if $(MINGW),-lboost_filesystem) \
    $(if $(MINGW)$(APPLE),-liconv)

include build/app.mk

###
install: fmidiplay$(APP_EXT)
	install -D -m 755 fmidiplay$(APP_EXT) $(DESTDIR)$(PREFIX)/fmidiplay$(APP_EXT)
.PHONY: install
