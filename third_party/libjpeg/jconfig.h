/* jconfig.h for byo-pop (ours; see BYO-POP.md). The settings `configure` would pick for a plain ANSI C compiler,
 * as in the jconfig.h that PopCap's SexyAppFramework ships with its copy of libjpeg 6b. Nothing here changes what the
 * decoder computes: it only says which C features the compilers (llvm-mingw, Emscripten) have. */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
/* Define this if you get warnings about undefined structures. */
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED
#define INLINE
/* These are for configuring the JPEG memory manager (jmemnobs.c: no temporary files, no memory limit). */
#undef DEFAULT_MAX_MEM
#undef NO_MKTEMP

#endif /* JPEG_INTERNALS */
