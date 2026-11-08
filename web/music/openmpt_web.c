/* The site's music player's decoder (web/music.js; docs/WEB_PLAYER.md, "The music player"): libopenmpt on its own, built for
   the web as openmpt.js and openmpt.wasm beside the pages. It plays the module files the player imported with a game (.mo3,
   .it, .s3m, .xm, .oxm); a module's subsongs are its tunes, or ranges of its orders where a game plays it so (Bonnie's
   Bookstore). It also decodes Ogg Vorbis with stb_vorbis, for a browser whose decodeAudioData cannot. No game code and no game
   files are in it. */
#include <stdlib.h>
#include <emscripten.h>
#include "libopenmpt/libopenmpt.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

/* A module from the bytes at data (copied by libopenmpt), or 0 when they are not one */
EMSCRIPTEN_KEEPALIVE openmpt_module* bm_load(const void* data, int size) {
  return openmpt_module_create_from_memory2(data, (size_t)size, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}
EMSCRIPTEN_KEEPALIVE void bm_free(openmpt_module* m) { openmpt_module_destroy(m); }
EMSCRIPTEN_KEEPALIVE int bm_subsongs(openmpt_module* m) { return openmpt_module_get_num_subsongs(m); }
/* Plays subsong i from its start; -1 plays them all as one */
EMSCRIPTEN_KEEPALIVE int bm_select(openmpt_module* m, int i) { return openmpt_module_select_subsong(m, i); }
/* loop: the tune repeats for ever; else it ends (bm_render returns fewer frames than asked, then 0) */
EMSCRIPTEN_KEEPALIVE void bm_loop(openmpt_module* m, int loop) { openmpt_module_set_repeat_count(m, loop ? -1 : 0); }
/* The selected subsong's length in seconds, once through */
EMSCRIPTEN_KEEPALIVE double bm_duration(openmpt_module* m) { return openmpt_module_get_duration_seconds(m); }
EMSCRIPTEN_KEEPALIVE double bm_position(openmpt_module* m) { return openmpt_module_get_position_seconds(m); }
/* Plays on from the start of order theOrder; the time of that point in seconds */
EMSCRIPTEN_KEEPALIVE double bm_set_order(openmpt_module* m, int order) { return openmpt_module_set_position_order_row(m, order, 0); }
EMSCRIPTEN_KEEPALIVE int bm_order(openmpt_module* m) { return openmpt_module_get_current_order(m); }
EMSCRIPTEN_KEEPALIVE int bm_orders(openmpt_module* m) { return openmpt_module_get_num_orders(m); }
/* An Ogg Vorbis file decoded whole: 16-bit interleaved samples at *out (free them with free()), *channels and *rate set; the
   frames, or -1 when it is not one */
EMSCRIPTEN_KEEPALIVE int bm_ogg(const unsigned char* data, int size, int* channels, int* rate, short** out) {
  return stb_vorbis_decode_memory(data, size, channels, rate, out);
}
/* frames of interleaved stereo float at rate into out; returns the frames written */
EMSCRIPTEN_KEEPALIVE int bm_render(openmpt_module* m, int rate, int frames, float* out) {
  return (int)openmpt_module_read_interleaved_float_stereo(m, rate, (size_t)frames, out);
}
