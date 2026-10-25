# BookWorm Deluxe (PopCap, 2004; games/bookworm/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/bookworm/docs), written against the 2004
# framework compatibility map (docs/FRAMEWORK_2004.md); the shared sexycompat2004 (framework/compat2004) holds the
# old framework behaviours.
# Native: bookworm.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/bookworm/ (games/bookworm/tools/stage_bookworm_assets.py) or the retail install by itself.
# Web: bookworm.html/.js/.wasm; its page (web/shell.html with web/page.js and page.css here) imports the user's game
# files into /bwpersist (IDBFS) and starts the game with callMain(). The music is an MO3 module: libopenmpt via sexyopenmpt.
file(GLOB BOOKWORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB BOOKWORM_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/BookWormApp.cpp)
  add_executable(bookworm ${BOOKWORM_SOURCES} ${BOOKWORM_TESTHOOK_SOURCES})
  target_include_directories(bookworm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(bookworm PRIVATE sexyframework sexycompat2004 sexyopenmpt)
  if(NOT MSVC)
    target_compile_options(bookworm PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(bookworm TITLE "BookWorm Deluxe"
      COPYRIGHT "BookWorm Deluxe © 2002, 2003 PopCap Games, Inc. All rights reserved." LICENSES libopenmpt)
  else()
    add_custom_command(TARGET bookworm POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bookworm>
      VERBATIM)
  endif()
endif()

# mo3info: prints an MO3 module's orders and per-order levels (native tool, no audio device)
if(NOT EMSCRIPTEN)
  add_executable(mo3info tools/mo3info/mo3info.cpp)
  target_link_libraries(mo3info PRIVATE openmpt)
endif()
