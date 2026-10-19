# Bejeweled Deluxe 1.87 (games/bejeweled/src) on the shared framework and platform layer.
# Native: bejeweled.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp).
# Web: bejeweled.html/.js/.wasm; web/shell.html imports the user's game files into /persist (IDBFS)
# and starts the game with callMain().
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM GAME_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
# Test hooks (-tracehash, -autoplay): test code kept out of the game code (the autoplay bot has its own RNG)
file(GLOB TESTHOOK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/testhooks/*.cpp)
add_executable(bejeweled src/main.cpp ${GAME_SOURCES} ${TESTHOOK_SOURCES})
target_include_directories(bejeweled PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bejeweled PRIVATE sexyframework)
if(NOT MSVC)
  target_compile_options(bejeweled PRIVATE -Wall -Wno-unused-private-field)
endif()
# Code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.2): BEJ_NO_VARIANTS makes every Board variant
# flag a compile-time false, so the compiler drops the variant hooks; that build must pass the regression
# oracle too.
option(BEJ_NO_VARIANTS "Build the game without the code-mod variant paths" OFF)
if(BEJ_NO_VARIANTS)
  target_compile_definitions(bejeweled PRIVATE BEJ_NO_VARIANTS)
endif()
if(EMSCRIPTEN)
  set_target_properties(bejeweled PROPERTIES SUFFIX ".html")
  set_property(TARGET bejeweled APPEND PROPERTY LINK_DEPENDS ${PROJECT_SOURCE_DIR}/web/shell.html)
  target_link_options(bejeweled PRIVATE
    "--shell-file=${PROJECT_SOURCE_DIR}/web/shell.html"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']" "-lidbfs.js")
else()
  add_custom_command(TARGET bejeweled POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweled>
    VERBATIM)
endif()

# Code-mod rules tests (games/bejeweled/tools/rulestest): the framework-free GameRules with its unit
# fixtures and RulesId vectors, the MTRand a board owns, and the .bjr replay format (stage 3). Native:
# rulestest.exe. Web: rulestest.js, run with Node (the vectors must match native).
option(BUILD_RULESTEST "Build the code-mod rules tests" ON)
if(BUILD_RULESTEST)
  add_executable(rulestest tools/rulestest/rulestest.cpp src/GameRules.cpp src/Sha256.cpp src/Replay.cpp
    ${PROJECT_SOURCE_DIR}/platform/Json.cpp ${PROJECT_SOURCE_DIR}/framework/MTRand.cpp)
  target_include_directories(rulestest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/framework)
  if(NOT MSVC)
    target_compile_options(rulestest PRIVATE -Wall)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(rulestest PROPERTIES SUFFIX ".js")
    target_link_options(rulestest PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1" "-sALLOW_MEMORY_GROWTH=1")
  endif()
endif()

# LiveSplit probe (games/bejeweled/tools/livesplit/lsprobe): reads the game's LiveSplit block from outside
# the way the auto splitter does (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15). A 64-bit exe, like
# LiveSplit, so it reads the 32-bit game across WOW64: built with llvm-mingw's x86_64 compiler next to the
# i686 one, statically linked (the build folder's runtime DLLs are 32-bit).
option(BUILD_LSPROBE "Build the LiveSplit probe (Windows)" ON)
if(BUILD_LSPROBE AND WIN32 AND NOT EMSCRIPTEN)
  get_filename_component(LSPROBE_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(LSPROBE_CXX "${LSPROBE_COMPILER_DIR}/x86_64-w64-mingw32-clang++${CMAKE_EXECUTABLE_SUFFIX}")
  if(EXISTS "${LSPROBE_CXX}")
    set(LSPROBE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/tools/livesplit/lsprobe/lsprobe.cpp)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lsprobe.exe
      COMMAND "${LSPROBE_CXX}" -std=c++17 -O2 -Wall -static -I${CMAKE_CURRENT_SOURCE_DIR}/src
        -o ${CMAKE_BINARY_DIR}/lsprobe.exe ${LSPROBE_SOURCE} -lpsapi
      DEPENDS ${LSPROBE_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/LiveSplitBlock.h
      COMMENT "Building lsprobe.exe (x86_64)"
      VERBATIM)
    add_custom_target(lsprobe ALL DEPENDS ${CMAKE_BINARY_DIR}/lsprobe.exe)
  else()
    message(STATUS "lsprobe is not built: no x86_64 compiler at ${LSPROBE_CXX}")
  endif()
endif()
