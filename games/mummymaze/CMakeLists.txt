# Mummy Maze Deluxe (PopCap, 2002; games/mummymaze/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/mummymaze/docs), written against the 2002
# framework (docs/FRAMEWORK_2004.md for its successor, games/mummymaze/docs/FRAMEWORK_2002.md for the
# differences); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: mummymaze.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/mummymaze/ (games/mummymaze/tools/stage_mummymaze_assets.py) or the retail install by itself.
# Web: mummymaze.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game
# files into /mmpersist (IDBFS) and starts the game with callMain().
file(GLOB MUMMYMAZE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB MUMMYMAZE_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MummyApp.cpp)
  add_executable(mummymaze ${MUMMYMAZE_SOURCES} ${MUMMYMAZE_TESTHOOK_SOURCES})
  target_include_directories(mummymaze PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(mummymaze PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(mummymaze PRIVATE -Wall -Wno-unused-private-field)
    option(MM_CRASH_REPORT "Mummy Maze: print the address of a crash" OFF)
    if(MM_CRASH_REPORT)
      target_compile_definitions(mummymaze PRIVATE MM_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    # _byopop_livesplit_block: the page's speedrun timer asks for the autosplit block's address (src/LiveSplitObserver.cpp)
    byopop_web_game(mummymaze EXPORTS _byopop_livesplit_block TITLE "Mummy Maze"
      COPYRIGHT "Mummy Maze © 2002 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET mummymaze POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:mummymaze>
      VERBATIM)
  endif()
endif()
