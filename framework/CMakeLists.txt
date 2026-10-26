# Portable SexyAppFramework subset (framework/) + platform layer (platform/, SDL2), one static library
# that every game links. Includes: "SexyAppBase.h" (framework/) and "platform/VFS.h" (the repository
# root, which holds framework/ and platform/). That root holds games/ too, so nothing but this rule keeps
# a game from including another game's headers: games include only framework/ and platform/ headers.
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SHARED_ROOT)
file(GLOB FRAMEWORK_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${SHARED_ROOT}/platform/*.cpp)
# Software triangle rasterizer pieces are #included by SWTri.cpp, not compiled on their own.
list(FILTER FRAMEWORK_SOURCES EXCLUDE REGEX ".*/SWTri_[^/]*\\.cpp$")

add_library(sexyframework STATIC ${FRAMEWORK_SOURCES})
target_include_directories(sexyframework PUBLIC
  ${SHARED_ROOT}
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sexyframework PUBLIC ${PORT_SDL} stb xmp_lite tremor minimp3)
if(NOT MSVC)
  # The framework predates C++11 and relies on a few now-pedantic idioms.
  target_compile_options(sexyframework PRIVATE -Wno-deprecated-declarations -Wno-switch -Wno-parentheses
    -Wno-unused-value -Wno-dangling-else -Wno-format-security -Wno-char-subscripts)
endif()
if(EMSCRIPTEN)
  # Emscripten's default 64 KB stack is too small for the framework (font descriptor parsing overflows it)
  target_link_options(sexyframework PUBLIC "-lidbfs.js" "-sSTACK_SIZE=1048576")
endif()
if((NOT EMSCRIPTEN) AND (NOT MSVC) AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$"))
  # The original computes doubles on the x87 with 53-bit precision control, which rounds like SSE2
  # doubles. x87 code at mingw's default 64-bit precision would not (it changes e.g. the gem-shrink
  # sizes), so keep floating point on SSE2 explicitly.
  target_compile_options(sexyframework PUBLIC -msse2 -mfpmath=sse)
endif()

# MO3 music through libopenmpt (platform/openmpt): a separate library, so only the games that link it
# (and call Platform::RegisterOpenMPTModules) carry libopenmpt.
add_library(sexyopenmpt STATIC ${SHARED_ROOT}/platform/openmpt/OpenMPTPlayer.cpp)
target_link_libraries(sexyopenmpt PUBLIC sexyframework PRIVATE openmpt)

# The early-2004 SexyAppFramework's behaviours (legacy dialog buttons, the 2004 Slider, the 2004 text layout,
# MTRand 1.2 seeding, MSVC rand, the repeatable game clock) that its games (BookWorm, Typer Shark, AstroPop)
# rely on: framework/compat2004, docs/FRAMEWORK_2004.md section 8. A separate library a game links to opt in.
add_library(sexycompat2004 STATIC ${CMAKE_CURRENT_SOURCE_DIR}/compat2004/Compat2004.cpp)
target_include_directories(sexycompat2004 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/compat2004)
target_link_libraries(sexycompat2004 PUBLIC sexyframework)
