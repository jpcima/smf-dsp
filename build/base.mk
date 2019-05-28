###
CC = $(TARGET)gcc
CXX = $(TARGET)g++
CCLD = $(CC)
CXXLD = $(CXX)
CFLAGS = -O2 -g -Wall -fvisibility=hidden
CXXFLAGS = $(CFLAGS) -std=c++11
PKG_CONFIG = $(TARGET)pkg-config
PKG_CONFIG_FLAGS =

###
_CC_PLATFORM = $(shell $(CC) -dumpmachine)
$(eval $(if $(findstring linux,$(_CC_PLATFORM)),LINUX = 1,$(if \
            $(findstring mingw,$(_CC_PLATFORM)),MINGW = 1,$(if \
            $(findstring apple,$(_CC_PLATFORM)),APPLE = 1,$(if \
            $(findstring haiku,$(_CC_PLATFORM)),HAIKU = 1, \
            $(error unrecognized platform: $(_CC_PLATFORM)))))))

###
APP_EXT = $(if $(MINGW),.exe,)
LIB_EXT = $(if $(MINGW),.dll,$(if $(APPLE),.dylib,.so))

###
define color_warning
$(warning $(shell printf "\033[31m")$(1)$(shell printf "\033[0m"))
endef

define color_print
	@printf "\033[%s%s\033[%s%s\033[0m\n" "$(1)" "$(2)" "$(3)" "$(4)"
endef

define compile_c
	@mkdir -p $(dir $@)
	$(call color_print,95m,Compile C    ,33m,$@)
	$(if $(V),,@)$(CC) -MD -MP $(CFLAGS) $(CPPFLAGS) $(1) -c -o $@ $<
endef

define compile_cxx
	@mkdir -p $(dir $@)
	$(call color_print,95m,Compile C++  ,33m,$@)
	$(if $(V),,@)$(CXX) -MD -MP $(CXXFLAGS) $(CPPFLAGS) $(1) -c -o $@ $<
endef

define link_c
	@mkdir -p $(dir $@)
	$(call color_print,92m,Link C       ,33m,$@)
	$(if $(V),,@)$(CCLD) $(LDFLAGS) $(1) -o $@ $^ $(LIBS)
endef

define link_cxx
	@mkdir -p $(dir $@)
	$(call color_print,92m,Link C++     ,33m,$@)
	$(if $(V),,@)$(CXXLD) $(LDFLAGS) $(1) -o $@ $^ $(LIBS)
endef

define pkg_config_cflags
	$(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags $(1))
endef

define pkg_config_libs
	$(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs $(1))
endef

define try_compile_c
$(shell $(CC) $(1) 2> /dev/null && echo 1; rm -f $(if $(MINGW),a.exe,a.out))
endef

define try_compile_cxx
$(shell $(CXX) $(1) 2> /dev/null && echo 1; rm -f $(if $(MINGW),a.exe,a.out))
endef

all:
clean:
.PHONY: all clean
.SUFFIXES:
