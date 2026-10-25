# libopenmpt (MO3 music)

libopenmpt 0.8.9 from `https://lib.openmpt.org/files/libopenmpt/src/libopenmpt-0.8.9+release.makefile.tar.gz`
(1,725,467 bytes, SHA-256 `9273b88b67973cc69e54d748ab1b749399d6d07695f1c37d0c59f88b4106074f`), downloaded on
2026-10-18 with the user's approval. BSD-3-Clause (`LICENSE`); the bundled minimp3 is CC0
(`include/minimp3/LICENSE`) and stb_vorbis is public domain / MIT.

Copied unchanged from the package: `LICENSE`, `README.md`, `common/`, `soundlib/`, `sounddsp/`, `src/`,
`libopenmpt/*.{h,hpp,cpp}`, `build/svn_version/svn_version.h`, `include/minimp3/`, `include/stb_vorbis/`.
Left out: the tools, tests, examples, bindings, plugins (in_openmpt, xmp-openmpt), miniz and the build
system; `third_party/CMakeLists.txt` builds the library (`openmpt`) from the Makefile's source lists.

Only MO3 goes through it (the user's choice): `platform/openmpt/OpenMPTPlayer.cpp` registers it with
`Platform::Mixer` for files starting with "MO3", and games link it through the `sexyopenmpt` library.
`include/stb_vorbis/stb_vorbis.c` is only included header-only (`common/BuildSettings.h` defines
`STB_VORBIS_HEADER_ONLY`); the implementation is the one in `third_party/stb`.
