cmake_minimum_required(VERSION "3.7")

project(smf-dsp VERSION "0.10.0" LANGUAGES C CXX)
set(PROJECT_DISPLAY_NAME "SMF-DSP")
set(PROJECT_AUTHOR "JPC")
set(PROJECT_DESCRIPTION "Advanced MIDI file player")
math(EXPR PROJECT_LETTER_VERSION "97+${PROJECT_VERSION_PATCH}")
string(ASCII "${PROJECT_LETTER_VERSION}" PROJECT_LETTER_VERSION)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}${PROJECT_LETTER_VERSION}")

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  string(APPEND CMAKE_C_FLAGS " -Wall")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  string(APPEND CMAKE_CXX_FLAGS " -Wall")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86|X86)$")
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    string(APPEND CMAKE_C_FLAGS " -msse")
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    string(APPEND CMAKE_CXX_FLAGS " -msse")
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    string(APPEND CMAKE_C_FLAGS " -municode")
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    string(APPEND CMAKE_CXX_FLAGS " -municode")
  endif()
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/check.c" "int main() { return 0; }")
try_compile(HAVE_LINKER_FLAG_AS_NEEDED "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/check.c" LINK_LIBRARIES "-Wl,--as-needed")
function(link_what_you_use TARGET)
  if(HAVE_LINKER_FLAG_AS_NEEDED)
    set_property(TARGET "${TARGET}" APPEND PROPERTY LINK_OPTIONS "-Wl,--as-needed")
  endif()
endfunction()

include("CMakeLists.deps.txt")

add_executable(smf-dsp WIN32
  "sources/configuration.cc"
  "sources/application.cc"
  "sources/main.cc"
  "sources/version_info.rc"
  "sources/audio/analyzer_10band.cc"
  "sources/audio/eq_5band.cc"
  "sources/audio/reverb.cc"
  "sources/player/player.cc"
  "sources/player/seeker.cc"
  "sources/player/playlist.cc"
  "sources/player/instrument.cc"
  "sources/player/keystate.cc"
  "sources/player/clock.cc"
  "sources/player/smftext.cc"
  "sources/player/adev/adev.cc"
  "sources/player/adev/adev_sdl.cc"
  "sources/player/adev/adev_haiku.cc"
  "sources/player/adev/adev_jack.cc"
  "sources/player/instruments/dummy.cc"
  "sources/player/instruments/port.cc"
  "sources/player/instruments/synth.cc"
  "sources/player/instruments/synth_fx.cc"
  "sources/synth/synth_host.cc"
  "sources/data/ins_names.cc"
  "sources/ui/main_layout.cc"
  "sources/ui/text.cc"
  "sources/ui/fonts.cc"
  "sources/ui/fonts_data.cc"
  "sources/ui/paint.cc"
  "sources/ui/color_palette.cc"
  "sources/ui/piano.cc"
  "sources/ui/file_browser.cc"
  "sources/ui/file_browser_model.cc"
  "sources/ui/file_entry.cc"
  "sources/ui/metadata_display.cc"
  "sources/ui/modal_box.cc"
  "sources/ui/level_meter.cc"
  "sources/utility/charset.cc"
  "sources/utility/paths.cc"
  "sources/utility/file_scan.cc"
  "sources/utility/portfts.cc"
  "sources/utility/uv++.cc"
  "sources/utility/load_library.cc"
  "sources/utility/logs.cc"
  "sources/utility/desktop.cc")

target_compile_definitions(smf-dsp PRIVATE
  "PROGRAM_NAME=\"${PROJECT_NAME}\""
  "PROGRAM_DISPLAY_NAME=\"${PROJECT_DISPLAY_NAME}\""
  "PROGRAM_AUTHOR=\"${PROJECT_AUTHOR}\""
  "PROGRAM_VERSION=\"${PROJECT_VERSION}\"")

target_include_directories(smf-dsp PRIVATE
  "sources"
  "fontdata"
  "imagedata")

target_link_libraries(smf-dsp PRIVATE
  vendor::uchardet
  vendor::gsl-lite
  vendor::any-lite
  vendor::utf
  vendor::simpleini
  vendor::ring-buffer
  vendor::fmidi
  vendor::rtmidi
  sys::SDL2
  sys::uv
  sys::iconv
  ICU::uc
  ICU::i18n
  Threads::Threads)

if(TARGET sys::jack)
  target_link_libraries(smf-dsp PRIVATE sys::jack)
  target_compile_definitions(smf-dsp PRIVATE "ADEV_JACK")
endif()

if(TARGET sys::SDL2_image)
  target_link_libraries(smf-dsp PRIVATE sys::SDL2_image)
  target_compile_definitions(smf-dsp PRIVATE "HAVE_SDL2_IMAGE")
else()
  target_link_libraries(smf-dsp PRIVATE vendor::stb)
  target_sources(smf-dsp PRIVATE "sources/utility/SDL_stb_image.c")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_link_libraries(smf-dsp PRIVATE Boost::filesystem)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(smf-dsp PRIVATE sys::jack sys::alsa)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
  target_link_libraries(smf-dsp PRIVATE sys::mediakit)
endif()

link_what_you_use(smf-dsp)

#
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/version_info")
target_include_directories(smf-dsp PRIVATE "${PROJECT_BINARY_DIR}/version_info")
configure_file("sources/version_info.h.in" "${PROJECT_BINARY_DIR}/version_info/version_info.h" @ONLY)

#
install(TARGETS smf-dsp
  RUNTIME DESTINATION "bin"
  COMPONENT "player")

install(FILES "resources/desktop/smf-dsp.desktop"
  DESTINATION "share/applications"
  COMPONENT "desktop-entry")
install(FILES "resources/desktop/smf-dsp.png"
  DESTINATION "share/pixmaps"
  COMPONENT "desktop-entry")

if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Windows|Darwin)$" AND NOT CMAKE_CROSSCOMPILING)
  find_program(XDG_OPEN_PROGRAM "xdg-open")
  if(NOT XDG_OPEN_PROGRAM)
    message(WARNING "Cannot find the runtime dependency xdg-open")
  endif()
endif()

include("CMakeLists.plugins.txt")
