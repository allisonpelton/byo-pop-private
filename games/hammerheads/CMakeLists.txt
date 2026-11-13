# Hammer Heads Deluxe (Nuclide Interactive for PopCap, 2006; games/hammerheads) on the shared framework and platform layer.
# The game code is a translation of the original's game classes (games/hammerheads/docs), built from our own Ghidra export.
# Native: hammerheads.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/hammerheads/ by itself.
file(GLOB HAMMERHEADS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(hammerheads ${HAMMERHEADS_SOURCES})
target_include_directories(hammerheads PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(hammerheads PRIVATE sexyframework sexycompat2004)
if(NOT MSVC)
  target_compile_options(hammerheads PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and its scores and timers depend on it
  target_compile_options(hammerheads PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A float or double out of the int range converts as on x86 (cvttsd2si: INT_MIN), not as wasm's saturating conversion
  target_compile_options(hammerheads PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(hammerheads TITLE "Hammer Heads Deluxe"
    COPYRIGHT "Hammer Heads Deluxe © 2006 Nuclide Interactive and PopCap Games, Inc. All rights reserved.")
else()
  add_custom_command(TARGET hammerheads POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:hammerheads>
    VERBATIM)
endif()
