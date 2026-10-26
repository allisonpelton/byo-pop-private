# OpenJPEG (JPEG 2000 images)

OpenJPEG 2.5.4 from `https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.4.tar.gz` (2,157,028 bytes,
SHA-256 `a695fbe19c0165f295a8531b1e4e855cd94d0875d2f88ec4b61080677e27188a`), downloaded on 2026-10-25 with the
user's approval. BSD-2-Clause (`LICENSE`).

Copied unchanged from the package: `LICENSE`, `README.md`, `AUTHORS.md` and the library's sources in
`src/lib/openjp2/`. Left out: the JPIP index writers (`cidx_manager`, `*_manager.c`, `indexbox_manager.h`, only
built with `BUILD_JPIP`), the benchmark and test programs (`bench_dwt.c`, `test_sparse_array.c`), the table
generators (`t1_generate_luts.c`, `t1_ht_generate_luts.c`; their output `t1_luts.h` and `t1_ht_luts.h` is kept), the
command-line tools, the other libraries and the build system. `third_party/CMakeLists.txt` builds the library
(`openjp2`) from the same source list as OpenJPEG's own CMake.

`byopop/opj_config.h` and `byopop/opj_config_private.h` are ours: they stand in for the two headers OpenJPEG's CMake
generates from `*.cmake.in`, for the Windows (llvm-mingw) and Emscripten builds. No `MUTEX_*` is defined, so
`thread.c` uses its single-threaded stubs and decoding is the same on both.

It decodes the JPEG 2000 images of the games that have them: both Bookworm Adventures games, the Peggle games,
Bejeweled 3, Bejeweled Twist and Zuma's Revenge (docs/POPCAP_GAMES_SURVEY.md, G5). Those games' web pages decode
them at run time from the player's imported files, so the library is in the game, not only in the asset staging.
