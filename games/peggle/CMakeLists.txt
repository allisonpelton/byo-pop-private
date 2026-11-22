# Peggle Deluxe (PopCap, 2007; games/peggle) on the shared framework and platform layer. The port is a translation of
# the original's game classes (the class "Thunderball" is its app), built from our own Ghidra export.
# Native: peggle.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/peggle/ by itself
# (games/peggle/tools/stage_peggle_assets.py stages it from the user's own copy).
file(GLOB PEGGLE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(peggle ${PEGGLE_SOURCES})
target_include_directories(peggle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(peggle PRIVATE sexyframework sexycompat2004 sexyopenjpeg sexyopenmpt)
if(NOT MSVC)
  target_compile_options(peggle PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and the game depends on it
  target_compile_options(peggle PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A float or double out of the int range converts as on x86 (cvttsd2si: INT_MIN), not as wasm's saturating conversion
  target_compile_options(peggle PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(peggle TITLE "Peggle"
    COPYRIGHT "Peggle Deluxe © 2007 PopCap Games, Inc. All rights reserved."
    LICENSES libopenmpt)
else()
  add_custom_command(TARGET peggle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:peggle>
    VERBATIM)
endif()
