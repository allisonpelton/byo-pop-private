#ifndef SDL_h_
#define SDL_h_

// port(variant): headless shim of <SDL.h> for the verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not
// part of the original game. The board's code logs through SDL_Log only (a sidecar it cannot write, which a
// playback board never writes): the verifier sends it to stderr.

#ifdef __cplusplus
extern "C" {
#endif

void SDL_Log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
