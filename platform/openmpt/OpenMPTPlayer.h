#ifndef __PLATFORM_OPENMPTPLAYER_H__
#define __PLATFORM_OPENMPTPLAYER_H__

// MO3 music through libopenmpt (third_party/libopenmpt), for the games whose music is an MO3 module
// played by order position (BookWorm Deluxe, Typer Shark, AstroPop, ...; docs/POPCAP_GAMES_SURVEY.md
// 1.4). Only MO3 goes through it: S3M/IT/XM stay on libxmp-lite, so the other games sound as before.
//
// Built as its own library (sexyopenmpt, framework/CMakeLists.txt): a game that wants it links it and
// calls RegisterOpenMPTModules() once before loading music. Games that do not never link libopenmpt.

namespace Platform
{

// Registers libopenmpt with Platform::Mixer for MO3 files ("MO3" magic).
void RegisterOpenMPTModules();

// Registers libopenmpt for OXM files (XM modules with Ogg Vorbis samples, which libxmp-lite cannot read). Plain XM
// files stay on libxmp-lite.
void RegisterOpenMPTOggXM();

}

#endif
