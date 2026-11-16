# libopenmpt (MO3 music)

libopenmpt 0.8.9 from `https://lib.openmpt.org/files/libopenmpt/src/libopenmpt-0.8.9+release.makefile.tar.gz`
(1,725,467 bytes, SHA-256 `9273b88b67973cc69e54d748ab1b749399d6d07695f1c37d0c59f88b4106074f`), downloaded on
2026-10-18 with the user's approval. BSD-3-Clause (`LICENSE`); the bundled minimp3 is CC0
(`include/minimp3/LICENSE`) and stb_vorbis is public domain / MIT.

Copied from the package, unchanged but for the changes below: `LICENSE`, `README.md`, `common/`, `soundlib/`, `sounddsp/`, `src/`,
`libopenmpt/*.{h,hpp,cpp}`, `build/svn_version/svn_version.h`, `include/minimp3/`, `include/stb_vorbis/`.
Left out: the tools, tests, examples, bindings, plugins (in_openmpt, xmp-openmpt), miniz and the build
system; `third_party/CMakeLists.txt` builds the library (`openmpt`) from the Makefile's source lists.

Only MO3 goes through it (the user's choice): `platform/openmpt/OpenMPTPlayer.cpp` registers it with
`Platform::Mixer` for files starting with "MO3", and games link it through the `sexyopenmpt` library.
`include/stb_vorbis/stb_vorbis.c` is only included header-only (`common/BuildSettings.h` defines
`STB_VORBIS_HEADER_ONLY`); the implementation is the one in `third_party/stb`.

## Changes

Streamed samples (2026-11-15, the user: "Merge both"): a long Ogg Vorbis sample of an MO3 can be
kept as its Vorbis data and decoded as it plays instead of decoded whole when the module loads, so that Mahjong Escape's
recorded music does not hold tens of MB of decoded samples. It is off unless `byopop_openmpt_stream_samples(1)` is called
before a module is loaded (`platform/openmpt/OpenMPTPlayer.cpp` does it for Mahjong Escape's music only). Each changed
line is marked `byo-pop`:
- `soundlib/SampleStream.{h,cpp}` (new): the kept samples. A sample is kept when it is 16-bit, has no loop or sustain
  loop, is at least 44,100 frames long and has one or two channels; its frames are decoded from the start through the
  loader's own stb_vorbis calls and conversion, so that every frame, padding included, is the one the whole sample held.
  Two decoders a sample, for a note fading out while a new one starts at another place of the same sample.
- `soundlib/Load_mo3.cpp`: the stb_vorbis branch keeps a wanted sample's header and data instead of decoding it.
- `soundlib/ModSample.cpp`: `PrecomputeLoops` leaves a kept sample's one-frame placeholder alone.
- `soundlib/Fastmix.cpp`: `MixChannel` points a channel playing a kept sample at the decoded frames its run reads.
- `soundlib/Sndfile.{h,cpp}`: the module's `m_sampleStreams`, freed first in `Destroy`.
