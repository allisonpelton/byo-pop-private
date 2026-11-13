# Heavy Weapon Deluxe (PopCap, 2005; the decompilation is named Popgun; games/heavyweapon) on the shared framework and
# platform layer. The game code is a translation of the original's game classes from our own Ghidra export, written
# against the shared framework with the 2004 compatibility library (framework/compat2004) where the original's early-2005
# framework behaves as BookWorm's does (games/heavyweapon/docs/FRAMEWORK_2005.md).
# Native: heavyweapon.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/heavyweapon/ by itself
# (games/heavyweapon/tools/stage_heavyweapon_assets.py). The music is an MO3 module: libopenmpt via sexyopenmpt.
# The three Flash cutscenes play through the shared SWF player (engines/swf, sexyswf).
file(GLOB HEAVYWEAPON_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (the standard test commands): test code kept out of the game code
file(GLOB HEAVYWEAPON_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
add_executable(heavyweapon ${HEAVYWEAPON_SOURCES} ${HEAVYWEAPON_TESTHOOK_SOURCES})
target_include_directories(heavyweapon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(heavyweapon PRIVATE sexyframework sexycompat2004 sexyopenmpt sexyswf)
if(NOT MSVC)
  target_compile_options(heavyweapon PRIVATE -Wall -Wno-overloaded-virtual -Wno-unused-private-field)
  # The original's int arithmetic wraps (x86 imul and add), and its scores and timers depend on it
  target_compile_options(heavyweapon PRIVATE -fwrapv)
endif()
if(EMSCRIPTEN)
  # A float or double out of the int range converts as on x86 (cvttsd2si: INT_MIN), not as wasm's saturating conversion
  target_compile_options(heavyweapon PRIVATE -mno-nontrapping-fptoint)
  byopop_web_game(heavyweapon TITLE "Heavy Weapon Deluxe"
    COPYRIGHT "Heavy Weapon Deluxe © 2005 PopCap Games, Inc. All rights reserved." LICENSES libopenmpt)
else()
  add_custom_command(TARGET heavyweapon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:heavyweapon>
    VERBATIM)
endif()
