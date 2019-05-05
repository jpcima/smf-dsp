TARGET ?=
STATIC ?=
CC = $(TARGET)gcc
CXX = $(TARGET)g++
PKG_CONFIG = $(TARGET)pkg-config

###
PLATFORM=
EXEEXT=

GCC_PLATFORM = $(shell $(CC) -dumpmachine)
ifneq (,$(findstring linux,$(GCC_PLATFORM)))
PLATFORM=linux
endif
ifneq (,$(findstring mingw,$(GCC_PLATFORM)))
PLATFORM=mingw
EXEEXT=.exe
endif
ifneq (,$(findstring apple,$(GCC_PLATFORM)))
PLATFORM=apple
endif

ifeq (,$(PLATFORM))
$(error unrecognized platform: $(GCC_PLATFORM))
endif

###
APP = fmidiplay$(EXEEXT)

OBJS = \
  sources/fmidiplay.o \
  sources/main.o \
  sources/player/player.o \
  sources/player/playlist.o \
  sources/player/instrument.o \
  sources/player/keystate.o \
  sources/player/clock.o \
  sources/data/ins_names.o \
  sources/ui/main_layout.o \
  sources/ui/text.o \
  sources/ui/fonts.o \
  sources/ui/paint.o \
  sources/ui/color_palette.o \
  sources/ui/piano.o \
  sources/ui/file_browser.o \
  sources/ui/file_entry.o \
  sources/ui/metadata_display.o \
  sources/utility/charset.o \
  sources/utility/paths.o \
  sources/utility/file_scan.o \
  sources/utility/portfts.o \
  thirdparty/fmidi/sources/fmidi/fmidi_util.o \
  thirdparty/fmidi/sources/fmidi/fmidi_player.o \
  thirdparty/fmidi/sources/fmidi/fmidi_internal.o \
  thirdparty/fmidi/sources/fmidi/fmidi_seq.o \
  thirdparty/fmidi/sources/fmidi/file/read_smf.o \
  thirdparty/fmidi/sources/fmidi/file/identify.o \
  thirdparty/fmidi/sources/fmidi/file/read_xmi.o \
  thirdparty/fmidi/sources/fmidi/file/write_smf.o \
  thirdparty/fmidi/sources/fmidi/u_memstream.o \
  thirdparty/fmidi/sources/fmidi/u_stdio.o \
  thirdparty/rtmidi/RtMidi.o

###
CFLAGS = -O2 -g -Wall
CXXFLAGS = $(CFLAGS) -std=c++11
CPPFLAGS = -DFMIDI_STATIC=1 -DFMIDI_DISABLE_DESCRIBE_API=1
CPPFLAGS += -Isources -Ifontdata
CPPFLAGS += -Ithirdparty/fmidi/sources -Ithirdparty/rtmidi -Ithirdparty/gsl-lite/include
LDFLAGS =
PKG_CONFIG_FLAGS =

ifneq (,$(STATIC))
LDFLAGS += -static
PKG_CONFIG_FLAGS += -static
endif

DEP_CFLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) sdl2 libuv uchardet --cflags)
DEP_CXXFLAGS = $(DEP_CFLAGS)
DEP_CPPFLAGS =
DEP_LIBS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) sdl2 libuv uchardet --libs) -lboost_locale

ifeq ($(PLATFORM),linux)
DEP_LIBS += -lasound
endif
ifeq ($(PLATFORM),mingw)
DEP_LIBS += -lwinmm
endif
ifeq ($(PLATFORM),apple)
DEP_LIBS += -framework CoreMIDI -framework CoreAudio
endif

ifeq ($(PLATFORM),mingw)
DEP_LIBS += -lboost_filesystem
endif

ifeq ($(PLATFORM),mingw)
DEP_CPPFLAGS += -DWINICONV_CONST=
DEP_LIBS += -liconv
endif

###
.SUFFIXES:

all: $(APP)

clean:
	rm -f $(APP)
	rm -rf obj/

###
QUIET = @
QUIET_MSG = @echo -e
ifneq (,$(V))
QUIET =
QUIET_MSG = @true
endif

###
OBJS_ACTUAL = $(OBJS:%.o=obj/%.o)
CXX_COMPILE = $(QUIET)$(CXX) -MD -MP
CXX_LINK = $(QUIET)$(CXX)

define color_echo
	@printf "\e[%s%s\e[%s%s\e[0m\n" "$(1)" "$(2)" "$(3)" "$(4)"
endef

###
$(APP): $(OBJS_ACTUAL)
	$(call color_echo,92m,Link C++     ,33m,$@)
	$(CXX_LINK) $(LDFLAGS) -o $@ $^ $(DEP_LIBS)

$(filter obj/sources/%.o,$(OBJS_ACTUAL)): obj/%.o: %.cc
	@mkdir -p $(dir $@)
	$(call color_echo,95m,Compile C++  ,33m,$@)
	$(CXX_COMPILE) $(CXXFLAGS) $(CPPFLAGS) $(DEP_CXXFLAGS) $(DEP_CPPFLAGS) -c -o $@ $<

$(filter obj/thirdparty/fmidi/%.o,$(OBJS_ACTUAL)): obj/%.o: %.cc
	@mkdir -p $(dir $@)
	$(call color_echo,95m,Compile C++  ,33m,$@)
	$(CXX_COMPILE) -DFMIDI_BUILD=1 $(CXXFLAGS) $(CPPFLAGS) $(DEP_CXXFLAGS) $(DEP_CPPFLAGS) -c -o $@ $<

ifeq ($(PLATFORM),linux)
$(filter obj/thirdparty/rtmidi/%.o,$(OBJS_ACTUAL)): obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call color_echo,95m,Compile C++  ,33m,$@)
	$(CXX_COMPILE) -D__LINUX_ALSA__=1 $(CXXFLAGS) $(CPPFLAGS) $(DEP_CXXFLAGS) $(DEP_CPPFLAGS) -c -o $@ $<
endif
ifeq ($(PLATFORM),mingw)
$(filter obj/thirdparty/rtmidi/%.o,$(OBJS_ACTUAL)): obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call color_echo,95m,Compile C++  ,33m,$@)
	$(CXX_COMPILE) -D__WINDOWS_MM__=1 $(CXXFLAGS) $(CPPFLAGS) $(DEP_CXXFLAGS) $(DEP_CPPFLAGS) -c -o $@ $<
endif
ifeq ($(PLATFORM),apple)
$(filter obj/thirdparty/rtmidi/%.o,$(OBJS_ACTUAL)): obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(call color_echo,95m,Compile C++  ,33m,$@)
	$(CXX_COMPILE) -D__MACOSX_CORE__=1 $(CXXFLAGS) $(CPPFLAGS) $(DEP_CXXFLAGS) $(DEP_CPPFLAGS) -c -o $@ $<
endif

-include $(OBJS_ACTUAL:%.o=%.d)
