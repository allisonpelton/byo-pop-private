# Seven Seas Deluxe (PopCap, 2003; games/sevenseas/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/sevenseas/docs), written against the 2003
# framework (docs/FRAMEWORK_2004.md for its successor, games/sevenseas/docs/FRAMEWORK_2003.md for the
# differences); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: sevenseas.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/sevenseas/ (games/sevenseas/tools/stage_sevenseas_assets.py) or the retail install by itself.
# Web: sevenseas.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game
# files into /sspersist (IDBFS) and starts the game with callMain().
file(GLOB SEVENSEAS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB SEVENSEAS_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/SevenSeasApp.cpp)
  add_executable(sevenseas ${SEVENSEAS_SOURCES} ${SEVENSEAS_TESTHOOK_SOURCES})
  target_include_directories(sevenseas PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(sevenseas PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(sevenseas PRIVATE -Wall -Wno-unused-private-field)
    option(SS_CRASH_REPORT "Seven Seas: print the address of a crash" OFF)
    if(SS_CRASH_REPORT)
      target_compile_definitions(sevenseas PRIVATE SS_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(sevenseas TITLE "Seven Seas Deluxe"
      COPYRIGHT "Seven Seas Deluxe © 2003 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET sevenseas POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:sevenseas>
      VERBATIM)
  endif()
endif()
