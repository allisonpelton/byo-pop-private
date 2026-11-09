/* The site's music player's decoder (web/music.js; docs/WEB_PLAYER.md, "The music player"): built for the web as openmpt.js
   and openmpt.wasm beside the pages. It plays the module files the player imported with a game (.mo3, .it, .s3m, .xm, .oxm)
   by the renderer and settings the game itself plays them with (platform/MusicLevels.h): libopenmpt for MO3 and OXM, libxmp-lite
   for the rest, so that a tune is as loud as in its game. A module's subsongs are its tunes (libxmp-lite's sequences, which
   for every game's modules are libopenmpt's subsongs), or ranges of its orders where a game plays it so (Bonnie's Bookstore's
   OXM). It also decodes Ogg Vorbis with stb_vorbis, for a browser whose decodeAudioData cannot. No game code and no game files
   are in it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "libopenmpt/libopenmpt.h"
#include <xmp.h>
#include "../../platform/ModuleText.h"
#include "../../platform/MusicLevels.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

/* One module: either renderer */
typedef struct {
  openmpt_module* mpt;
  xmp_context xmp;
  int rate;       /* the rate libxmp-lite's player was started at */
  int sequence;   /* libxmp-lite's selected sequence */
  int loop;       /* libxmp-lite: repeat for ever */
  int ended;
  short buf[4096 * 2];
} bm_module;

/* An MO3, or an XM whose samples are Ogg Vorbis (an OXM), as Platform::Mixer tells them (platform/openmpt/OpenMPTPlayer.cpp) */
static int is_openmpt(const unsigned char* d, int n) {
  static const char xm[] = "Extended Module: ";
  if (n > 4 && !memcmp(d, "MO3", 3)) return 1;
  if (n < 0x150 || memcmp(d, xm, sizeof(xm) - 1)) return 0;
  for (int i = 0x150; i + 4 <= n; i++) if (!memcmp(d + i, "OggS", 4)) return 1;
  return 0;
}

static void xmp_start(bm_module* b, int rate) {
  b->rate = rate;
  xmp_start_player(b->xmp, rate, 0);
  xmp_set_player(b->xmp, XMP_PLAYER_INTERP, BYOPOP_XMP_INTERP);
}

/* A module from the bytes at data (copied), or 0 when they are not one */
EMSCRIPTEN_KEEPALIVE bm_module* bm_load(const void* data, int size) {
  bm_module* b = (bm_module*)calloc(1, sizeof(bm_module));
  if (!b) return 0;
  if (is_openmpt((const unsigned char*)data, size)) {
    b->mpt = openmpt_module_create_from_memory2(data, (size_t)size, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!b->mpt) { free(b); return 0; }
    openmpt_module_set_render_param(b->mpt, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, BYOPOP_OPENMPT_INTERPOLATION);
    openmpt_module_set_render_param(b->mpt, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, BYOPOP_OPENMPT_SEPARATION);
    return b;
  }
  b->xmp = xmp_create_context();
  if (xmp_load_module_from_memory(b->xmp, data, size) != 0) { xmp_free_context(b->xmp); free(b); return 0; }
  xmp_start(b, 48000);
  return b;
}
EMSCRIPTEN_KEEPALIVE void bm_free(bm_module* b) {
  if (b->mpt) openmpt_module_destroy(b->mpt);
  if (b->xmp) { xmp_end_player(b->xmp); xmp_release_module(b->xmp); xmp_free_context(b->xmp); }
  free(b);
}
static struct xmp_module_info info(bm_module* b) { struct xmp_module_info mi; xmp_get_module_info(b->xmp, &mi); return mi; }
EMSCRIPTEN_KEEPALIVE int bm_subsongs(bm_module* b) { return b->mpt ? openmpt_module_get_num_subsongs(b->mpt) : info(b).num_sequences; }
/* Plays subsong i from its start; -1 plays them all as one (libopenmpt only) */
EMSCRIPTEN_KEEPALIVE int bm_select(bm_module* b, int i) {
  if (b->mpt) return openmpt_module_select_subsong(b->mpt, i);
  struct xmp_module_info mi = info(b);
  if (i < 0 || i >= mi.num_sequences) i = 0;
  b->sequence = i;
  b->ended = 0;
  xmp_restart_module(b->xmp);
  xmp_set_position(b->xmp, mi.seq_data[i].entry_point);
  return 1;
}
/* loop: the tune repeats for ever; else it ends (bm_render returns fewer frames than asked, then 0) */
EMSCRIPTEN_KEEPALIVE void bm_loop(bm_module* b, int loop) {
  if (b->mpt) openmpt_module_set_repeat_count(b->mpt, loop ? -1 : 0);
  else b->loop = loop;
}
/* The selected subsong's length in seconds, once through */
EMSCRIPTEN_KEEPALIVE double bm_duration(bm_module* b) {
  if (b->mpt) return openmpt_module_get_duration_seconds(b->mpt);
  struct xmp_module_info mi = info(b);
  return mi.num_sequences ? mi.seq_data[b->sequence].duration / 1000.0 : 0;
}
EMSCRIPTEN_KEEPALIVE double bm_position(bm_module* b) {
  if (b->mpt) return openmpt_module_get_position_seconds(b->mpt);
  struct xmp_frame_info fi;
  xmp_get_frame_info(b->xmp, &fi);
  return fi.time / 1000.0;
}
/* Plays on from the start of order theOrder; the time of that point in seconds (libopenmpt: a game's ranges of orders are
   only in Bonnie's Bookstore's OXM; libxmp-lite answers its position before the move) */
EMSCRIPTEN_KEEPALIVE double bm_set_order(bm_module* b, int order) {
  if (b->mpt) return openmpt_module_set_position_order_row(b->mpt, order, 0);
  b->ended = 0;
  xmp_set_position(b->xmp, order);
  return bm_position(b);
}
EMSCRIPTEN_KEEPALIVE int bm_order(bm_module* b) {
  if (b->mpt) return openmpt_module_get_current_order(b->mpt);
  struct xmp_frame_info fi;
  xmp_get_frame_info(b->xmp, &fi);
  return fi.pos;
}
EMSCRIPTEN_KEEPALIVE int bm_orders(bm_module* b) { return b->mpt ? openmpt_module_get_num_orders(b->mpt) : info(b).mod->len; }
/* An Ogg Vorbis file decoded whole: 16-bit interleaved samples at *out (free them with free()), *channels and *rate set; the
   frames, or -1 when it is not one */
EMSCRIPTEN_KEEPALIVE int bm_ogg(const unsigned char* data, int size, int* channels, int* rate, short** out) {
  return stb_vorbis_decode_memory(data, size, channels, rate, out);
}
/* frames of interleaved stereo float at rate into out; returns the frames written */
EMSCRIPTEN_KEEPALIVE int bm_render(bm_module* b, int rate, int frames, float* out) {
  if (b->mpt) return (int)openmpt_module_read_interleaved_float_stereo(b->mpt, rate, (size_t)frames, out);
  if (b->ended) return 0;
  if (rate != b->rate) {
    /* the player's rate: libxmp-lite starts again at it, from the order it was at */
    int order = bm_order(b);
    xmp_end_player(b->xmp);
    xmp_start(b, rate);
    xmp_set_position(b->xmp, order);
  }
  int done = 0;
  while (done < frames) {
    int n = frames - done > 4096 ? 4096 : frames - done;
    if (xmp_play_buffer(b->xmp, b->buf, n * 4, b->loop ? 0 : 1) != 0) { b->ended = 1; break; }
    for (int i = 0; i < n * 2; i++) out[done * 2 + i] = b->buf[i] / 32768.0f;
    done += n;
  }
  return done;
}

/* The game's own music level (platform/MusicLevels.h), for a tune of web/soundtracks.js's "level": a BASS amplify (0 to
   100) as the gain it plays at, and FMOD 3's XM pre-amp */
EMSCRIPTEN_KEEPALIVE double bm_level_bass_amplify(int amp) { return ByopopBassAmplifyGain(amp); }
EMSCRIPTEN_KEEPALIVE double bm_level_fmod3_xm(void) { return BYOPOP_FMOD3_XM_GAIN; }

/* ---------- the music player's tracker view and seek bar (web/music.js; the user, 2026-11-08) ---------- */
/* Where the decoder is, into out[0..4]: the pattern, its row, the pattern's rows, the module's channels, the order. The player
   asks only while its tracker view is shown, between slices of what it renders, so a closed view costs nothing. */
EMSCRIPTEN_KEEPALIVE void bm_view(bm_module* b, int* out) {
  if (b->mpt) {
    int pattern = openmpt_module_get_current_pattern(b->mpt);
    out[0] = pattern;
    out[1] = openmpt_module_get_current_row(b->mpt);
    out[2] = openmpt_module_get_pattern_num_rows(b->mpt, pattern);
    out[3] = openmpt_module_get_num_channels(b->mpt);
    out[4] = openmpt_module_get_current_order(b->mpt);
    return;
  }
  struct xmp_frame_info fi;
  xmp_get_frame_info(b->xmp, &fi);
  out[0] = fi.pattern;
  out[1] = fi.row;
  out[2] = fi.num_rows;
  out[3] = info(b).mod->chn;
  out[4] = fi.pos;
}

/* Which channels play a note anywhere in the module: out[c] is 1 for each that does, 0 for one that never does (up to max
   channels); returns the module's channels. The tracker view leaves the silent ones out. Read once a tune, when its view is first
   drawn. */
EMSCRIPTEN_KEEPALIVE int bm_used(bm_module* b, unsigned char* out, int max) {
  if (b->mpt) {
    int channels = openmpt_module_get_num_channels(b->mpt), patterns = openmpt_module_get_num_patterns(b->mpt);
    for (int c = 0; c < channels && c < max; c++) out[c] = 0;
    for (int p = 0; p < patterns; p++) {
      int rows = openmpt_module_get_pattern_num_rows(b->mpt, p);
      for (int r = 0; r < rows; r++)
        for (int c = 0; c < channels && c < max; c++) {
          if (out[c]) continue;
          uint8_t note = openmpt_module_get_pattern_row_channel_command(b->mpt, p, r, c, OPENMPT_MODULE_COMMAND_NOTE);
          if (note >= 1 && note <= 120) out[c] = 1;
        }
    }
    return channels;
  }
  struct xmp_module* mod = info(b).mod;
  for (int c = 0; c < mod->chn && c < max; c++) out[c] = 0;
  for (int p = 0; p < mod->pat; p++) {
    if (!mod->xxp[p]) continue;
    for (int c = 0; c < mod->chn && c < max; c++) {
      const struct xmp_track* t = mod->xxt[mod->xxp[p]->index[c]];
      if (out[c] || !t) continue;
      for (int r = 0; r < t->rows; r++) if (t->event[r].note >= 1 && t->event[r].note <= 120) { out[c] = 1; break; }
    }
  }
  return mod->chn;
}

/* One row of a pattern as text, its channels' cells separated by '|' (libopenmpt's own notation for its modules; for libxmp-lite's
   the note, instrument, volume and effect as a tracker shows them, the effect as libxmp-lite's own type in hex). The text lives
   until the next call. */
static char row_text[64 * 24 + 1];
EMSCRIPTEN_KEEPALIVE const char* bm_row_text(bm_module* b, int pattern, int row) {
  size_t at = 0;
  row_text[0] = 0;
  if (b->mpt) {
    int channels = openmpt_module_get_num_channels(b->mpt);
    for (int c = 0; c < channels; c++) {
      const char* cell = openmpt_module_format_pattern_row_channel(b->mpt, pattern, row, c, 0, 1);
      size_t n = cell ? strlen(cell) : 0;
      if (at + n + 2 >= sizeof row_text) { if (cell) openmpt_free_string(cell); break; }
      if (c) row_text[at++] = '|';
      if (n) memcpy(row_text + at, cell, n);
      at += n;
      row_text[at] = 0;
      if (cell) openmpt_free_string(cell);
    }
    return row_text;
  }
  struct xmp_module* mod = info(b).mod;
  if (pattern < 0 || pattern >= mod->pat || !mod->xxp[pattern]) return row_text;
  for (int c = 0; c < mod->chn; c++) {
    char cell[24];
    const struct xmp_track* t = mod->xxt[mod->xxp[pattern]->index[c]];
    static const struct xmp_event none;
    byopop_xmp_cell(t && row < t->rows ? &t->event[row] : &none, cell);
    size_t n = strlen(cell);
    if (at + n + 2 >= sizeof row_text) break;
    if (c) row_text[at++] = '|';
    memcpy(row_text + at, cell, n);
    at += n;
    row_text[at] = 0;
  }
  return row_text;
}

/* Plays on from seconds into the selected subsong (the seek bar), the decoder at rate; the position it reached (libxmp-lite goes
   to the frame nearest it) */
EMSCRIPTEN_KEEPALIVE double bm_seek(bm_module* b, int rate, double seconds) {
  if (b->mpt) return openmpt_module_set_position_seconds(b->mpt, seconds);
  if (rate != b->rate) { xmp_end_player(b->xmp); xmp_start(b, rate); }
  b->ended = 0;
  xmp_seek_time_frame(b->xmp, (int)(seconds * 1000));
  return bm_position(b);
}
