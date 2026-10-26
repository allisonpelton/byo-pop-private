# Pixelus Deluxe (Nuclide Games / PopCap, 2004; games/pixelus/src) on the shared framework and platform layer. The
# game code is a translation of the original's game classes (games/pixelus/docs), written against the 2004
# framework (docs/FRAMEWORK_2004.md); the shared sexycompat2004 (framework/compat2004) holds the old framework
# behaviours. The original also has a small application layer of its own (the Nuclide framework, "Nc..." classes);
# src/NcApp.* is its translation.
# Native: pixelus.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/pixelus/ (games/pixelus/tools/stage_pixelus_assets.py) or the retail install by itself.
# Web: pixelus.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /pxpersist (IDBFS) and starts the game with callMain().
file(GLOB PIXELUS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB PIXELUS_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/PixelusApp.cpp)
  add_executable(pixelus ${PIXELUS_SOURCES} ${PIXELUS_TESTHOOK_SOURCES})
  target_include_directories(pixelus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(pixelus PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(pixelus PRIVATE -Wall -Wno-unused-private-field)
    option(PX_CRASH_REPORT "Pixelus: print the address of a crash" OFF)
    if(PX_CRASH_REPORT)
      target_compile_definitions(pixelus PRIVATE PX_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(pixelus TITLE "Pixelus Deluxe"
      COPYRIGHT "Pixelus Deluxe © 2004 Nuclide Games and PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET pixelus POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:pixelus>
      VERBATIM)
  endif()
endif()
