cmake_minimum_required(VERSION "3.7")

project(fmidiplay VERSION "0.10.0" LANGUAGES C CXX)
set(PROJECT_DISPLAY_NAME "FMidiPlay")
set(PROJECT_AUTHOR "JPC")
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

include("CMakeLists.deps.txt")

add_executable(fmidiplay WIN32
  "sources/configuration.cc"
  "sources/fmidiplay.cc"
  "sources/main.cc"
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
  "sources/utility/charset.cc"
  "sources/utility/paths.cc"
  "sources/utility/file_scan.cc"
  "sources/utility/portfts.cc"
  "sources/utility/uv++.cc"
  "sources/utility/load_library.cc"
  "sources/utility/logs.cc")

target_compile_definitions(fmidiplay PRIVATE
  "PROGRAM_NAME=\"${PROJECT_NAME}\""
  "PROGRAM_DISPLAY_NAME=\"${PROJECT_DISPLAY_NAME}\""
  "PROGRAM_AUTHOR=\"${PROJECT_AUTHOR}\""
  "PROGRAM_VERSION=\"${PROJECT_VERSION}\"")

target_include_directories(fmidiplay PRIVATE
  "sources"
  "fontdata"
  "imagedata")

target_link_libraries(fmidiplay PRIVATE
  vendor::uchardet
  vendor::gsl-lite
  vendor::any-lite
  vendor::utf
  vendor::simpleini
  vendor::ring-buffer
  vendor::fmidi
  vendor::rtmidi
  sys::SDL2
  sys::SDL2_image
  sys::uv
  sys::iconv
  ICU::uc
  ICU::i18n
  Threads::Threads)

if(TARGET sys::jack)
  target_link_libraries(fmidiplay PRIVATE sys::jack)
  target_compile_definitions(fmidiplay PRIVATE "ADEV_JACK")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_link_libraries(fmidiplay PRIVATE Boost::filesystem)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(fmidiplay PRIVATE sys::jack sys::alsa)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
  target_link_libraries(fmidiplay PRIVATE sys::mediakit)
endif()

install(TARGETS fmidiplay
  RUNTIME DESTINATION "bin"
  COMPONENT "player")

include("CMakeLists.plugins.txt")
