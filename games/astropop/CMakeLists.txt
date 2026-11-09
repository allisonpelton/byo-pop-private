# AstroPop Deluxe (PopCap, 2004; games/astropop/src) on the shared framework and platform layer. The game code is a
# translation of the original's game classes (games/astropop/docs), written against the 2004 framework compatibility
# map (docs/FRAMEWORK_2004.md); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: astropop.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/astropop/ (games/astropop/tools/stage_astropop_assets.py) or the retail install by itself.
# Web: astropop.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /appersist (IDBFS) and starts the game with callMain(). The music is an MO3 module: libopenmpt via sexyopenmpt.
file(GLOB ASTROPOP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay and the standard test commands): test code kept out of the game code
file(GLOB ASTROPOP_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/AstroPopApp.cpp)
  add_executable(astropop ${ASTROPOP_SOURCES} ${ASTROPOP_TESTHOOK_SOURCES})
  target_include_directories(astropop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(astropop PRIVATE sexyframework sexycompat2004 sexyopenmpt)
  if(NOT MSVC)
    target_compile_options(astropop PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(astropop TITLE "AstroPop"
      COPYRIGHT "AstroPop © 2004 PopCap Games, Inc. All rights reserved." LICENSES libopenmpt)
  else()
    add_custom_command(TARGET astropop POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:astropop>
      VERBATIM)
  endif()
endif()
