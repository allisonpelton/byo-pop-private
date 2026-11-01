#ifndef __PLATFORM_OPENMPTPLAYER_H__
#define __PLATFORM_OPENMPTPLAYER_H__

// MO3 music through libopenmpt (third_party/libopenmpt), for the games whose music is an MO3 module
// played by order position (BookWorm Deluxe, Typer Shark, AstroPop, ...; docs/POPCAP_GAMES_SURVEY.md
// 1.4). Only MO3 goes through it: S3M/IT/XM stay on libxmp-lite, so the other games sound as before.
//
// Built as its own library (sexyopenmpt, framework/CMakeLists.txt): a game that wants it links it and
// calls RegisterOpenMPTModules() once before loading music. Games that do not never link libopenmpt.

#include "platform/Audio.h"

#include <memory>
#include <vector>

namespace Platform
{

// Registers libopenmpt with Platform::Mixer for MO3 files ("MO3" magic).
void RegisterOpenMPTModules();

// A player for an MO3 file in memory, or nullptr: for a game that renders it through a ModulePlayer of its own
// (Mixer::AddSong), such as Mahjong Escape's, whose BASS row syncs jump between orders as the music plays.
std::unique_ptr<ModulePlayer> CreateOpenMPTModule(const std::vector<uint8_t>& theFile, int theRate);

}

#endif
