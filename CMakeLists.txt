cmake_minimum_required(VERSION 3.20)
project(BejeweledPort C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

option(BUILD_SMOKE "Build the toolchain smoke test" OFF)
if(BUILD_SMOKE)
  add_executable(smoke tools/smoke/smoke.cpp)
  target_link_libraries(smoke PRIVATE ${PORT_SDL} stb xmp_lite)
  if(EMSCRIPTEN)
    set_target_properties(smoke PROPERTIES SUFFIX ".html")
    target_link_options(smoke PRIVATE "--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets" "-sALLOW_MEMORY_GROWTH=1")
  endif()
endif()

add_subdirectory(src)

if(NOT EMSCRIPTEN)
  # Native builds run from the build folder: SDL2.dll and the llvm-mingw runtime DLLs go next to the exe
  get_filename_component(PORT_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  file(GLOB PORT_RUNTIME_DLLS "${PORT_COMPILER_DIR}/../i686-w64-mingw32/bin/*.dll")
endif()

# The game: Bejeweled Deluxe 1.87 (src/game) on the portable framework.
# Native: bejeweled.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp).
# Web: bejeweled.html/.js/.wasm; web/shell.html imports the user's game files into /persist (IDBFS)
# and starts the game with callMain().
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/game/*.cpp)
add_executable(bejeweled src/main.cpp src/SteamPath.cpp ${GAME_SOURCES})
target_include_directories(bejeweled PRIVATE ${CMAKE_SOURCE_DIR}/src/game)
target_link_libraries(bejeweled PRIVATE sexyframework)
if(NOT MSVC)
  target_compile_options(bejeweled PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  set_target_properties(bejeweled PROPERTIES SUFFIX ".html")
  set_property(TARGET bejeweled APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html)
  target_link_options(bejeweled PRIVATE
    "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']" "-lidbfs.js")
else()
  add_custom_command(TARGET bejeweled POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweled>
    VERBATIM)
endif()

# Framework test app (tools/fwtest): loads the game's assets through the portable framework and draws a
# test screen. Native: fwtest.exe -frames=N -screenshot=<png> saves a screenshot and quits.
option(BUILD_FWTEST "Build the framework test app" ON)
if(BUILD_FWTEST)
  add_executable(fwtest tools/fwtest/fwtest.cpp)
  target_link_libraries(fwtest PRIVATE sexyframework)
  if(NOT MSVC)
    target_compile_options(fwtest PRIVATE -Wno-deprecated-declarations -Wno-switch)
  endif()
  if(EMSCRIPTEN)
    target_compile_definitions(fwtest PRIVATE FWTEST_GAMEDIR="/assets" FWTEST_USERDIR="/userdata")
    set_target_properties(fwtest PROPERTIES SUFFIX ".html")
    target_link_options(fwtest PRIVATE "--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets" "-sALLOW_MEMORY_GROWTH=1")
  else()
    target_compile_definitions(fwtest PRIVATE FWTEST_GAMEDIR="${CMAKE_SOURCE_DIR}/assets" FWTEST_USERDIR="${CMAKE_BINARY_DIR}/userdata")
    add_custom_command(TARGET fwtest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:fwtest>
      VERBATIM)
  endif()
endif()
