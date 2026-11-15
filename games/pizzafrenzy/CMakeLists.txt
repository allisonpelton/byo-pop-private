# Pizza Frenzy (Sprout Games, 2006; games/pizzafrenzy) on the shared platform layer. Pizza Frenzy runs on Sprout's own
# first-generation engine (namespace engine; shared with Word Harmony), not the SexyAppFramework: engines/sprout is
# that engine, translated and mapped onto the platform through a host interface (one copy, shared with Word Harmony),
# game/ is the game's own code on top of it, and src/ the app that hosts both (games/pizzafrenzy/docs). The
# SexyAppFramework only hosts the app: its window, clock, input and sound device, and the 800x600 screen the game
# draws into.
file(GLOB PIZZAFRENZY_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp)
file(GLOB PIZZAFRENZY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(pizzafrenzygame STATIC ${PIZZAFRENZY_GAME_SOURCES})
target_include_directories(pizzafrenzygame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/game)
target_link_libraries(pizzafrenzygame PUBLIC sprout)
if(NOT MSVC)
  target_compile_options(pizzafrenzygame PRIVATE -Wall -Wno-unused-private-field)
endif()

# The game: src/ hosts the engine on the framework (PizzaFrenzyApp) and draws with the engine's host blitter
# (engines/sprout/host). Native: pizzafrenzy.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it finds
# assets/pizzafrenzy/ (tools/stage_pizzafrenzy_assets.py) or the retail install by itself.
# Web: pizzafrenzy.html/.js/.wasm. The engine runs a frame per framework update, so no ASYNCIFY.
add_executable(pizzafrenzy ${PIZZAFRENZY_SOURCES})
target_include_directories(pizzafrenzy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pizzafrenzy PRIVATE pizzafrenzygame sprouthost sexyframework sexycompat2004 stb)
if(NOT MSVC)
  target_compile_options(pizzafrenzy PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  byopop_web_game(pizzafrenzy TITLE "Pizza Frenzy"
    COPYRIGHT "Pizza Frenzy © 2006 Sprout Games. All rights reserved.")
else()
  add_custom_command(TARGET pizzafrenzy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:pizzafrenzy>
    VERBATIM)
endif()
