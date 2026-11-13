# Typer Shark! Deluxe (PopCap, 2004; games/typershark/src) on the shared framework and platform layer. The game code
# is a translation of the original's game classes (games/typershark/docs), written against the 2004 framework
# compatibility map (docs/FRAMEWORK_2004.md); the shared sexycompat2004 (framework/compat2004) holds the old framework
# behaviours.
# Native: typershark.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/typershark/ (games/typershark/tools/stage_typershark_assets.py) or the retail install by itself.
# Web: typershark.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game files into
# /tspersist (IDBFS) and starts the game with callMain(). The music is an MO3 module: libopenmpt via sexyopenmpt.
file(GLOB TYPERSHARK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay, test commands): test code kept out of the game code
file(GLOB TYPERSHARK_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/WinWSApp.cpp)
  add_executable(typershark ${TYPERSHARK_SOURCES} ${TYPERSHARK_TESTHOOK_SOURCES})
  target_include_directories(typershark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(typershark PRIVATE sexyframework sexycompat2004 sexyopenmpt)
  if(NOT MSVC)
    target_compile_options(typershark PRIVATE -Wall -Wno-unused-private-field)
  endif()
  # Leak counting (src/testhooks/LeakCount.cpp, docs/LEAKS.md): only the scratch build asks for it, cmake -DTS_LEAKCOUNT=ON
  if(TS_LEAKCOUNT AND NOT EMSCRIPTEN)
    target_compile_definitions(typershark PRIVATE TS_LEAKCOUNT)
    target_compile_options(typershark PRIVATE -fno-omit-frame-pointer -gline-tables-only)
    # static C++ library (libc++.dll's own operator new would not meet the counter's), the C allocation functions wrapped
    target_link_options(typershark PRIVATE -static -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free)
  endif()
  if(EMSCRIPTEN)
    # _byopop_livesplit_block: the page's speedrun timer asks for the autosplit block's address (src/LiveSplitObserver.cpp)
    byopop_web_game(typershark EXPORTS _byopop_livesplit_block TITLE "Typer Shark"
      COPYRIGHT "Typer Shark! Deluxe © 2003, 2004 PopCap Games, Inc. All rights reserved." LICENSES libopenmpt)
  else()
    add_custom_command(TARGET typershark POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:typershark>
      VERBATIM)
  endif()
endif()
