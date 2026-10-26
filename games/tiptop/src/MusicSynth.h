#ifndef __TIPTOP_MUSICSYNTH_H__
#define __TIPTOP_MUSICSYNTH_H__

// port: the MIDI output of TipTop's music (Music.h). The original sent its sequencer's messages to the Windows MIDI
// mapper, whose General MIDI synth played them; the port plays them with TinySoundFont (third_party/tinysoundfont), a
// SoundFont synthesizer, and GeneralUser GS cut down to the presets the music uses (third_party/generaluser-gs/
// tiptop.sf2, made by tools/sf2trim.py from the game's MIDI file). The synth is a song of the platform's mixer, so the
// music goes through SDL's audio like the game's sounds, and test runs stay silent.

#include <string>

namespace tiptop
{

// Loads the bank and gives the mixer the synth; false (and no music) without the bank
bool StartMusicSynth(const std::string& theBankPath);
void StopMusicSynth();

// Test hook (the -script command musicwav, testhooks/Script.cpp): starts the music again and renders theSeconds of it
// into a 16-bit stereo WAV file, the audio device held off meanwhile
bool RenderMusicWav(const std::string& theFile, int theSeconds);

} // namespace tiptop

#endif
