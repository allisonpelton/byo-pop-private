# Bejeweled Deluxe 1.87 (games/bejeweled/src) on the shared framework and platform layer.
# Native: bejeweled.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see src/main.cpp).
# Web: bejeweled.html/.js/.wasm; its page (web/shell.html with web/page.js and page.css here) imports the user's game
# files into /persist (IDBFS)
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
  # A double out of the int range converts as on x86 (cvttsd2si: INT_MIN, NaN too), not as wasm's saturating
  # conversion (INT_MAX, INT_MIN, NaN 0) that emcc emits by default: the board casts doubles that tunables and
  # diff.txt values can push out of range, and a replay recorded on one platform must verify on the other
  # (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.1 rule 3). The same switch for bejeweled_verify below.
  target_compile_options(bejeweled PRIVATE -mno-nontrapping-fptoint)
  # fflush: the Node runner (tools/verify/wasmrun.js) flushes the test hooks' files (-tracehash, -autoplaylog,
  # -codeccheck) when the game quits; a page never deletes the app, which closes them natively
  byopop_web_game(bejeweled EXPORTS _fflush TITLE "Bejeweled Deluxe"
    COPYRIGHT "Bejeweled Deluxe © 2000, 2007 PopCap Games, Inc. All rights reserved. Bejeweled is a trademark of PopCap Games, Inc.")
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
  add_executable(rulestest tools/rulestest/rulestest.cpp src/GameRules.cpp src/MatchFinder.cpp src/MoveSet.cpp src/Specials.cpp src/Sha256.cpp src/Replay.cpp
    ${PROJECT_SOURCE_DIR}/platform/Json.cpp ${PROJECT_SOURCE_DIR}/framework/MTRand.cpp)
  target_include_directories(rulestest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/framework)
  if(NOT MSVC)
    target_compile_options(rulestest PRIVATE -Wall)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(rulestest PROPERTIES SUFFIX ".js")
    # The game's stack: the v2 save tests hold a dozen VariantSaves (2.5 KB of MT state each), past the 64 KB default
    target_link_options(rulestest PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1" "-sALLOW_MEMORY_GROWTH=1" "-sSTACK_SIZE=1048576")
  endif()
endif()

# Headless verifier, bejeweled_verify (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9, src/headless/Verifier.h): the
# Board sources unchanged, compiled against the framework shims in src/headless/shim, which come first on the
# include path ("Widget.h", "SexyApp.h", <SDL.h> ... are the shims; the game's own headers in src/ are found
# next to the sources that include them). No SDL, no framework library, fonts, images, user files or mods.
# Native: bejeweled_verify.exe (i686, the game's floating point). Web: bejeweled_verify.wasm, a standalone module
# without JS glue (its WASI imports stubbed by the host: games/bejeweled/tools/verify/wasmrun.js under Node).
option(BUILD_VERIFY "Build the headless replay verifier" ON)
if(BUILD_VERIFY)
  add_executable(bejeweled_verify
    src/Board.cpp src/BoardRules.cpp src/BoardVariant.cpp src/BoardSave.cpp src/BoardShape.cpp src/BoardDrawVariant.cpp src/BoardTrace.cpp src/BoardMatch.cpp src/BoardMoves.cpp src/BoardSpecials.cpp src/BoardGold.cpp src/GameConfig.cpp src/GameRules.cpp
    src/MatchFinder.cpp src/MoveSet.cpp src/Specials.cpp src/StateCodec.cpp src/Replay.cpp src/Sha256.cpp src/WatchController.cpp
    src/headless/HeadlessShims.cpp src/headless/WinDMAppHeadless.cpp src/headless/ReplayRecorderHeadless.cpp
    src/headless/DiffBot.cpp src/headless/Verifier.cpp src/headless/verify_main.cpp
    ${PROJECT_SOURCE_DIR}/framework/MTRand.cpp ${PROJECT_SOURCE_DIR}/framework/Buffer.cpp
    ${PROJECT_SOURCE_DIR}/framework/Color.cpp ${PROJECT_SOURCE_DIR}/platform/Json.cpp
    ${PROJECT_SOURCE_DIR}/platform/ModSettings.cpp ${PROJECT_SOURCE_DIR}/platform/Text.cpp)
  target_include_directories(bejeweled_verify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/headless/shim
    ${CMAKE_CURRENT_SOURCE_DIR}/src/headless ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/framework
    ${PROJECT_SOURCE_DIR})
  if(NOT MSVC)
    target_compile_options(bejeweled_verify PRIVATE -Wall -Wno-unused-private-field)
    # Two framework idioms the framework library builds without -Wall (framework/CMakeLists.txt)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/framework/Buffer.cpp ${PROJECT_SOURCE_DIR}/framework/Color.cpp
      TARGET_DIRECTORY bejeweled_verify PROPERTIES COMPILE_OPTIONS "-Wno-parentheses;-Wno-reorder-ctor")
  endif()
  # The board plays as the game's build does: the same variant switch, the same floating point (framework/CMakeLists.txt)
  if(BEJ_NO_VARIANTS)
    target_compile_definitions(bejeweled_verify PRIVATE BEJ_NO_VARIANTS)
  endif()
  if((NOT EMSCRIPTEN) AND (NOT MSVC) AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$"))
    target_compile_options(bejeweled_verify PRIVATE -msse2 -mfpmath=sse)
  endif()
  if(NOT EMSCRIPTEN)
    # cascadehunt writes replay files: the native verifier only
    target_sources(bejeweled_verify PRIVATE src/headless/CascadeHunt.cpp)
  endif()
  if(EMSCRIPTEN)
    # Out-of-range double-to-int conversions as on x86 and in the web game (bejeweled above)
    target_compile_options(bejeweled_verify PRIVATE -mno-nontrapping-fptoint)
    set_target_properties(bejeweled_verify PROPERTIES SUFFIX ".wasm")
    target_link_options(bejeweled_verify PRIVATE "-sSTANDALONE_WASM" "--no-entry" "-sFILESYSTEM=0"
      "-sALLOW_MEMORY_GROWTH=1" "-sSTACK_SIZE=1048576")
  else()
    add_custom_command(TARGET bejeweled_verify POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweled_verify>
      VERBATIM)
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
