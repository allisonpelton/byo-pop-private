# Big Money Deluxe (PopCap, 2005; games/bigmoney/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/bigmoney/docs).
# Native: bigmoney.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/bigmoney/ (games/bigmoney/tools/stage_bigmoney_assets.py) or the Steam install by itself.
# Web: bigmoney.html/.js/.wasm; web/shell.html's Big Money copy (games/bigmoney/web/shell.html) imports
# the user's game files into /bmpersist (IDBFS) and starts the game with callMain().
file(GLOB BIGMONEY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the bot has its own RNG)
file(GLOB BIGMONEY_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
add_executable(bigmoney ${BIGMONEY_SOURCES} ${BIGMONEY_TESTHOOK_SOURCES})
target_include_directories(bigmoney PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bigmoney PRIVATE sexyframework)
if(NOT MSVC)
  target_compile_options(bigmoney PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  byopop_web_game(bigmoney ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html TITLE "Big Money Deluxe" COPYRIGHT
    "Big Money Deluxe © 2000-2006 PopCap Games, Inc. All rights reserved. Big Money is a registered trademark of PopCap Games, Inc.")
else()
  add_custom_command(TARGET bigmoney POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bigmoney>
    VERBATIM)
endif()
