# TipTop Deluxe (PopCap, 2003; games/tiptop) on the shared framework and platform layer. TipTop has its own engine,
# LOWOS (DirectDraw, DirectInput, DirectSound), not the SexyAppFramework: engine/ maps it onto the platform and
# src/ is the translation of the game on top of it (games/tiptop/docs). The framework only hosts the app: its
# window, clock, input and the 640x480 screen the game draws into.
# Native: tiptop.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/tiptop/
# (games/tiptop/tools/stage_tiptop_assets.py) or the retail install by itself.
# Web: tiptop.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /ttpersist (IDBFS) and starts the game with callMain().
# The music's General MIDI bank (third_party/generaluser-gs/tiptop.sf2, src/MusicSynth.h) is copied beside the native
# exe and embedded in the web build.
set(TIPTOP_SOUNDFONT ${PROJECT_SOURCE_DIR}/third_party/generaluser-gs/tiptop.sf2)
file(GLOB TIPTOP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code
file(GLOB TIPTOP_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/TipTopApp.cpp)
  add_executable(tiptop ${TIPTOP_SOURCES} ${TIPTOP_TESTHOOK_SOURCES})
  target_include_directories(tiptop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/engine)
  target_link_libraries(tiptop PRIVATE sexyframework sexycompat2004 stb tinysoundfont)
  if(NOT MSVC)
    target_compile_options(tiptop PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(tiptop TITLE "TipTop Deluxe"
      COPYRIGHT "TipTop Deluxe © 2003 PopCap Games, Inc. All rights reserved." LICENSES tinysoundfont generaluser-gs)
    target_link_options(tiptop PRIVATE "--embed-file=${TIPTOP_SOUNDFONT}@/tiptop.sf2")
    set_property(TARGET tiptop APPEND PROPERTY LINK_DEPENDS ${TIPTOP_SOUNDFONT})
  else()
    add_custom_command(TARGET tiptop POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:tiptop>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TIPTOP_SOUNDFONT} $<TARGET_FILE_DIR:tiptop>
      VERBATIM)
  endif()
endif()
