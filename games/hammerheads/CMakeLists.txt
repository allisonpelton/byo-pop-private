# Hammer Heads Deluxe (Nuclide Interactive for PopCap, 2006; games/hammerheads) on the shared framework and platform layer.
# The game code is a translation of the original's game classes (games/hammerheads/docs), built from our own Ghidra export.
# Native: hammerheads.exe [-gamedir=<dir>] [-userdir=<dir>] (see src/main.cpp); it finds assets/hammerheads/ by itself.
# The game's own text (the high score table's first names, the trophies' names and descriptions) is read from the exe by
# tools/highscorenames.py and tools/trophytext.py into git-ignored files of src/. A tree with no exe in re/hammerheads gets
# plain stand-in text, so that it still builds; the real text comes the next time CMake configures with the exe there.
set(HAMMERHEADS_TEXT_FILES HighScoreNames TrophyText)
set(HAMMERHEADS_TEXT_TOOLS highscorenames trophytext)
find_program(HAMMERHEADS_PYTHON NAMES python3 python)
foreach(HAMMERHEADS_TEXT HAMMERHEADS_TOOL IN ZIP_LISTS HAMMERHEADS_TEXT_FILES HAMMERHEADS_TEXT_TOOLS)
  set(HAMMERHEADS_INC ${CMAKE_CURRENT_SOURCE_DIR}/src/${HAMMERHEADS_TEXT}.inc)
  set(HAMMERHEADS_EXE ${PROJECT_SOURCE_DIR}/re/hammerheads/bin/HammerHeadsGame.exe)
  if(HAMMERHEADS_PYTHON AND EXISTS ${HAMMERHEADS_EXE} AND (NOT EXISTS ${HAMMERHEADS_INC} OR ${HAMMERHEADS_EXE} IS_NEWER_THAN ${HAMMERHEADS_INC}))
    execute_process(COMMAND ${HAMMERHEADS_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/tools/${HAMMERHEADS_TOOL}.py
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE HAMMERHEADS_TOOL_RESULT)
  endif()
  if(NOT EXISTS ${HAMMERHEADS_INC})
    if(HAMMERHEADS_TEXT STREQUAL "HighScoreNames")
      set(HAMMERHEADS_STAND_IN "// Stand-in text: the real names are made by tools/highscorenames.py from the exe.\nstatic const char* const DEFAULT_NAMES_A[10] = { \"Player 1\", \"Player 2\", \"Player 3\", \"Player 4\", \"Player 5\", \"Player 6\", \"Player 7\", \"Player 8\", \"Player 9\", \"Player 10\" };\nstatic const char* const DEFAULT_NAMES_B[10] = { \"Player 1\", \"Player 2\", \"Player 3\", \"Player 4\", \"Player 5\", \"Player 6\", \"Player 7\", \"Player 8\", \"Player 9\", \"Player 10\" };\n")
    else()
      set(HAMMERHEADS_STAND_IN "// Stand-in text: the real texts are made by tools/trophytext.py from the exe.\nstatic const char* const TROPHY_NAMES[21] = { \"Trophy 1\", \"Trophy 2\", \"Trophy 3\", \"Trophy 4\", \"Trophy 5\", \"Trophy 6\", \"Trophy 7\", \"Trophy 8\", \"Trophy 9\", \"Trophy 10\", \"Trophy 11\", \"Trophy 12\", \"Trophy 13\", \"Trophy 14\", \"Trophy 15\", \"Trophy 16\", \"Trophy 17\", \"Trophy 18\", \"Trophy 19\", \"Trophy 20\", \"Trophy 21\" };\nstatic const char* const TROPHY_DESCRIPTIONS[21] = { \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\" };\n")
    endif()
    file(WRITE ${HAMMERHEADS_INC} "${HAMMERHEADS_STAND_IN}")
  endif()
endforeach()
file(GLOB HAMMERHEADS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(hammerheads ${HAMMERHEADS_SOURCES})
target_include_directories(hammerheads PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# The how-to-play page plays its Flash movie through the shared SWF player (engines/swf, sexyswf).
target_link_libraries(hammerheads PRIVATE sexyframework sexycompat2004 sexyswf)
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

# hammerheads_swftest plays the how-to-play movie through the shared SWF player, headless (tools/swftest/swftest.cpp)
if(NOT EMSCRIPTEN)
  add_executable(hammerheads_swftest tools/swftest/swftest.cpp)
  target_link_libraries(hammerheads_swftest PRIVATE sexyswf)
  add_custom_command(TARGET hammerheads_swftest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:hammerheads_swftest>
    VERBATIM)
endif()
