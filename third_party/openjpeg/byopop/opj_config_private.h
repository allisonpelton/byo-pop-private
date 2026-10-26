/* byo-pop: what OpenJPEG's CMake makes from src/lib/openjp2/opj_config_private.h.cmake.in (BYO-POP.md), for the
   two targets built here: Windows with llvm-mingw (ucrt) and Emscripten. Both are little-endian, and neither needs
   the large-file defines or fseeko (the games decode images from memory). */

#define OPJ_PACKAGE_VERSION "2.5.4"

/* the aligned allocator opj_malloc.c uses: the CRT's _aligned_malloc on Windows, posix_memalign elsewhere */
#if defined(_WIN32)
#define OPJ_HAVE_MALLOC_H
#define OPJ_HAVE__ALIGNED_MALLOC
#else
#define OPJ_HAVE_POSIX_MEMALIGN
#endif

#if !defined(_POSIX_C_SOURCE)
#if defined(OPJ_HAVE_FSEEKO) || defined(OPJ_HAVE_POSIX_MEMALIGN)
/* Get declarations of fseeko, ftello, posix_memalign. */
#define _POSIX_C_SOURCE 200112L
#endif
#endif

/* Byte order: little-endian (x86-64, wasm32), so OPJ_BIG_ENDIAN stays undefined. */
