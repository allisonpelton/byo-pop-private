cmake_minimum_required(VERSION 3.20)
# byo-pop (Bring Your Own Pop[Cap]): decompilations, SDL2/WebAssembly ports and mods of PopCap games. One
# build makes every game's targets; the game files come from the user's own installs (assets/<game>/).
project(byo-pop C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Every game and tool lands in the build folder itself (build/native/bejeweled.exe, build/web/rulestest.js)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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
  add_executable(fwtest tools/fwtest/fwtest.cpp)
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
