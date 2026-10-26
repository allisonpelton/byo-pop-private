#ifndef __TIPTOP_MUSIC_H__
#define __TIPTOP_MUSIC_H__

// TipTop's music (TipTop.exe 0x4276f0-0x428031): a MIDI file of the resources (group 1) played by the game's own
// sequencer on a thread, through the Windows MIDI mapper (midiOutShortMsg). The port sends the messages to a MIDI
// output that the host sets, a SoundFont synthesizer (MusicSynth.cpp), and runs the thread's loop in the synth's
// rendering in the audio callback, a pass for each millisecond of sound instead of every 20 ms; the functions the
// game calls hold the mixer's lock, as the audio thread runs the sequencer.

#include <cstdint>

namespace tiptop
{

// Where the sequencer's short MIDI messages go (status | data1 << 8 | data2 << 16, as midiOutShortMsg)
typedef void (*MidiOutFunc)(uint32_t theMessage);
void SetMidiOut(MidiOutFunc theFunc);
// port: the message that stands for the original's midiOutClose when the music stops (a MIDI system reset)
const uint32_t MIDI_CLOSE = 0xff;

void BuildMusicVolumes(uint32_t theVolume);         // 0x42782c (0..0x100)
uint32_t MusicVolume();                             // 0x427921
char PlayMusic(int theGroup, int theId);            // 0x427f35 1 on failure
char MusicPlaying();                                // 0x427fe5
void StopMusic_();                                  // 0x427fef
void PauseMusic(char isPaused);                     // 0x428031 counted
// One pass of the player thread's loop (0x427ddc), from the synth's rendering; theTime is the milliseconds of sound
// rendered, the clock the sequencer reads instead of the tick count
void UpdateMusic(uint32_t theTime);

} // namespace tiptop

#endif
