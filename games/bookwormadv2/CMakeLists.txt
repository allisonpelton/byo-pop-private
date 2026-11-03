# Bookworm Adventures Volume 2 (PopCap, 2009; games/bookwormadv2). Most of its game is PopLua script: the
# game's own scripts\*.luc, which tools/stage_bookwormadv2_assets.py stages into assets/bookwormadv2/ with the
# rest of the user's game files. The port is the engine they run on. poplua/ is PopCap's fork of Lua 5.0.2
# (docs/POPLUA.md), kept free of game code so that other PopLua games can use it.
file(GLOB POPLUA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/poplua/*.cpp)
add_library(bwa2_poplua STATIC ${POPLUA_SOURCES})
target_include_directories(bwa2_poplua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/poplua)
if(NOT MSVC)
  target_compile_options(bwa2_poplua PRIVATE -Wall)
endif()
# PopLua's errors are C++ exceptions, as in the original
if(EMSCRIPTEN)
  target_compile_options(bwa2_poplua PUBLIC -fexceptions)
  target_link_options(bwa2_poplua PUBLIC -fexceptions)
endif()

# bwa2_luarun: PopLua without the game, headless. It loads every compiled script of the staged game files and
# runs the ones named on its command line (tools/luarun/luarun.cpp).
if(NOT EMSCRIPTEN)
  add_executable(bwa2_luarun tools/luarun/luarun.cpp)
  target_link_libraries(bwa2_luarun PRIVATE bwa2_poplua)
  add_custom_command(TARGET bwa2_luarun POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bwa2_luarun>
    VERBATIM)
endif()

# The engine the scripts run on, kept free of game code: lualib/ (the original's LuaLib: LuaSupport and the
# class bindings) and luaapp/ (PopLua's framework bindings, the Lua widgets and LuaApp, a SexyApp that runs a
# game written in Lua).
add_library(bwa2_lualib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lualib/LuaSupport.cpp)
target_include_directories(bwa2_lualib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lualib)
target_link_libraries(bwa2_lualib PUBLIC bwa2_poplua)
file(GLOB LUAAPP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/luaapp/*.cpp)
add_library(bwa2_luaapp STATIC ${LUAAPP_SOURCES})
target_include_directories(bwa2_luaapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/luaapp)
target_link_libraries(bwa2_luaapp PUBLIC bwa2_lualib sexyframework)
if(NOT MSVC)
  target_compile_options(bwa2_lualib PRIVATE -Wall)
  # The Lua widgets' setters take Lua handles under their framework names (SetFont, SetChecked, ...), as the
  # original's did: they hide the framework's overloads on purpose, which the framework calls through its own
  # classes anyway
  target_compile_options(bwa2_luaapp PRIVATE -Wall -Wno-overloaded-virtual)
endif()

# bwa2_popanim: Sexy::PopAnim, PopCap's player for .pam animations (popanim/), kept free of game code.
# bwa2_pamtest loads every animation of the staged game files and draws a frame of any of them to a PNG,
# headless (tools/pamtest/pamtest.cpp).
file(GLOB POPANIM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/popanim/*.cpp)
add_library(bwa2_popanim STATIC ${POPANIM_SOURCES})
target_include_directories(bwa2_popanim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/popanim)
target_link_libraries(bwa2_popanim PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bwa2_popanim PRIVATE -Wall)
endif()
if(NOT EMSCRIPTEN)
  add_executable(bwa2_pamtest tools/pamtest/pamtest.cpp)
  target_link_libraries(bwa2_pamtest PRIVATE bwa2_popanim)
  add_custom_command(TARGET bwa2_pamtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bwa2_pamtest>
    VERBATIM)
endif()

# bookwormadv2: the game (src/: its C++ classes, which the scripts drive; jefflib/: the PopCap Lua games' helper
# library, kept free of game code). Native: bookwormadv2.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp);
# it finds assets/bookwormadv2/ by itself. Web: bookwormadv2.html/.js/.wasm; its page (web/shell.html with web/page.js
# here) imports the player's install into /bwapersist (IDBFS), main.pak unpacked. The music is an MO3 module
# (libopenmpt via sexyopenmpt), and 42 of the pictures are JPEG 2000 (OpenJPEG via sexyopenjpeg).
file(GLOB BWA2_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB JEFFLIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/jefflib/*.cpp)
add_library(bwa2_jefflib STATIC ${JEFFLIB_SOURCES})
target_include_directories(bwa2_jefflib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jefflib)
target_link_libraries(bwa2_jefflib PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bwa2_jefflib PRIVATE -Wall)
endif()
# The game itself (src/ reworked for Volume 2; the battles are milestone 3, still in progress): always built, as the suite's
# new-player run needs the exe
add_executable(bookwormadv2 ${BWA2_SOURCES})
target_include_directories(bookwormadv2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bookwormadv2 PRIVATE bwa2_luaapp bwa2_popanim bwa2_swf bwa2_jefflib sexyframework sexyopenmpt sexyopenjpeg)
if(NOT MSVC)
  target_compile_options(bookwormadv2 PRIVATE -Wall -Wno-overloaded-virtual)
endif()
if(EMSCRIPTEN)
  byopop_web_game(bookwormadv2 TITLE "Bookworm Adventures Volume 2"
    COPYRIGHT "Bookworm Adventures Volume 2 © 2009 PopCap Games, Inc. All rights reserved." LICENSES libopenmpt lua openjpeg)
else()
  add_custom_command(TARGET bookwormadv2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bookwormadv2>
    VERBATIM)
endif()

# bwa2_swf: Sexy::SwfMovie, a small player for the Flash (.swf) movies of the story cutscenes (swf/), kept free of
# game code. bwa2_swftest plays every cutscene of the staged game files through and draws frames of any of them to
# PNGs, headless (tools/swftest/swftest.cpp).
file(GLOB SWF_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/swf/*.cpp)
add_library(bwa2_swf STATIC ${SWF_SOURCES})
target_include_directories(bwa2_swf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/swf)
target_link_libraries(bwa2_swf PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bwa2_swf PRIVATE -Wall)
endif()
if(NOT EMSCRIPTEN)
  add_executable(bwa2_swftest tools/swftest/swftest.cpp)
  target_link_libraries(bwa2_swftest PRIVATE bwa2_swf)
  add_custom_command(TARGET bwa2_swftest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bwa2_swftest>
    VERBATIM)
endif()
