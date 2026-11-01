# Bonnie's Bookstore Deluxe (PopCap, 2008; games/bonnies) on the shared platform layer. The original is an SDL 1.2
# program with FMOD 3 on an engine of its own (the "PL_" layer: screens, widgets, a pak, fonts and FMOD's samples and
# module; games/bonnies/docs/ENGINE.md), not the SexyAppFramework: engine/ is that engine, translated and mapped onto
# the platform through bonnies::Host (engine/Host.h), and game/ is the translation of the game on top of it. The
# SexyAppFramework only hosts the app (src/): its window, clock, input and sound device, and the 800x600 screen the
# game draws into.
# Native: bonnies.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/bonnies/
# (games/bonnies/tools/stage_bonnies_assets.py) or the retail install by itself.
# Web: bonnies.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /bbpersist (IDBFS) and starts the game with callMain(). The game keeps its own loop (and one in each modal dialog), so
# the web build is made with ASYNCIFY: the loop's sleep yields to the browser.
# The engine's header names (Graphics.h, Widget.h, ...) are the framework's too, so engine/ and game/ are on the
# include path of the game's library only; the host includes engine/Exe.h, engine/Host.h and game/Start.h by path.
file(GLOB BONNIES_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp)
file(GLOB BONNIES_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(bonniesgame STATIC ${BONNIES_GAME_SOURCES})
target_include_directories(bonniesgame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine ${CMAKE_CURRENT_SOURCE_DIR}/game)
if(NOT MSVC)
  # the original's x87 arithmetic: no fused multiply-adds
  target_compile_options(bonniesgame PRIVATE -Wall -Wno-unused-private-field -Wno-shift-op-parentheses -ffp-contract=off)
endif()

add_executable(bonnies ${BONNIES_SOURCES})
target_link_libraries(bonnies PRIVATE bonniesgame sexyframework sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(bonnies PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  target_link_options(bonnies PRIVATE "-sASYNCIFY=1" "-sASYNCIFY_STACK_SIZE=65536")
  byopop_web_game(bonnies TITLE "Bonnie's Bookstore Deluxe"
    COPYRIGHT "Bonnie's Bookstore Deluxe © 2008 PopCap Games, Inc. All rights reserved.")
else()
  add_custom_command(TARGET bonnies POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bonnies>
    VERBATIM)
endif()
