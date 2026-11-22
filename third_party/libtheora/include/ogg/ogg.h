/* byo-pop: the few libogg names libtheora's decoder uses, its types and its allocation macros, and nothing else (see
   ../../README.md). It is not libogg: no function of libogg is declared or defined here, and the decoder calls none.
   Tremor's ogg.h (third_party/tremor) defines an ogg_packet of another layout, so no file includes both headers (one
   that did would not compile: the two ogg_packet typedefs clash), and this struct's tag is libtheora's own, so the two
   types never share a name for linkage. */
#if !defined(BYOPOP_THEORA_OGG_H)
#define BYOPOP_THEORA_OGG_H

#include <stdint.h>
#include <stdlib.h>

/* As libogg's os_types.h has them */
#define _ogg_malloc malloc
#define _ogg_calloc calloc
#define _ogg_realloc realloc
#define _ogg_free free

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;

/* One packet of a logical stream, with the fields libogg's ogg_packet has, in its order */
typedef struct theora_ogg_packet {
  unsigned char *packet;  /* the packet's bytes */
  long bytes;             /* how many */
  long b_o_s;             /* nonzero for a stream's first packet */
  long e_o_s;             /* nonzero for its last */
  ogg_int64_t granulepos; /* the stream's position after this packet, -1 if unknown */
  ogg_int64_t packetno;   /* the packet's number in its stream */
} ogg_packet;

#endif
