# Atomica Deluxe (PopCap, 2003; games/atomica/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/atomica/docs), written against the 2002
# framework (docs/FRAMEWORK_2004.md for its successor, games/atomica/docs/FRAMEWORK_2003.md for the
# differences); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: atomica.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/atomica/ (games/atomica/tools/stage_atomica_assets.py) or the retail install by itself.
# Web: atomica.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game
# files into /atpersist (IDBFS) and starts the game with callMain().
file(GLOB ATOMICA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB ATOMICA_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/AtomicaApp.cpp)
  add_executable(atomica ${ATOMICA_SOURCES} ${ATOMICA_TESTHOOK_SOURCES})
  target_include_directories(atomica PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(atomica PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(atomica PRIVATE -Wall -Wno-unused-private-field)
    option(ATOMICA_CRASH_REPORT "Atomica: print the address of a crash" OFF)
    if(ATOMICA_CRASH_REPORT)
      target_compile_definitions(atomica PRIVATE ATOMICA_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(atomica TITLE "Atomica Deluxe"
      COPYRIGHT "Atomica Deluxe © 2003 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET atomica POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:atomica>
      VERBATIM)
  endif()
endif()
