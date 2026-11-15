/* The music player's MIDI tunes (web/music.js; docs/WEB_PLAYER.md, "The music player"), in openmpt.wasm beside the module
   decoder (web/music/openmpt_web.c). A MIDI tune plays as its game plays it: TipTop, the one game whose music is MIDI, through
   its own sequencer as the port has it (games/tiptop/src/Music.cpp, TipTop.exe 0x4276f0-0x428031: a pass over the tracks
   each millisecond, its channel volume table, and the song started again once every track has ended) into TinySoundFont with
   the port's General MIDI bank, set up as the port sets it (games/tiptop/src/MusicSynth.cpp). The page fetches the bank
   (tiptop.sf2, beside the pages) and hands it in with the file. No game files are in it. */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO
#include "tsf.h"

/* MusicSynth.cpp's voices and level */
#define MIDI_MAX_VOICES 256
#define MIDI_GAIN_DB -1.0f
/* the longest pass bm_midi_seconds follows, in milliseconds (a file whose tracks never all end) */
#define MIDI_LONGEST (60 * 60 * 1000)

typedef struct {
  const uint8_t* pos;   /* the next event's delta time; null once the track has ended */
  uint32_t wait;        /* ticks gone by since the last event */
  uint8_t status;       /* the running status */
} midi_track;

typedef struct {
  /* the sequencer (Music.cpp) */
  uint8_t* file;
  int size;
  midi_track tracks[16];
  uint16_t division;
  uint32_t tempo;
  uint64_t remainder;
  uint32_t last_time;
  uint8_t volumes[0x80];
  uint16_t channel_volumes[16];
  int volume_dirty;
  uint32_t sent_volume;
  /* the synth (MusicSynth.cpp); null for a length only */
  tsf* synth;
  uint8_t synth_volume[16];
  uint8_t synth_expression[16];
  int chase;            /* a seek: notes are not played, the rest is */
  /* the player */
  int rate;
  uint32_t time;        /* the synth's clock in milliseconds */
  float* ms_out;        /* the millisecond in progress, rendered whole (as the port's musicwav renders it: TinySoundFont's
                           envelopes and filters step in blocks counted from each render call's start, so a millisecond split
                           in two would sound slightly different) */
  int ms_frames;        /* its frames */
  int ms_at;            /* how many of them have been given out */
  int again;            /* the millisecond in progress started the song again */
  int loop;             /* repeat: play on after the song's end */
  int ended;
  uint64_t pass_frames; /* frames since the song (re)started */
} bm_midi;

static uint16_t big16(const uint8_t* p) { return (uint16_t)(p[0] << 8 | p[1]); }
static uint32_t big32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

/* MusicSynth.cpp's SetChannelVolume: 40 log10 of each of CC 7 and CC 11 over 127, a square law */
static void synth_channel_volume(bm_midi* h, int c) {
  float level = (h->synth_volume[c] / 127.0f) * (h->synth_expression[c] / 127.0f);
  tsf_channel_set_volume(h->synth, c, level * level);
}

/* MusicSynth.cpp's ResetChannels: program 0, channel 10 on the drum kits, volume 100 */
static void synth_reset_channels(bm_midi* h) {
  for (int c = 15; c >= 0; c--) {
    tsf_channel_set_presetnumber(h->synth, c, 0, c == 9);
    h->synth_volume[c] = 100;
    h->synth_expression[c] = 127;
    synth_channel_volume(h, c);
  }
}

/* MusicSynth.cpp's SynthMessage: a short message to the synth */
static void midi_out(bm_midi* h, uint32_t message) {
  if (!h->synth) return;
  int status = message & 0xff, channel = status & 0xf, first = (message >> 8) & 0x7f, second = (message >> 16) & 0x7f;
  switch (status & 0xf0) {
  case 0x80:
    if (!h->chase) tsf_channel_note_off(h->synth, channel, first);
    break;
  case 0x90:
    if (!h->chase) tsf_channel_note_on(h->synth, channel, first, second / 127.0f);   /* velocity 0 is a note off */
    break;
  case 0xb0:
    if (first == 7) h->synth_volume[channel] = (uint8_t)second;
    else if (first == 11) h->synth_expression[channel] = (uint8_t)second;
    else tsf_channel_midi_control(h->synth, channel, first, second);
    if (first == 121) h->synth_expression[channel] = 127;   /* reset all controllers: the expression, not the volume */
    if (first == 7 || first == 11 || first == 121) synth_channel_volume(h, channel);
    break;
  case 0xc0:
    tsf_channel_set_presetnumber(h->synth, channel, first, channel == 9);
    break;
  case 0xe0:
    tsf_channel_set_pitchwheel(h->synth, channel, second << 7 | first);
    break;
  }
}

/* Music.cpp's StartTracks (0x4276f0): the tracks at the starts of the file's track chunks */
static void start_tracks(bm_midi* h) {
  const uint8_t* at = h->file + 0xe;
  uint32_t count = big16(h->file + 10);
  h->division = big16(h->file + 0xc);
  for (uint32_t i = 0; i < 16 && i != count; i++) {
    h->tracks[i].pos = at + 8;
    at = at + 8 + big32(at + 4);
  }
  for (int i = 0; i < 16; i++) h->channel_volumes[i] = 0x7f;
  h->volume_dirty = 1;
}

/* Music.cpp's Sequence (0x42792b): the events that are due; 1 when every track has ended and the song started again */
static int sequence(bm_midi* h) {
  uint32_t now = h->time, ticks;
  if (now < h->last_time) h->last_time = now;
  uint32_t elapsed = now - h->last_time;
  h->last_time = now;
  if (h->tempo == 0) ticks = 0;
  else {
    uint64_t scaled = (uint64_t)elapsed * h->division * 1000 + h->remainder;
    ticks = (uint32_t)(scaled / h->tempo);
    h->remainder = scaled % h->tempo;
  }
  for (int t = 0; t < 16; t++) {
    midi_track* track = &h->tracks[t];
    if (!track->pos) continue;
    track->wait += ticks;
    while (track->pos) {
      uint32_t delta = 0;
      int length = 0;
      while (length < 4 && (delta = delta << 7 | (track->pos[length] & 0x7f), (~track->pos[length] & 0x80) == 0)) length++;
      if (track->wait < delta) break;
      track->wait -= delta;
      track->pos += length + 1;
      uint8_t status = *track->pos;
      if (status < 0x80) status = track->status;
      else { track->status = status; track->pos++; }
      if (status < 0xf0) {
        uint32_t message;
        if ((status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0) {
          message = (uint32_t)(*track->pos << 8 | status);
          track->pos++;
        } else {
          uint8_t second;
          if ((status & 0xf0) == 0xb0 && *track->pos == 7) {
            h->channel_volumes[status & 0xf] = track->pos[1];
            second = h->volumes[track->pos[1]];
          } else second = track->pos[1];
          message = (uint32_t)status | (uint32_t)second << 16 | (uint32_t)*track->pos << 8;
          track->pos += 2;
        }
        midi_out(h, message);
      } else if (status == 0xff) {
        if (*track->pos == '/') {
          /* the end of a track; when all have ended, the song starts again */
          track->pos = 0;
          for (int i = 0; !h->tracks[i].pos; i++) {
            if (i == 15) {
              for (i = 0; i < 16; i++) h->tracks[i].wait = track->wait;
              start_tracks(h);
              return 1;
            }
          }
        } else if (*track->pos == 'Q') {
          h->tempo = (uint32_t)track->pos[2] << 16 | (uint32_t)track->pos[3] << 8 | track->pos[4];
        }
        if (track->pos) track->pos += track->pos[1] + 2;
      }
    }
  }
  return 0;
}

/* Music.cpp's UpdateMusic (0x427ddc): one pass of the player thread, for the millisecond h->time */
static int update(bm_midi* h) {
  if (h->sent_volume != 0x100 || h->volume_dirty) {
    h->sent_volume = 0x100;
    h->volume_dirty = 0;
    for (int i = 0; i < 16; i++)
      if (h->channel_volumes[i] != 0xff) midi_out(h, (uint32_t)h->volumes[h->channel_volumes[i]] << 16 | 0x7b0 | i);
  }
  return sequence(h);
}

/* Music.cpp's PlayMusic (0x427f35): the song from its start, the synth's notes ended and its channels as a GS synth starts them */
static void restart(bm_midi* h) {
  memset(h->tracks, 0, sizeof(h->tracks));
  start_tracks(h);
  h->last_time = h->time;
  h->remainder = 0;
  h->tempo = 0;
  h->sent_volume = 0x101;
  h->ms_frames = h->ms_at = 0;
  h->pass_frames = 0;
  h->ended = 0;
  if (h->synth) { tsf_reset(h->synth); synth_reset_channels(h); }
}

/* the frames of the millisecond h->time */
static int frames_of(bm_midi* h) {
  return (int)((uint64_t)(h->time + 1) * h->rate / 1000 - (uint64_t)h->time * h->rate / 1000);
}

/* the bytes after a file's copy: ends of tracks, for a track that runs past its file's end */
#define MIDI_PAD 512

static bm_midi* midi_new(const void* data, int size, int rate) {
  const uint8_t* d = (const uint8_t*)data;
  if (size < 22 || memcmp(d, "MThd", 4) || big32(d + 4) != 6) return 0;
  /* (every track chunk the sequencer reads inside the file) */
  uint32_t count = big16(d + 10), at = 0xe;
  for (uint32_t i = 0; i < 16 && i != count; i++) {
    if (at + 8 > (uint32_t)size || memcmp(d + at, "MTrk", 4) || big32(d + at + 4) > (uint32_t)size - at - 8) return 0;
    at += 8 + big32(d + at + 4);
  }
  bm_midi* h = (bm_midi*)calloc(1, sizeof(bm_midi));
  if (!h) return 0;
  h->file = (uint8_t*)malloc((size_t)size + MIDI_PAD);
  if (!h->file) { free(h); return 0; }
  memcpy(h->file, d, (size_t)size);
  for (int i = 0; i < MIDI_PAD; i += 4) memcpy(h->file + size + i, "\x00\xff\x2f\x00", 4);
  h->size = size;
  h->rate = rate > 0 ? rate : 48000;
  /* Music.cpp's BuildMusicVolumes (0x42782c) at the full music volume, 0x100 */
  for (int i = 0; i < 0x80; i++) {
    double level = (double)i / 127.0;
    level = level * level;
    level = (double)0x100 * level / 256.0;
    uint8_t v = (uint8_t)(int64_t)(pow(level * level, 0.25) * 127.0);
    h->volumes[i] = v < 0x7f ? v : 0x7f;
  }
  return h;
}

EMSCRIPTEN_KEEPALIVE void bm_midi_free(bm_midi* h);

/* A MIDI tune from its file and a SoundFont bank (both copied), rendered at rate; 0 when either is not one */
EMSCRIPTEN_KEEPALIVE bm_midi* bm_midi_load(const void* data, int size, const void* bank, int bank_size, int rate) {
  bm_midi* h = midi_new(data, size, rate);
  if (!h) return 0;
  h->synth = tsf_load_memory(bank, bank_size);
  h->ms_out = (float*)malloc(((size_t)h->rate / 1000 + 1) * 2 * sizeof(float));
  if (!h->synth || !h->ms_out) { bm_midi_free(h); return 0; }
  tsf_set_output(h->synth, TSF_STEREO_INTERLEAVED, h->rate, MIDI_GAIN_DB);
  tsf_set_max_voices(h->synth, MIDI_MAX_VOICES);
  restart(h);
  return h;
}

EMSCRIPTEN_KEEPALIVE void bm_midi_free(bm_midi* h) {
  if (!h) return;
  if (h->synth) tsf_close(h->synth);
  free(h->ms_out);
  free(h->file);
  free(h);
}

/* repeat: 1 plays on after the song's end, as the game does; 0 ends there */
EMSCRIPTEN_KEEPALIVE void bm_midi_loop(bm_midi* h, int loop) { h->loop = loop; }

/* The next millisecond: the sequencer's pass, and its frames to render. 0 where the song ends there and repeat is off. The
   millisecond in which the sequencer starts the song again is the new pass's first. */
static int next_ms(bm_midi* h) {
  h->time++;
  if (update(h)) {
    if (!h->loop) { h->ended = 1; return 0; }
    h->pass_frames = 0;
    h->again = 1;
  }
  h->ms_frames = frames_of(h);
  h->ms_at = 0;
  return 1;
}

/* Renders up to frames stereo frames into out (MusicSynth.cpp's SynthSong::Render: the sequencer's pass for each millisecond,
   then that millisecond's frames). Fewer where the song ends: with repeat on, the next call goes on from its start; 0 once it
   ended. */
EMSCRIPTEN_KEEPALIVE int bm_midi_render(bm_midi* h, int frames, float* out) {
  int got = 0;
  while (got < frames && !h->ended) {
    if (h->ms_at == h->ms_frames) {
      h->again = 0;
      if (!next_ms(h)) break;
      tsf_render_float(h->synth, h->ms_out, h->ms_frames, 0);
      if (h->again && got) break;
    }
    int n = frames - got < h->ms_frames - h->ms_at ? frames - got : h->ms_frames - h->ms_at;
    memcpy(out + got * 2, h->ms_out + h->ms_at * 2, (size_t)n * 2 * sizeof(float));
    got += n;
    h->ms_at += n;
    h->pass_frames += (uint64_t)n;
  }
  return got;
}

/* seconds into the song since it (last) started */
EMSCRIPTEN_KEEPALIVE double bm_midi_position(bm_midi* h) { return (double)h->pass_frames / h->rate; }

/* The song from seconds in: from its start, the sequencer run to there without its notes (the rest of what it sends, programs,
   controllers and the pitch wheel, set as they would be), and the synth's notes ended */
EMSCRIPTEN_KEEPALIVE void bm_midi_seek(bm_midi* h, double seconds) {
  restart(h);
  uint64_t target = seconds > 0 ? (uint64_t)(seconds * h->rate) : 0;
  h->chase = 1;
  while (!h->ended && h->pass_frames + (uint64_t)frames_of(h) <= target) {
    h->again = 0;
    if (!next_ms(h)) break;
    h->pass_frames += (uint64_t)h->ms_frames;
    h->ms_frames = h->ms_at = 0;
    if (h->again) break;
  }
  h->chase = 0;
}

/* The song's length, from its start until the sequencer starts it again, in seconds (0: not a MIDI file, or no end within an
   hour) */
EMSCRIPTEN_KEEPALIVE double bm_midi_seconds(const void* data, int size) {
  bm_midi* h = midi_new(data, size, 1000);
  if (!h) return 0;
  restart(h);
  double s = 0;
  for (uint32_t ms = 0; ms < MIDI_LONGEST; ms++) {
    h->time++;
    if (update(h)) { s = ms / 1000.0; break; }
  }
  bm_midi_free(h);
  return s;
}
