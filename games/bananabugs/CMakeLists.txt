# Banana Bugs (PopCap, 2010; games/bananabugs) on the shared framework and platform layer. The port is a translation of
# the original's game classes (docs/WORKING_NOTES.md), built from our own Ghidra export.
# Native: bananabugs.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/bananabugs/ by itself
# (games/bananabugs/tools/stage_bananabugs_assets.py stages it from the user's own copy).
file(GLOB BANANABUGS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(bananabugs ${BANANABUGS_SOURCES})
target_include_directories(bananabugs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bananabugs PRIVATE sexyframework sexycompat2004 sexyopenjpeg)
if(NOT MSVC)
  target_compile_options(bananabugs PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and the game depends on it
  target_compile_options(bananabugs PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A float or double out of the int range converts as on x86 (cvttsd2si: INT_MIN), not as wasm's saturating conversion
  target_compile_options(bananabugs PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(bananabugs TITLE "Banana Bugs"
    COPYRIGHT "Banana Bugs © 2010 PopCap Games, Inc. All rights reserved.")
else()
  add_custom_command(TARGET bananabugs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bananabugs>
    VERBATIM)
endif()
