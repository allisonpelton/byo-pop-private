# Venice Deluxe (Retro64 for PopCap, 2007; games/venice) on the shared framework and platform layer. The port is a
# translation of the original's engine and game classes (docs/NOTES.md), built from our own Ghidra export.
# Native: venice.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/venice/ by itself.
file(GLOB VENICE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(venice ${VENICE_SOURCES})
target_include_directories(venice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(venice PRIVATE sexyframework sexycompat2004)
if(NOT MSVC)
  target_compile_options(venice PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and its scores and timers depend on it
  target_compile_options(venice PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A float or double out of the int range converts as on x86 (cvttsd2si: INT_MIN), not as wasm's saturating conversion
  target_compile_options(venice PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(venice EXPORTS _byopop_livesplit_block TITLE "Venice Deluxe"
    COPYRIGHT "Venice Deluxe © 2007 Retro64 and PopCap Games, Inc. All rights reserved.")
else()
  add_custom_command(TARGET venice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:venice>
    VERBATIM)
endif()
