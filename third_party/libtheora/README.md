# libtheora 1.1.1 (Theora video decoder)

Xiph.Org's libtheora, release 1.1.1 ("Thusnelda"), from `https://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz`
(2,111,877 bytes, SHA-256 `40952956c47811928d1e7922cda3bc1f427eb75680c3c37249c91e949054916b`), downloaded on 2026-11-16
with the user's approval ("ok to download libtheora", 2026-11-16). BSD-style license in `COPYING`;
`web/licenses/libtheora.txt` carries it to the copyright pages of the games that link it.

Sprout's second-generation engine (`engines/sprout2`: Feeding Frenzy 2, Talismania) keeps its animations as Theora
video, one `.theora` file per animation. The file is not an Ogg stream: it is the stream's packets one after another,
each behind a 28-byte header holding the packet's fields (bytes, b_o_s, e_o_s as 32-bit numbers, granulepos and
packetno as 64-bit ones), so the engine reads the packets itself and needs no Ogg framing. The streams were made by the
2004 encoder ("Xiph.Org libTheora I 20040317", bitstream 3.2.0); Theora's decoding is defined to the bit, so any
conforming decoder gives the same pictures.

## What is here

Only the decoder, built without its x86 assembly (the C code gives the same output):

- `include/theora/`: `codec.h`, `theoradec.h` and `theora.h`, unchanged. `theora.h` is the old API's header, which
  `lib/internal.h` includes; nothing here implements that API.
- `lib/`: the decoder's sources from `lib/Makefile.am` (`decoder_sources`) without the old API's wrappers
  (`apiwrapper.c`, `decapiwrapper.c`) and the x86 folders: `bitpack.c`, `decinfo.c`, `decode.c`, `dequant.c`,
  `fragment.c`, `huffdec.c`, `idct.c`, `info.c`, `internal.c`, `quant.c`, `state.c`, with the headers they include,
  all unchanged.
- `include/ogg/ogg.h`: ours, not libogg's. The decoder uses nothing of libogg but its integer types, its
  `ogg_packet` struct and the allocation macros of its `os_types.h` (`_ogg_malloc` and the rest, the C library's), so
  this header supplies those and nothing else. Every game links Tremor (`../tremor`), whose `ogg.h` defines libogg's
  function names over its own low-memory framing and an `ogg_packet` of another layout: real libogg would collide with
  it, and this header must never be included in a file that includes Tremor's (it would not compile).

The built library needs only the C library and defines only `th_` and `oc_` names.

## Changes

None to Xiph.Org's files. The one file of ours is `include/ogg/ogg.h`, above.
