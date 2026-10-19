# Vendored third-party code, built as static libraries for both native and Emscripten targets.

# stb (public domain / MIT): image decoding and Ogg Vorbis decoding
add_library(stb STATIC stb/stb_impl.c)
target_include_directories(stb PUBLIC stb)
if(NOT MSVC)
  target_compile_options(stb PRIVATE -w)
endif()

# libxmp-lite 4.7.4 (MIT): MOD/S3M/XM/IT module playback for the game's .s3m music.
# Use the library's own source list so platform shims (snprintf replacements etc.) are included.
include(libxmp-lite/cmake/libxmp-sources.cmake)
list(TRANSFORM LIBXMP_SRC_LIST PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/libxmp-lite/")
add_library(xmp_lite STATIC ${LIBXMP_SRC_LIST})
target_include_directories(xmp_lite PUBLIC libxmp-lite/include/libxmp-lite)
target_include_directories(xmp_lite PRIVATE libxmp-lite/src)
target_compile_definitions(xmp_lite PUBLIC LIBXMP_STATIC)
target_compile_definitions(xmp_lite PRIVATE LIBXMP_CORE_PLAYER _USE_MATH_DEFINES)
if(NOT MSVC)
  target_compile_options(xmp_lite PRIVATE -w)
endif()

# Tremor (Xiph.Org, BSD-style, see tremor/COPYING): the 2002 integer Ogg Vorbis decoder that
# SexyAppFramework 1.2 shipped and decoded sounds with. Platform::Mixer uses it for Ogg files stb_vorbis
# cannot open: Big Money's 2002 sounds use Vorbis floor 0, which stb_vorbis does not support.
file(GLOB TREMOR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tremor/*.c)
add_library(tremor STATIC ${TREMOR_SOURCES})
target_include_directories(tremor PUBLIC tremor)
if(NOT MSVC)
  # misc.h calls abs() without <stdlib.h> (an error for C99 compilers such as Emscripten's clang)
  target_compile_options(tremor PRIVATE -w -include stdlib.h)
endif()
