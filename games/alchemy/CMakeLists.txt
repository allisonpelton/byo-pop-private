# Alchemy Deluxe (PopCap, 2004; games/alchemy/src) on the shared framework and platform layer. The game code is a
# translation of the original's game classes (games/alchemy/docs), written against the 2004 framework
# (docs/FRAMEWORK_2004.md); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: alchemy.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/alchemy/ (games/alchemy/tools/stage_alchemy_assets.py) or the retail install by itself.
# Web: alchemy.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files
# into /alpersist (IDBFS) and starts the game with callMain().
file(GLOB ALCHEMY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB ALCHEMY_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/AlchemyApp.cpp)
  add_executable(alchemy ${ALCHEMY_SOURCES} ${ALCHEMY_TESTHOOK_SOURCES})
  target_include_directories(alchemy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(alchemy PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(alchemy PRIVATE -Wall -Wno-unused-private-field)
    option(ALCHEMY_CRASH_REPORT "Alchemy: print the address of a crash" OFF)
    if(ALCHEMY_CRASH_REPORT)
      target_compile_definitions(alchemy PRIVATE ALCHEMY_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(alchemy TITLE "Alchemy"
      COPYRIGHT "Alchemy © 2004 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET alchemy POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:alchemy>
      VERBATIM)
  endif()
endif()
