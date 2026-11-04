# Rocket Mania Deluxe (PopCap and Nuclide, 2003; games/rocketmania/src) on the shared framework and platform layer. The game
# code is a translation of the original's game classes (games/rocketmania/docs), written against the 2003
# framework (docs/FRAMEWORK_2004.md for its successor, games/rocketmania/docs/FRAMEWORK_2003.md for the
# differences); the shared sexycompat2004 (framework/compat2004) holds the old framework behaviours.
# Native: rocketmania.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp); it finds
# assets/rocketmania/ (games/rocketmania/tools/stage_rocketmania_assets.py) or the retail install by itself.
# Web: rocketmania.html/.js/.wasm; its page (web/shell.html with web/page.js here) imports the user's game
# files into /rmpersist (IDBFS) and starts the game with callMain().
file(GLOB ROCKETMANIA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# Test hooks (-autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB ROCKETMANIA_TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/RocketApp.cpp)
  add_executable(rocketmania ${ROCKETMANIA_SOURCES} ${ROCKETMANIA_TESTHOOK_SOURCES})
  target_include_directories(rocketmania PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(rocketmania PRIVATE sexyframework sexycompat2004)
  if(NOT MSVC)
    target_compile_options(rocketmania PRIVATE -Wall -Wno-unused-private-field)
    option(RM_CRASH_REPORT "Rocket Mania: print the address of a crash" OFF)
    if(RM_CRASH_REPORT)
      target_compile_definitions(rocketmania PRIVATE RM_CRASH_REPORT)
    endif()
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(rocketmania TITLE "Rocket Mania"
      COPYRIGHT "Rocket Mania © 2003 PopCap Games, Inc. All rights reserved.")
  else()
    add_custom_command(TARGET rocketmania POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:rocketmania>
      VERBATIM)
  endif()
endif()
