# Portable SexyAppFramework subset + platform layer (SDL2).
file(GLOB FRAMEWORK_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp)
# Software triangle rasterizer pieces are #included by SWTri.cpp, not compiled on their own.
list(FILTER FRAMEWORK_SOURCES EXCLUDE REGEX ".*/SWTri_[^/]*\\.cpp$")

add_library(sexyframework STATIC ${FRAMEWORK_SOURCES})
target_include_directories(sexyframework PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/SexyAppFramework)
target_link_libraries(sexyframework PUBLIC ${PORT_SDL} stb xmp_lite)
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
