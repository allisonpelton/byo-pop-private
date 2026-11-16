# Feeding Frenzy (Sprout Games, 2005; games/feedingfrenzy) on the shared platform layer. Feeding Frenzy runs on Sprout's
# own first-generation engine in an older build than Pizza Frenzy's (namespace engine; shared with Word Harmony and
# Pizza Frenzy), not the SexyAppFramework: engines/sprout is that engine, translated and mapped onto the platform
# through a host interface, game/ is the game's own code on top of it, and src/ the app that hosts both
# (games/feedingfrenzy/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound device, and
# the 640x480 screen the game draws into.
file(GLOB FEEDINGFRENZY_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp)
file(GLOB FEEDINGFRENZY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(feedingfrenzygame STATIC ${FEEDINGFRENZY_GAME_SOURCES})
target_include_directories(feedingfrenzygame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/game)
target_link_libraries(feedingfrenzygame PUBLIC sprout)
if(NOT MSVC)
  target_compile_options(feedingfrenzygame PRIVATE -Wall -Wno-unused-private-field)
endif()

# The game: src/ hosts the engine on the framework (FeedingFrenzyApp) and draws with the engine's host blitter
# (engines/sprout/host). Native: feedingfrenzy.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it finds
# assets/feedingfrenzy/ (tools/stage_feedingfrenzy_assets.py) or the install by itself.
# Web: feedingfrenzy.html/.js/.wasm. The engine runs a frame per framework update, so no ASYNCIFY.
add_executable(feedingfrenzy ${FEEDINGFRENZY_SOURCES})
target_include_directories(feedingfrenzy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(feedingfrenzy PRIVATE feedingfrenzygame sprouthost sexyframework sexycompat2004 stb)
if(NOT MSVC)
  target_compile_options(feedingfrenzy PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  byopop_web_game(feedingfrenzy TITLE "Feeding Frenzy"
    COPYRIGHT "Feeding Frenzy © 2005 Sprout Games. All rights reserved.")
else()
  add_custom_command(TARGET feedingfrenzy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:feedingfrenzy>
    VERBATIM)
endif()
