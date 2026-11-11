cmake_minimum_required(VERSION 3.20)
# byo-pop (Bring Your Own Pop[Cap]): decompilations, SDL2/WebAssembly ports and mods of PopCap games. One
# build makes every game's targets; the game files come from the user's own installs (assets/<game>/).
project(byo-pop C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Every game and tool lands in the build folder itself (build/native/bejeweled.exe, build/web/rulestest.js)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# A build shares the machine with the user and several sessions, and Ninja would run a job per core (20 here) at
# normal priority: at most 6 compiles and 2 links run at once (the user, 2026-11-01). A web link's wasm-opt has its own
# cap (cmake/ByopopWeb.cmake).
set_property(GLOBAL APPEND PROPERTY JOB_POOLS byopop_compile=6 byopop_link=2)
set(CMAKE_JOB_POOL_COMPILE byopop_compile)
set(CMAKE_JOB_POOL_LINK byopop_link)

if(EMSCRIPTEN)
  set(SDL2_LIBRARIES "")
  add_library(sdl2_port INTERFACE)
  target_compile_options(sdl2_port INTERFACE "-sUSE_SDL=2")
  target_link_options(sdl2_port INTERFACE "-sUSE_SDL=2")
  set(PORT_SDL sdl2_port)
else()
  if(NOT SDL2_DIR AND DEFINED ENV{SDL2_DIR})
    set(SDL2_DIR "$ENV{SDL2_DIR}")
  endif()
  find_package(SDL2 REQUIRED CONFIG)
  set(PORT_SDL SDL2::SDL2 SDL2::SDL2main)
endif()

add_subdirectory(third_party)
# byopop_web_game(): every game's web link options (shell page, runtime methods, the shell's C functions)
include(cmake/ByopopWeb.cmake)

# The web smoke and fwtest pack Bejeweled's staged files (assets/bejeweled/) into their .data file, so a
# web build leaves them out until those files are staged (games/bejeweled/tools/stage_assets.py, then
# configure again). Native builds read the files at run time and always build them.
set(BEJ_ASSETS_DIR "${PROJECT_SOURCE_DIR}/assets/bejeweled")
set(WEB_LACKS_BEJ_ASSETS OFF)
if(EMSCRIPTEN AND NOT IS_DIRECTORY "${BEJ_ASSETS_DIR}")
  set(WEB_LACKS_BEJ_ASSETS ON)
endif()

# Toolchain smoke test (tools/smoke): decodes Bejeweled's gem sheet and plays its music, framework-free.
option(BUILD_SMOKE "Build the toolchain smoke test" OFF)
if(BUILD_SMOKE AND WEB_LACKS_BEJ_ASSETS)
  message(WARNING "BUILD_SMOKE: assets/bejeweled/ is not staged, so the web smoke test is not built")
elseif(BUILD_SMOKE)
  add_executable(smoke tools/smoke/smoke.cpp)
  target_link_libraries(smoke PRIVATE ${PORT_SDL} stb xmp_lite)
  if(EMSCRIPTEN)
    set_target_properties(smoke PROPERTIES SUFFIX ".html")
    target_link_options(smoke PRIVATE "--preload-file=${BEJ_ASSETS_DIR}@/assets/bejeweled"
      "-sALLOW_MEMORY_GROWTH=1")
  endif()
endif()

# Shared by every game: the portable SexyAppFramework (framework/) and the platform layer (platform/),
# built together as the sexyframework library (framework/CMakeLists.txt).
add_subdirectory(framework)

if(NOT EMSCRIPTEN)
  # Native builds run from the build folder: SDL2.dll and the llvm-mingw runtime DLLs go next to the exe
  get_filename_component(PORT_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  file(GLOB PORT_RUNTIME_DLLS "${PORT_COMPILER_DIR}/../i686-w64-mingw32/bin/*.dll")
endif()

# Framework test app (tools/fwtest): loads Bejeweled's assets through the portable framework and draws a
# test screen. Native: fwtest.exe -frames=N -screenshot=<png> saves a screenshot and quits.
option(BUILD_FWTEST "Build the framework test app" ON)
if(BUILD_FWTEST AND WEB_LACKS_BEJ_ASSETS)
  message(STATUS "assets/bejeweled/ is not staged, so the web fwtest is not built")
elseif(BUILD_FWTEST)
  add_executable(fwtest tools/fwtest/fwtest.cpp tools/fwtest/winstate.cpp)
  target_link_libraries(fwtest PRIVATE sexyframework)
  if(NOT MSVC)
    target_compile_options(fwtest PRIVATE -Wno-deprecated-declarations -Wno-switch)
  endif()
  if(EMSCRIPTEN)
    target_compile_definitions(fwtest PRIVATE FWTEST_GAMEDIR="/assets" FWTEST_USERDIR="/userdata")
    set_target_properties(fwtest PROPERTIES SUFFIX ".html")
    target_link_options(fwtest PRIVATE "--preload-file=${BEJ_ASSETS_DIR}@/assets" "-sALLOW_MEMORY_GROWTH=1")
  else()
    target_compile_definitions(fwtest PRIVATE FWTEST_GAMEDIR="${BEJ_ASSETS_DIR}"
      FWTEST_USERDIR="${CMAKE_BINARY_DIR}/userdata")
    add_custom_command(TARGET fwtest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:fwtest>
      VERBATIM)
  endif()
endif()

# Music level (tools/musiclevel): the port's music level, rendered offline through Platform::Mixer, against the
# originals' engines measured by tools/musiclevel/musiclevel.c (docs/WEB_PLAYER.md, "Music levels"). Native only.
if(NOT EMSCRIPTEN)
  add_executable(portlevel tools/musiclevel/portlevel.cpp)
  target_link_libraries(portlevel PRIVATE sexyopenmpt)
  add_custom_command(TARGET portlevel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:portlevel>
    VERBATIM)
endif()

# Song position test (tools/songpostest): Mixer::GetSongPosition on modules rendered offline, through libxmp-lite and
# libopenmpt. Native only; songpostest.exe <gamedir> <music path>... (tools/check.sh unit).
if(NOT EMSCRIPTEN)
  add_executable(songpostest tools/songpostest/songpostest.cpp)
  target_link_libraries(songpostest PRIVATE sexyopenmpt)
  add_custom_command(TARGET songpostest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:songpostest>
    VERBATIM)
endif()

# Spectrum test (tools/spectrumtest): Mixer::GetSongFFT on tones rendered offline, against what BASS 2.3's FFT gives
# for the same tones. Native only; spectrumtest.exe (tools/check.sh unit).
if(NOT EMSCRIPTEN)
  add_executable(spectrumtest tools/spectrumtest/spectrumtest.cpp)
  target_link_libraries(spectrumtest PRIVATE sexyopenmpt)
  add_custom_command(TARGET spectrumtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:spectrumtest>
    VERBATIM)
endif()

# Geometry test (tools/geomtest): framework/DisplayGeometry against a table of what DDInterface gave before it moved
# out. Pure, no SDL; native only, geomtest.exe (tools/check.sh unit).
if(NOT EMSCRIPTEN)
  add_executable(geomtest tools/geomtest/geomtest.cpp framework/DisplayGeometry.cpp)
  target_include_directories(geomtest PRIVATE framework)
  # the framework's own floating point (framework/CMakeLists.txt), so the exact compares test the game's arithmetic
  if((NOT MSVC) AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$"))
    target_compile_options(geomtest PRIVATE -msse2 -mfpmath=sse)
  endif()
endif()

# Texture wrap test (tools/wraptest): framework/TriangleWrap, which cuts DrawTrianglesTex's triangles at whole-number u and v
# for a game that wraps its texture (Bejeweled 2's Hyperspace tunnel), against what wrapping means. Pure, no SDL; native only,
# wraptest.exe (tools/check.sh unit).
if(NOT EMSCRIPTEN)
  add_executable(wraptest tools/wraptest/wraptest.cpp framework/TriangleWrap.cpp)
  target_include_directories(wraptest PRIVATE framework)
  if((NOT MSVC) AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$"))
    target_compile_options(wraptest PRIVATE -msse2 -mfpmath=sse)
  endif()
endif()

# Sound test (tools/soundtest): sounds kept as 16-bit at their own rate, and voices that resample them as they mix,
# against the old pipeline (stereo floats at the device rate). Native only; soundtest.exe (tools/check.sh unit).
if(NOT EMSCRIPTEN)
  add_executable(soundtest tools/soundtest/soundtest.cpp)
  target_link_libraries(soundtest PRIVATE sexyopenmpt)
  add_custom_command(TARGET soundtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:soundtest>
    VERBATIM)
endif()

# Mod settings tests (tools/modsettingstest): the settings schema, values, placeholders, levels and score key
# entries (platform/ModSettings.*) and the mod menu's model in ModManager (mods.json and mod-settings.json
# edits, the effective manifests, next-game commits). Native: modsettingstest.exe. Web: modsettingstest.js,
# run with Node; both check the same vectors.
option(BUILD_MODSETTINGSTEST "Build the mod settings tests" ON)
if(BUILD_MODSETTINGSTEST)
  add_executable(modsettingstest tools/modsettingstest/modsettingstest.cpp)
  target_link_libraries(modsettingstest PRIVATE sexyframework)
  if(NOT MSVC)
    target_compile_options(modsettingstest PRIVATE -Wall)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(modsettingstest PROPERTIES SUFFIX ".js")
    target_compile_definitions(modsettingstest PRIVATE MODSETTINGSTEST_FIXTURES="/fixtures/mods"
      MODSETTINGSTEST_EXAMPLE="/examples/example-settings/mod.json")
    target_link_options(modsettingstest PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1" "-sALLOW_MEMORY_GROWTH=1"
      "--embed-file=${PROJECT_SOURCE_DIR}/tools/fixtures/mods@/fixtures/mods"
      "--embed-file=${PROJECT_SOURCE_DIR}/games/bejeweled/mods/example-settings/mod.json@/examples/example-settings/mod.json")
  else()
    target_compile_definitions(modsettingstest PRIVATE MODSETTINGSTEST_FIXTURES="${PROJECT_SOURCE_DIR}/tools/fixtures/mods"
      MODSETTINGSTEST_EXAMPLE="${PROJECT_SOURCE_DIR}/games/bejeweled/mods/example-settings/mod.json")
    add_custom_command(TARGET modsettingstest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:modsettingstest>
      VERBATIM)
  endif()
endif()

# JPEG test (tools/jpegtest): Platform::DecodeJPEG / LoadImageFromMemory / DecodeImage against IJG libjpeg 6b's reference
# output (third_party/libjpeg/test) and synthetic pictures (tools/jpegtest/data). Native: jpegtest.exe. Web:
# jpegtest.js, run with Node, since the error paths (setjmp/longjmp) only show in wasm what they do there.
option(BUILD_JPEGTEST "Build the JPEG test" ON)
if(BUILD_JPEGTEST)
  add_executable(jpegtest tools/jpegtest/jpegtest.cpp)
  target_link_libraries(jpegtest PRIVATE sexyframework)
  if(NOT MSVC)
    target_compile_options(jpegtest PRIVATE -Wall)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(jpegtest PROPERTIES SUFFIX ".js")
    target_compile_definitions(jpegtest PRIVATE JPEGTEST_DATA="/jpegtest/data" JPEGTEST_IJG="/jpegtest/ijg")
    target_link_options(jpegtest PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1" "-sALLOW_MEMORY_GROWTH=1"
      "--embed-file=${PROJECT_SOURCE_DIR}/tools/jpegtest/data@/jpegtest/data"
      "--embed-file=${PROJECT_SOURCE_DIR}/third_party/libjpeg/test@/jpegtest/ijg")
  else()
    target_compile_definitions(jpegtest PRIVATE JPEGTEST_DATA="${PROJECT_SOURCE_DIR}/tools/jpegtest/data"
      JPEGTEST_IJG="${PROJECT_SOURCE_DIR}/third_party/libjpeg/test")
    add_custom_command(TARGET jpegtest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:jpegtest>
      VERBATIM)
  endif()
endif()

# JPEG 2000 test (tools/jp2test): decodes the .jp2 images of a folder through the image loader (platform/openjpeg)
# and compares the pixels with reference PNGs, the decoder's own output (-write-ref, tools/jp2test/write_refs.sh);
# the loader's mask and extension rules with .jp2 files. Native only: tools/check.sh unit runs it on
# assets/bookwormadv/images when that folder is staged.
if(NOT EMSCRIPTEN)
  option(BUILD_JP2TEST "Build the JPEG 2000 test" ON)
  if(BUILD_JP2TEST)
    add_executable(jp2test tools/jp2test/jp2test.cpp)
    target_link_libraries(jp2test PRIVATE sexyframework sexyopenjpeg openjp2 stb)  # openjp2: the encoder, for synthetic images; stb: -write-ref
    if(NOT MSVC)
      target_compile_options(jp2test PRIVATE -Wall)
    endif()
    add_custom_command(TARGET jp2test POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:jp2test>
      VERBATIM)
  endif()
endif()

# Scaling test (tools/scaletest): what it costs to scale a finished frame up to the window without a 3D device (SDL's
# software renderer, GDI's stretch), and each way's picture. Native Windows only; it opens a window, so it runs under rundesk.
if(WIN32 AND NOT EMSCRIPTEN)
  add_executable(scaletest tools/scaletest/scaletest.cpp)
  target_link_libraries(scaletest PRIVATE SDL2::SDL2 gdi32)
  if(NOT MSVC)
    target_compile_options(scaletest PRIVATE -Wall)
  endif()
  add_custom_command(TARGET scaletest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:scaletest>
    VERBATIM)
endif()

# GPU test (tools/gputest): the GL GPUInterface's Direct3D 9 conventions on the real GPU, one test each, exact pixels.
# Native only; it opens a hidden GL window, so tools/check.sh gpu runs it under rundesk.
if(NOT EMSCRIPTEN)
  option(BUILD_GPUTEST "Build the GPU test" ON)
  if(BUILD_GPUTEST)
    add_executable(gputest tools/gputest/gputest.cpp)
    target_link_libraries(gputest PRIVATE sexyframework)
    if(NOT MSVC)
      target_compile_options(gputest PRIVATE -Wall)
    endif()
    add_custom_command(TARGET gputest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:gputest>
      VERBATIM)
  endif()
endif()

# Test launcher (tools/rundesk): runs a command on a private, invisible desktop so test runs of a game
# never show a window or take focus. games/bejeweled/tools/oracle/compare.sh runs itself under it.
if(WIN32 AND NOT EMSCRIPTEN)
  add_executable(rundesk tools/rundesk/rundesk.c)
endif()

# The games, each in games/<game>/
add_subdirectory(games/bejeweled)
# Big Money Deluxe: optional while it is new; the build skips it if its sources are missing
option(BUILD_BIGMONEY "Build the Big Money Deluxe port" ON)
if(BUILD_BIGMONEY AND EXISTS ${PROJECT_SOURCE_DIR}/games/bigmoney/src/WinBMApp.cpp)
  add_subdirectory(games/bigmoney)
endif()
# BookWorm Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_BOOKWORM "Build the BookWorm Deluxe port" ON)
if(BUILD_BOOKWORM AND EXISTS ${PROJECT_SOURCE_DIR}/games/bookworm/CMakeLists.txt)
  add_subdirectory(games/bookworm)
endif()
# Noah's Ark Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_NOAHSARK "Build the Noah's Ark Deluxe port" ON)
if(BUILD_NOAHSARK AND EXISTS ${PROJECT_SOURCE_DIR}/games/noahsark/CMakeLists.txt)
  add_subdirectory(games/noahsark)
endif()
# Atomica Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_ATOMICA "Build the Atomica Deluxe port" ON)
if(BUILD_ATOMICA AND EXISTS ${PROJECT_SOURCE_DIR}/games/atomica/CMakeLists.txt)
  add_subdirectory(games/atomica)
endif()
# Alchemy Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_ALCHEMY "Build the Alchemy Deluxe port" ON)
if(BUILD_ALCHEMY AND EXISTS ${PROJECT_SOURCE_DIR}/games/alchemy/CMakeLists.txt)
  add_subdirectory(games/alchemy)
endif()
# Seven Seas Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_SEVENSEAS "Build the Seven Seas Deluxe port" ON)
if(BUILD_SEVENSEAS AND EXISTS ${PROJECT_SOURCE_DIR}/games/sevenseas/CMakeLists.txt)
  add_subdirectory(games/sevenseas)
endif()
option(BUILD_ROCKETMANIA "Build the Rocket Mania Deluxe port" ON)
if(BUILD_ROCKETMANIA AND EXISTS ${PROJECT_SOURCE_DIR}/games/rocketmania/CMakeLists.txt)
  add_subdirectory(games/rocketmania)
endif()
option(BUILD_MUMMYMAZE "Build the Mummy Maze Deluxe port" ON)
if(BUILD_MUMMYMAZE AND EXISTS ${PROJECT_SOURCE_DIR}/games/mummymaze/CMakeLists.txt)
  add_subdirectory(games/mummymaze)
endif()
# Bookworm Adventures Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_BOOKWORMADV "Build the Bookworm Adventures Deluxe port" ON)
if(BUILD_BOOKWORMADV AND EXISTS ${PROJECT_SOURCE_DIR}/games/bookwormadv/CMakeLists.txt)
  add_subdirectory(games/bookwormadv)
endif()
# Bookworm Adventures Volume 2: the same engine as Volume 1, copied into its own folder; optional while it is new
option(BUILD_BOOKWORMADV2 "Build the Bookworm Adventures Volume 2 port" ON)
if(BUILD_BOOKWORMADV2 AND EXISTS ${PROJECT_SOURCE_DIR}/games/bookwormadv2/CMakeLists.txt)
  add_subdirectory(games/bookwormadv2)
endif()
# TipTop Deluxe: its own engine (games/tiptop/engine) under the framework's app host; optional while it is new
option(BUILD_TIPTOP "Build the TipTop Deluxe port" ON)
if(BUILD_TIPTOP AND EXISTS ${PROJECT_SOURCE_DIR}/games/tiptop/CMakeLists.txt)
  add_subdirectory(games/tiptop)
endif()
# AstroPop Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_ASTROPOP "Build the AstroPop Deluxe port" ON)
if(BUILD_ASTROPOP AND EXISTS ${PROJECT_SOURCE_DIR}/games/astropop/CMakeLists.txt)
  add_subdirectory(games/astropop)
endif()
# Pixelus Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_PIXELUS "Build the Pixelus Deluxe port" ON)
if(BUILD_PIXELUS AND EXISTS ${PROJECT_SOURCE_DIR}/games/pixelus/CMakeLists.txt)
  add_subdirectory(games/pixelus)
endif()
# Typer Shark! Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_TYPERSHARK "Build the Typer Shark Deluxe port" ON)
if(BUILD_TYPERSHARK AND EXISTS ${PROJECT_SOURCE_DIR}/games/typershark/CMakeLists.txt)
  add_subdirectory(games/typershark)
endif()
# Ning Po Mahjong Deluxe: Pyrogon's framework (games/ningpo/engine) under the framework's app host; optional while it
# is new
option(BUILD_NINGPO "Build the Ning Po Mahjong Deluxe port" ON)
if(BUILD_NINGPO AND EXISTS ${PROJECT_SOURCE_DIR}/games/ningpo/CMakeLists.txt)
  add_subdirectory(games/ningpo)
endif()
# Word Harmony: Sprout's engine (games/wordharmony/engine) under the framework's app host; optional while it is new
option(BUILD_WORDHARMONY "Build the Word Harmony port" ON)
if(BUILD_WORDHARMONY AND EXISTS ${PROJECT_SOURCE_DIR}/games/wordharmony/CMakeLists.txt)
  add_subdirectory(games/wordharmony)
endif()
# Retro64's engine (engines/retro64), one copy for Cosmic Bugs and Water Bugs: built when either game is
option(BUILD_COSMICBUGS "Build the Cosmic Bugs port" ON)
option(BUILD_WATERBUGS "Build the Water Bugs port" ON)
if((BUILD_COSMICBUGS AND EXISTS ${PROJECT_SOURCE_DIR}/games/cosmicbugs/CMakeLists.txt) OR
   (BUILD_WATERBUGS AND EXISTS ${PROJECT_SOURCE_DIR}/games/waterbugs/CMakeLists.txt))
  add_subdirectory(engines/retro64)
endif()
# Cosmic Bugs: Retro64's engine under the framework's app host; optional while it is new
if(BUILD_COSMICBUGS AND EXISTS ${PROJECT_SOURCE_DIR}/games/cosmicbugs/CMakeLists.txt)
  add_subdirectory(games/cosmicbugs)
endif()
# Water Bugs: Cosmic Bugs' sequel on the same Retro64 engine; optional while it is new
if(BUILD_WATERBUGS AND EXISTS ${PROJECT_SOURCE_DIR}/games/waterbugs/CMakeLists.txt)
  add_subdirectory(games/waterbugs)
endif()
# Dynomite Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_DYNOMITE "Build the Dynomite Deluxe port" ON)
if(BUILD_DYNOMITE AND EXISTS ${PROJECT_SOURCE_DIR}/games/dynomite/CMakeLists.txt)
  add_subdirectory(games/dynomite)
endif()
# Bonnie's Bookstore Deluxe: its own SDL 1.2 engine (games/bonnies/engine) under the framework's app host; optional
# while it is new; the build skips it if its folder is missing
option(BUILD_BONNIES "Build the Bonnie's Bookstore Deluxe port" ON)
if(BUILD_BONNIES AND EXISTS ${PROJECT_SOURCE_DIR}/games/bonnies/CMakeLists.txt)
  add_subdirectory(games/bonnies)
endif()
# Mahjong Escape: Ancient Japan: JollyBear's engine (games/mahjongjapan/engine) under the framework's app host;
# optional while it is new
option(BUILD_MAHJONGJAPAN "Build the Mahjong Escape: Ancient Japan port" ON)
if(BUILD_MAHJONGJAPAN AND EXISTS ${PROJECT_SOURCE_DIR}/games/mahjongjapan/CMakeLists.txt)
  add_subdirectory(games/mahjongjapan)
endif()
# Mahjong Escape: Ancient China: Ancient Japan's port (games/mahjongchina/engine is a copy of its engine) with China's
# differences; optional while it is new
option(BUILD_MAHJONGCHINA "Build the Mahjong Escape: Ancient China port" ON)
if(BUILD_MAHJONGCHINA AND EXISTS ${PROJECT_SOURCE_DIR}/games/mahjongchina/CMakeLists.txt)
  add_subdirectory(games/mahjongchina)
endif()
# Bejeweled Twist: optional while it is new; the build skips it if its folder is missing
option(BUILD_BEJEWELEDTWIST "Build the Bejeweled Twist port" ON)
if(BUILD_BEJEWELEDTWIST AND EXISTS ${PROJECT_SOURCE_DIR}/games/bejeweledtwist/CMakeLists.txt)
  add_subdirectory(games/bejeweledtwist)
endif()
# Bejeweled 2 Deluxe: optional while it is new; the build skips it if its folder is missing
option(BUILD_BEJEWELED2 "Build the Bejeweled 2 Deluxe port" ON)
if(BUILD_BEJEWELED2 AND EXISTS ${PROJECT_SOURCE_DIR}/games/bejeweled2/CMakeLists.txt)
  add_subdirectory(games/bejeweled2)
endif()
