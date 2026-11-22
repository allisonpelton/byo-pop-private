# Vendored third-party code, built as static libraries for both native and Emscripten targets.

# stb (public domain / MIT): image decoding and Ogg Vorbis decoding
add_library(stb STATIC stb/stb_impl.c)
target_include_directories(stb PUBLIC stb)
if(NOT MSVC)
  target_compile_options(stb PRIVATE -w)
endif()

# IJG libjpeg 6b (libjpeg/README; libjpeg/BYO-POP.md): JPEG decoding, the library the games' own exes decode their
# .jpg images with (PopCap's ImageLib), so pixels match the originals; stb_image rounds some of them differently. Only
# the decoder's sources, unchanged, with jmemnobs.c (no temporary files) and our jconfig.h. Platform::DecodeJPEG
# (platform/JPEGDecoder.cpp) is its one caller. Private to the framework: its headers are not for game code.
add_library(ijgjpeg STATIC
  libjpeg/jcomapi.c libjpeg/jutils.c libjpeg/jerror.c libjpeg/jmemmgr.c libjpeg/jmemnobs.c
  libjpeg/jdapimin.c libjpeg/jdapistd.c libjpeg/jdatasrc.c libjpeg/jdmaster.c libjpeg/jdinput.c libjpeg/jdmarker.c
  libjpeg/jdhuff.c libjpeg/jdphuff.c libjpeg/jdmainct.c libjpeg/jdcoefct.c libjpeg/jdpostct.c libjpeg/jddctmgr.c
  libjpeg/jidctint.c libjpeg/jidctfst.c libjpeg/jidctflt.c libjpeg/jidctred.c libjpeg/jdsample.c libjpeg/jdcolor.c
  libjpeg/jquant1.c libjpeg/jquant2.c libjpeg/jdmerge.c)
target_include_directories(ijgjpeg PUBLIC libjpeg)
if(NOT MSVC)
  target_compile_options(ijgjpeg PRIVATE -w)
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

# libopenmpt 0.8.9 (BSD-3-Clause, libopenmpt/LICENSE; minimp3 CC0): MO3 music, only for games that link
# sexyopenmpt (platform/openmpt). Sources as in its Makefile (COMMON, SOUNDLIB and LIBOPENMPT sources),
# with minimp3 (below) for MP3-compressed samples. Vorbis-compressed samples use the stb_vorbis implementation of
# the stb library above (libopenmpt includes stb_vorbis.c header-only, common/BuildSettings.h).
set(OPENMPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libopenmpt)
file(GLOB OPENMPT_SOURCES
  ${OPENMPT_DIR}/src/openmpt/all/*.cpp ${OPENMPT_DIR}/src/openmpt/base/*.cpp
  ${OPENMPT_DIR}/src/openmpt/logging/*.cpp ${OPENMPT_DIR}/src/openmpt/random/*.cpp ${OPENMPT_DIR}/common/*.cpp
  ${OPENMPT_DIR}/src/openmpt/fileformat_base/*.cpp ${OPENMPT_DIR}/src/openmpt/soundbase/*.cpp
  ${OPENMPT_DIR}/src/openmpt/soundfile_data/*.cpp ${OPENMPT_DIR}/soundlib/*.cpp ${OPENMPT_DIR}/soundlib/plugins/*.cpp
  ${OPENMPT_DIR}/soundlib/plugins/dmo/*.cpp ${OPENMPT_DIR}/sounddsp/*.cpp ${OPENMPT_DIR}/libopenmpt/*.cpp)
# A native exe plays only MO3 and OXM through libopenmpt (platform/openmpt/OpenMPTPlayer.cpp; docs/BUILDS.md, "Trimming an exe"):
# ReadMO3 builds the module itself and Bonnie's OXM is read by the XM loader, so the other formats' loaders are not compiled; the stubs of
# openmpt-mo3/loader_stubs.cpp give the format table (Sndfile.cpp) the symbols it names. The web build keeps them all: its music
# player (web/music/openmpt_web.c) plays every format libopenmpt reads.
if(NOT EMSCRIPTEN)
  file(GLOB OPENMPT_LOADER_SOURCES ${OPENMPT_DIR}/soundlib/Load_*.cpp ${OPENMPT_DIR}/soundlib/load_*.cpp)
  # (Load_mo3.cpp and Load_xm.cpp are the ones needed; Load_it, Load_dmf, Load_ams and Load_s3m hold code ReadMO3 calls)
  list(REMOVE_ITEM OPENMPT_LOADER_SOURCES ${OPENMPT_DIR}/soundlib/Load_mo3.cpp ${OPENMPT_DIR}/soundlib/Load_xm.cpp ${OPENMPT_DIR}/soundlib/Load_it.cpp
    ${OPENMPT_DIR}/soundlib/Load_dmf.cpp ${OPENMPT_DIR}/soundlib/Load_ams.cpp ${OPENMPT_DIR}/soundlib/Load_s3m.cpp)
  list(REMOVE_ITEM OPENMPT_SOURCES ${OPENMPT_LOADER_SOURCES})
  list(APPEND OPENMPT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/openmpt-mo3/loader_stubs.cpp)
endif()
# minimp3 (CC0, libopenmpt/include/minimp3/LICENSE), which came with libopenmpt, is its own library: libopenmpt decodes
# MP3 samples with it and the framework MP3 sounds (platform/Audio.cpp, Mummy Maze), so both share one copy.
add_library(minimp3 STATIC ${OPENMPT_DIR}/include/minimp3/minimp3.c)
target_include_directories(minimp3 PUBLIC ${OPENMPT_DIR}/include/minimp3)
if(NOT MSVC)
  target_compile_options(minimp3 PRIVATE -w)
endif()
add_library(openmpt STATIC ${OPENMPT_SOURCES})
target_include_directories(openmpt PUBLIC ${OPENMPT_DIR})
target_include_directories(openmpt PRIVATE ${OPENMPT_DIR}/src ${OPENMPT_DIR}/common ${OPENMPT_DIR}/include)
target_compile_definitions(openmpt PRIVATE LIBOPENMPT_BUILD MPT_WITH_MINIMP3 MPT_WITH_STBVORBIS
  STB_VORBIS_NO_PULLDATA_API STB_VORBIS_NO_STDIO)
target_link_libraries(openmpt PRIVATE stb minimp3)
if(NOT MSVC)
  target_compile_options(openmpt PRIVATE -w)
endif()
if(EMSCRIPTEN)
  # libopenmpt reports load errors with C++ exceptions (as its own Emscripten build does)
  target_compile_options(openmpt PRIVATE "-sDISABLE_EXCEPTION_CATCHING=0")
  target_link_options(openmpt INTERFACE "-sDISABLE_EXCEPTION_CATCHING=0")
endif()

# OpenJPEG 2.5.4 (BSD-2-Clause, openjpeg/LICENSE; openjpeg/BYO-POP.md): JPEG 2000 decoding, for the games whose images
# are JPEG 2000 (Bookworm Adventures and later ones). The sources of src/lib/openjp2 as OpenJPEG's CMake lists them
# (without JPIP); byopop/ holds the two config headers that CMake would generate. No MUTEX_* define: single-threaded.
set(OPENJPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg)
file(GLOB OPENJPEG_SOURCES ${OPENJPEG_DIR}/src/lib/openjp2/*.c)
add_library(openjp2 STATIC ${OPENJPEG_SOURCES})
target_include_directories(openjp2 PUBLIC ${OPENJPEG_DIR}/src/lib/openjp2 ${OPENJPEG_DIR}/byopop)
target_compile_definitions(openjp2 PUBLIC OPJ_STATIC)
if(NOT MSVC)
  target_compile_options(openjp2 PRIVATE -w)
endif()

# libtheora 1.1.1 (Xiph.Org, BSD-style, libtheora/COPYING; libtheora/README.md): the Theora video decoder only, without
# its x86 assembly or the old API's wrappers, for Sprout's second-generation engine (engines/sprout2), whose animations
# are Theora packets. include/ogg/ogg.h is ours: the libogg types the decoder uses, no libogg; a file that includes it
# never includes Tremor's.
add_library(theoradec STATIC
  libtheora/lib/bitpack.c libtheora/lib/decinfo.c libtheora/lib/decode.c libtheora/lib/dequant.c
  libtheora/lib/fragment.c libtheora/lib/huffdec.c libtheora/lib/idct.c libtheora/lib/info.c libtheora/lib/internal.c
  libtheora/lib/quant.c libtheora/lib/state.c)
target_include_directories(theoradec PUBLIC libtheora/include)
target_include_directories(theoradec PRIVATE libtheora/lib)
if(NOT MSVC)
  target_compile_options(theoradec PRIVATE -w)
endif()

# TinySoundFont (MIT, tinysoundfont/LICENSE; tinysoundfont/BYO-POP.md): a SoundFont synthesizer (tsf.h) and MIDI file
# loader (tml.h), for the games whose music is MIDI (TipTop), with a General MIDI bank cut down from GeneralUser GS
# (generaluser-gs/). Single headers: the game source that uses them defines TSF_IMPLEMENTATION and TML_IMPLEMENTATION.
add_library(tinysoundfont INTERFACE)
target_include_directories(tinysoundfont INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinysoundfont)
