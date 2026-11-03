# OpenJPEG (JPEG 2000 images)

OpenJPEG 2.5.4 from `https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.4.tar.gz` (2,157,028 bytes,
SHA-256 `a695fbe19c0165f295a8531b1e4e855cd94d0875d2f88ec4b61080677e27188a`), downloaded on 2026-10-25 with the
user's approval. BSD-2-Clause (`LICENSE`).

Copied from the package: `LICENSE`, `README.md`, `AUTHORS.md` and the library's sources in `src/lib/openjp2/`,
unchanged but for the one change below. Left out: the JPIP index writers (`cidx_manager`, `*_manager.c`, `indexbox_manager.h`, only
built with `BUILD_JPIP`), the benchmark and test programs (`bench_dwt.c`, `test_sparse_array.c`), the table
generators (`t1_generate_luts.c`, `t1_ht_generate_luts.c`; their output `t1_luts.h` and `t1_ht_luts.h` is kept), the
command-line tools, the other libraries and the build system. `third_party/CMakeLists.txt` builds the library
(`openjp2`) from the same source list as OpenJPEG's own CMake.

`byopop/opj_config.h` and `byopop/opj_config_private.h` are ours: they stand in for the two headers OpenJPEG's CMake
generates from `*.cmake.in`, for the Windows (llvm-mingw) and Emscripten builds. No `MUTEX_*` is defined, so
`thread.c` uses its single-threaded stubs and decoding is the same on both.

Our one change, in `t1.c` (each place marked "byo-pop"): a coefficient whose code-block was cut short is rebuilt at
the low end of the interval its decoded bits leave open, as the games' own decoder (PopCap's `j2k-codec.dll`) does,
not at its middle as OpenJPEG does. A significant coefficient starts at its decoded magnitude instead of that plus
half a step, and a refinement bit of 1 adds the step and 0 adds nothing, instead of adding or taking half a step. The
games' art is coded with the reversible 5/3 wavelet but with its code-blocks truncated, so this moves most of its
pixels by a level or a few; a full lossless decode, where no interval is left open, is unchanged. An image coded with
the irreversible 9/7 wavelet changes everywhere, truncated or not, since its every coefficient is quantised and so
loses its half step: Bejeweled Twist's and both Bookworm Adventures games' files are all 5/3, but the Peggle games,
Zuma's Revenge and Bejeweled 3 should check theirs against their own originals before relying on it. The evidence is
Bejeweled Twist's title, read from the original's memory after its decoder ran: with this change 65% of its pixels
are equal to ours, against 35% before, and the sparkle points the game picks from the brightest opaque pixels match
all 1299 of the original's, where before some were missing and others extra. Of the rest, most pixels are a level
or two off (87% of all are within one level, 92% within two) and a few up to 31; their cause was not found, and
Bejeweled Twist's notes list them. tools/jp2test's references are this
decoder's output (`tools/jp2test/write_refs.sh` remakes them), so they hold it to this change.

It decodes the JPEG 2000 images of the games that have them: both Bookworm Adventures games, the Peggle games,
Bejeweled 3, Bejeweled Twist and Zuma's Revenge (docs/POPCAP_GAMES_SURVEY.md, G5). Those games' web pages decode
them at run time from the player's imported files, so the library is in the game, not only in the asset staging.
