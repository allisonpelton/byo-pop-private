# Ning Po Mahjong Deluxe (Pyrogon, 2002; games/ningpo) on the shared platform layer. Ning Po runs on Pyrogon's own
# framework (PGApp, PGDialog, HDisplay...; shared with Candy Cruncher), not the SexyAppFramework: engine/ is that
# framework, translated and mapped onto the platform, and src/ is the translation of the game on top of it
# (games/ningpo/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound device, and the
# 640x480 screen the game draws into.
# Native: ningpo.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/ningpo/
# (games/ningpo/tools/stage_ningpo_assets.py) or the retail install by itself.
# Web: ningpo.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /nppersist (IDBFS) and starts the game with callMain(). The game keeps its own loop (and a loop of its own in each
# dialog), so the web build is made with ASYNCIFY: the loop's sleep yields to the browser.
file(GLOB NINGPO_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB NINGPO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(ningpoengine STATIC ${NINGPO_ENGINE_SOURCES})
target_include_directories(ningpoengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(ningpoengine PUBLIC sexyframework stb)
if(NOT MSVC)
  target_compile_options(ningpoengine PRIVATE -Wall -Wno-unused-private-field)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/NingPoApp.cpp)
  add_executable(ningpo ${NINGPO_SOURCES})
  target_include_directories(ningpo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(ningpo PRIVATE ningpoengine sexyframework sexycompat2004 stb)
  if(NOT MSVC)
    target_compile_options(ningpo PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    target_link_options(ningpo PRIVATE "-sASYNCIFY=1" "-sASYNCIFY_STACK_SIZE=65536")
    byopop_web_game(ningpo TITLE "Ning Po Mahjong"
      COPYRIGHT "Ning Po Mahjong © 2002 Pyrogon, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET ningpo POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:ningpo>
      VERBATIM)
  endif()
endif()
