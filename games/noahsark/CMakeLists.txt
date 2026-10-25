# Noah's Ark Deluxe (PopCap, 2002; games/noahsark/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/noahsark/docs), written against the 2002
# framework (docs/FRAMEWORK_2004.md for its successor, games/noahsark/docs/FRAMEWORK_2002.md for the
# differences); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: noahsark.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/noahsark/ (games/noahsark/tools/stage_noahsark_assets.py) or the retail install by itself.
# Web: noahsark.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game
# files into /napersist (IDBFS) and starts the game with callMain().
file(GLOB NOAHSARK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB NOAHSARK_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/NoahApp.cpp)
  add_executable(noahsark ${NOAHSARK_SOURCES} ${NOAHSARK_TESTHOOK_SOURCES})
  target_include_directories(noahsark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(noahsark PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(noahsark PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(noahsark TITLE "Noah's Ark Deluxe"
      COPYRIGHT "Noah's Ark Deluxe © 2002 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET noahsark POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:noahsark>
      VERBATIM)
  endif()
endif()
