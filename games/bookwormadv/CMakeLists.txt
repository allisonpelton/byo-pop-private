# Bookworm Adventures Deluxe (PopCap, 2006; games/bookwormadv). Most of its game is PopLua script: the
# game's own scripts\*.luc, which tools/stage_bookwormadv_assets.py stages into assets/bookwormadv/ with the
# rest of the user's game files. The port is the engine they run on. poplua/ is PopCap's fork of Lua 5.0.2
# (docs/POPLUA.md), kept free of game code so that other PopLua games can use it.
file(GLOB POPLUA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/poplua/*.cpp)
add_library(poplua STATIC ${POPLUA_SOURCES})
target_include_directories(poplua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/poplua)
if(NOT MSVC)
  target_compile_options(poplua PRIVATE -Wall)
endif()
# PopLua's errors are C++ exceptions, as in the original
if(EMSCRIPTEN)
  target_compile_options(poplua PUBLIC -fexceptions)
  target_link_options(poplua PUBLIC -fexceptions)
endif()

# bwa_luarun: PopLua without the game, headless. It loads every compiled script of the staged game files and
# runs the ones named on its command line (tools/luarun/luarun.cpp).
if(NOT EMSCRIPTEN)
  add_executable(bwa_luarun tools/luarun/luarun.cpp)
  target_link_libraries(bwa_luarun PRIVATE poplua)
  add_custom_command(TARGET bwa_luarun POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bwa_luarun>
    VERBATIM)
endif()

# The engine the scripts run on, kept free of game code: lualib/ (the original's LuaLib: LuaSupport and the
# class bindings) and luaapp/ (PopLua's framework bindings, the Lua widgets and LuaApp, a SexyApp that runs a
# game written in Lua).
add_library(bwa_lualib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/lualib/LuaSupport.cpp)
target_include_directories(bwa_lualib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lualib)
target_link_libraries(bwa_lualib PUBLIC poplua)
file(GLOB LUAAPP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/luaapp/*.cpp)
add_library(bwa_luaapp STATIC ${LUAAPP_SOURCES})
target_include_directories(bwa_luaapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/luaapp)
target_link_libraries(bwa_luaapp PUBLIC bwa_lualib sexyframework)
if(NOT MSVC)
  target_compile_options(bwa_lualib PRIVATE -Wall)
  # The Lua widgets' setters take Lua handles under their framework names (SetFont, SetChecked, ...), as the
  # original's did: they hide the framework's overloads on purpose, which the framework calls through its own
  # classes anyway
  target_compile_options(bwa_luaapp PRIVATE -Wall -Wno-overloaded-virtual)
endif()

# bwa_popanim: Sexy::PopAnim, PopCap's player for .pam animations (popanim/), kept free of game code.
# bwa_pamtest loads every animation of the staged game files and draws a frame of any of them to a PNG,
# headless (tools/pamtest/pamtest.cpp).
file(GLOB POPANIM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/popanim/*.cpp)
add_library(bwa_popanim STATIC ${POPANIM_SOURCES})
target_include_directories(bwa_popanim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/popanim)
target_link_libraries(bwa_popanim PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bwa_popanim PRIVATE -Wall)
endif()
if(NOT EMSCRIPTEN)
  add_executable(bwa_pamtest tools/pamtest/pamtest.cpp)
  target_link_libraries(bwa_pamtest PRIVATE bwa_popanim)
  add_custom_command(TARGET bwa_pamtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bwa_pamtest>
    VERBATIM)
endif()

# bookwormadv: the game (src/: its C++ classes, which the scripts drive; jefflib/: the PopCap Lua games' helper
# library, kept free of game code). Native: bookwormadv.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp);
# it finds assets/bookwormadv/ by itself.
file(GLOB BWA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB JEFFLIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/jefflib/*.cpp)
add_library(bwa_jefflib STATIC ${JEFFLIB_SOURCES})
target_include_directories(bwa_jefflib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jefflib)
target_link_libraries(bwa_jefflib PUBLIC sexyframework)
add_executable(bookwormadv ${BWA_SOURCES})
target_include_directories(bookwormadv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bookwormadv PRIVATE bwa_luaapp bwa_popanim bwa_jefflib sexyframework)
if(NOT MSVC)
  target_compile_options(bwa_jefflib PRIVATE -Wall)
  target_compile_options(bookwormadv PRIVATE -Wall -Wno-overloaded-virtual)
endif()
if(NOT EMSCRIPTEN)
  add_custom_command(TARGET bookwormadv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bookwormadv>
    VERBATIM)
endif()
