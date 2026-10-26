#ifndef __TIPTOP_MUSIC_H__
#define __TIPTOP_MUSIC_H__

// TipTop's music (TipTop.exe 0x4276f0-0x428031): a MIDI file of the resources (group 1) played by the game's own
// sequencer on a thread, through the Windows MIDI mapper (midiOutShortMsg). The port runs the sequencer once per
// frame from the main loop instead of a thread every 20 ms, and sends its messages to a MIDI output that the host
// sets (none yet: the messages are dropped, so the game is silent apart from its sounds).

#include <cstdint>

namespace tiptop
{

// Where the sequencer's short MIDI messages go (status | data1 << 8 | data2 << 16, as midiOutShortMsg)
typedef void (*MidiOutFunc)(uint32_t theMessage);
void SetMidiOut(MidiOutFunc theFunc);

void BuildMusicVolumes(uint32_t theVolume);         // 0x42782c (0..0x100)
uint32_t MusicVolume();                             // 0x427921
char PlayMusic(int theGroup, int theId);            // 0x427f35 1 on failure
char MusicPlaying();                                // 0x427fe5
void StopMusic_();                                  // 0x427fef
void PauseMusic(char isPaused);                     // 0x428031 counted
// One pass of the player thread's loop (0x427ddc), from the main loop
void UpdateMusic();

} // namespace tiptop

#endif
