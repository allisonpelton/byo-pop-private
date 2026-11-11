# Bejeweled 2 Deluxe (PopCap, 2007; games/bejeweled2) on the shared framework and platform layer. The port is a
# translation of the original's game classes (docs/NOTES.md), built from our own Ghidra export.
# Native: bejeweled2.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/bejeweled2/ by itself.
# Web: bejeweled2.html/.js/.wasm; its page (web/page.js and page.css here) imports the user's game files into
# /bej2persist (IDBFS) and starts the game with callMain().
file(GLOB BEJ2_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(bejeweled2 ${BEJ2_SOURCES})
target_include_directories(bejeweled2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bejeweled2 PRIVATE sexyframework sexycompat2004 sexyopenmpt)
if(NOT MSVC)
  target_compile_options(bejeweled2 PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and the game depends on it: Finity's score runs past 2^31 within a few
  # levels, and the timer bar's ((score - target) * 0x2c4) / 2 wraps negative, which the compiler would otherwise fold into
  # (score - target) * 0x2c4 / 2 with no wrap. With -fwrapv the port computes the wrapped value as the original does.
  target_compile_options(bejeweled2 PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A double out of the int range converts as on x86 (cvttsd2si: INT_MIN, NaN too), not as wasm's saturating
  # conversion: the board casts doubles to int (games/bejeweled/CMakeLists.txt has the long form)
  target_compile_options(bejeweled2 PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(bejeweled2 TITLE "Bejeweled 2"
    COPYRIGHT "Bejeweled 2 © 2004, 2007 PopCap Games, Inc. All rights reserved. Bejeweled is a registered trademark of PopCap Games, Inc."
    LICENSES libopenmpt)
else()
  add_custom_command(TARGET bejeweled2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweled2>
    VERBATIM)
endif()
