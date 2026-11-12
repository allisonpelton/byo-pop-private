# Dynomite Deluxe (PopCap, 2003; games/dynomite/src) on the shared framework and platform layer. The game code is a
# translation of the original's game classes (games/dynomite/docs/GAME_NOTES.md), written against the 2003 framework
# (games/dynomite/docs/FRAMEWORK_2003.md); the shared sexycompat2004 (framework/compat2004) holds the old framework
# behaviours.
# Native: dynomite.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/dynomite/
# (games/dynomite/tools/stage_dynomite_assets.py) by itself.
file(GLOB DYNOMITE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/FossilizedApp.cpp)
  add_executable(dynomite ${DYNOMITE_SOURCES})
  target_include_directories(dynomite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(dynomite PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(dynomite PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(dynomite EXPORTS _byopop_livesplit_block TITLE "Dynomite"
      COPYRIGHT "Dynomite © 2003 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET dynomite POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:dynomite>
      VERBATIM)
  endif()
endif()
