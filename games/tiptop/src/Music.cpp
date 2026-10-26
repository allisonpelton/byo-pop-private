// TipTop's MIDI sequencer (TipTop.exe 0x4276f0-0x428031). See Music.h.

#include "Music.h"
#include "Game.h"
#include "GameText.h"
#include "Res.h"

#include <cmath>
#include <cstring>

namespace tiptop
{

namespace
{

// A track of the file being played (0xc bytes, 16 at 0x489f18)
struct Track
{
	const uint8_t* mPos;    // +0x00 the next event's delta time; null when the track has ended
	uint32_t mWait;         // +0x04 ticks gone by since the last event
	uint8_t mStatus;        // +0x08 the running status
};

Track gTracks[0x10];                 // 0x489f18
volatile char gPauseCount = 0;       // 0x489f08
volatile char gPausedAck = 0;        // 0x489f11 the thread has muted the music for a pause
uint32_t gLastTime = 0;              // 0x489f0c
char gVolumeDirty = 0;               // 0x489f10 the channels' volumes must be sent again
uint16_t gDivision = 0;              // 0x489fd8 ticks per quarter note
const uint8_t* gMidi = nullptr;      // 0x489fe0
uint32_t gTempo = 0;                 // 0x489fe4 microseconds per quarter note (0 until the file sets it)
uint8_t gVolumes[0x80];              // 0x489fe8 a channel volume (CC 7) scaled by the music volume
volatile char gStop = 0;             // 0x48a068 1 asks the thread to end, 2 it has ended
uint64_t gRemainder = 0;             // 0x48a070 the tick computation's remainder
uint16_t gChannelVolumes[0x10];      // 0x48a078 the last CC 7 of each channel (0xff: none)
char gPlaying = 0;                   // 0x48a098
uint32_t gMusicVolume = 0x100;       // 0x47e298
uint32_t gSentVolume = 0x101;        // the thread's local_c: the music volume it last sent

MidiOutFunc gMidiOut = nullptr;

void MidiOutShortMsg(uint32_t theMessage)
{
	if (gMidiOut != nullptr)
		gMidiOut(theMessage);
}

uint16_t Big16(const uint8_t* p)
{
	return (uint16_t)(p[0] << 8 | p[1]);
}

uint32_t Big32(const uint8_t* p)
{
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

// 0x4276f0: points the tracks at the starts of the file's track chunks
void StartTracks()
{
	const uint8_t* aPos = gMidi;
	uint32_t aTrackCount = Big16(gMidi + 10);
	gDivision = Big16(gMidi + 0xc);
	if (Big16(gMidi + 8) != 1)
		lowos::MessageBox_(GTC(0x0046f1b0), GTC(0x0046c498));
	if (0x10 < aTrackCount)
		lowos::MessageBox_(GTC(0x0046f17c), GTC(0x0046c498));
	aPos = aPos + 0xe;
	for (uint32_t i = 0; (i < 0x10 && (i != aTrackCount)); i = i + 1)
	{
		gTracks[i].mPos = aPos + 8;
		aPos = aPos + 8 + Big32(aPos + 4);
	}
	for (int i = 0; i < 0x10; i = i + 1)
		gChannelVolumes[i] = 0x7f;
	gVolumeDirty = 1;
}

// 0x42792b: plays the events that are due
void Sequence()
{
	uint32_t aNow = RealTime();
	if (aNow < gLastTime)
		gLastTime = aNow;
	uint32_t anElapsed = aNow - gLastTime;
	uint32_t aTicks;
	if (gTempo == 0)
	{
		aTicks = 0;
		gLastTime = aNow;
	}
	else
	{
		gLastTime = aNow;
		uint64_t aScaled = (uint64_t)anElapsed * gDivision * 1000 + gRemainder;
		aTicks = (uint32_t)(aScaled / gTempo);
		gRemainder = aScaled % gTempo;
	}
	for (int t = 0; t < 0x10; t = t + 1)
	{
		Track& aTrack = gTracks[t];
		if (aTrack.mPos == nullptr)
			continue;
		aTrack.mWait = aTrack.mWait + aTicks;
		while (aTrack.mPos != nullptr)
		{
			uint32_t aDelta = 0;
			int aLength = 0;
			while ((aLength < 4 && (aDelta = aDelta << 7 | (aTrack.mPos[aLength] & 0x7f),
				(~aTrack.mPos[aLength] & 0x80) == 0)))
				aLength = aLength + 1;
			if (aTrack.mWait < aDelta)
				break;
			aTrack.mWait = aTrack.mWait - aDelta;
			aTrack.mPos = aTrack.mPos + aLength + 1;
			uint8_t aStatus = *aTrack.mPos;
			if (aStatus < 0x80)
			{
				aStatus = aTrack.mStatus;
			}
			else
			{
				aTrack.mStatus = aStatus;
				aTrack.mPos = aTrack.mPos + 1;
			}
			if (aStatus < 0xf0)
			{
				uint32_t aMessage;
				if (((aStatus & 0xf0) == 0xc0) || ((aStatus & 0xf0) == 0xd0))
				{
					aMessage = (uint32_t)(*aTrack.mPos << 8 | aStatus);
					aTrack.mPos = aTrack.mPos + 1;
				}
				else
				{
					uint8_t aData2;
					if (((aStatus & 0xf0) == 0xb0) && (*aTrack.mPos == 7))
					{
						gChannelVolumes[aStatus & 0xf] = aTrack.mPos[1];
						aData2 = gVolumes[aTrack.mPos[1]];
					}
					else
					{
						aData2 = aTrack.mPos[1];
					}
					aMessage = (uint32_t)aStatus | (uint32_t)aData2 << 0x10;
					aMessage = aMessage | (uint32_t)*aTrack.mPos << 8;
					aTrack.mPos = aTrack.mPos + 2;
				}
				MidiOutShortMsg(aMessage);
			}
			else if (aStatus == 0xff)
			{
				if (*aTrack.mPos == '/')
				{
					// the end of a track; when all have ended, the song starts again
					aTrack.mPos = nullptr;
					for (int i = 0; gTracks[i].mPos == nullptr; i = i + 1)
					{
						if (i == 0xf)
						{
							for (i = 0; i < 0x10; i = i + 1)
								gTracks[i].mWait = aTrack.mWait;
							StartTracks();
							return;
						}
					}
				}
				else if (*aTrack.mPos == 'Q')
				{
					gTempo = (uint32_t)aTrack.mPos[2] << 16 | (uint32_t)aTrack.mPos[3] << 8 | aTrack.mPos[4];
				}
				if (aTrack.mPos != nullptr)
					aTrack.mPos = aTrack.mPos + aTrack.mPos[1] + 2;
			}
		}
	}
}

} // namespace

void SetMidiOut(MidiOutFunc theFunc)
{
	gMidiOut = theFunc;
}

// 0x42782c: the table of channel volumes for a music volume (0..0x100)
void BuildMusicVolumes(uint32_t theVolume)
{
	for (uint32_t i = 0; i < 0x80; i = i + 1)
	{
		double aLevel = (double)i / 127.0;
		aLevel = aLevel * aLevel;
		aLevel = (double)theVolume * aLevel / 256.0;
		double aValue = pow(aLevel * aLevel, 0.25) * 127.0;
		uint8_t aByte = (uint8_t)(int64_t)aValue;
		gVolumes[i] = (aByte < 0x7f) ? aByte : 0x7f;
	}
	gMusicVolume = theVolume;
}

// 0x427921
uint32_t MusicVolume()
{
	return gMusicVolume;
}

// 0x427ddc: the player thread. The port runs one pass of its loop per frame (UpdateMusic); the original slept
// 20 ms after each, and waited in the loop while the music was paused.
void UpdateMusic()
{
	if (!gPlaying || gStop != 0)
		return;
	if ((gSentVolume != gMusicVolume) || (gVolumeDirty != 0))
	{
		gSentVolume = gMusicVolume;
		gVolumeDirty = 0;
		for (int i = 0; i < 0x10; i = i + 1)
		{
			if (gChannelVolumes[i] != 0xff)
				MidiOutShortMsg((uint32_t)gVolumes[gChannelVolumes[i]] << 0x10 | 0x7b0 | i);
		}
	}
	if (gPauseCount == 0)
	{
		if (gPausedAck != 0)
		{
			// the end of the original's wait for the pause to end
			gLastTime = RealTime();
			gRemainder = 0;
			gPausedAck = 0;
		}
		Sequence();
	}
	else if (gPausedAck == 0)
	{
		for (int i = 0; i < 0x10; i = i + 1)
			MidiOutShortMsg(i | 0x7b0);
		gVolumeDirty = 1;
		gPausedAck = 1;
	}
}

// 0x427f35: plays resource theId of theGroup (the original opened the MIDI mapper here and started the thread)
char PlayMusic(int theGroup, int theId)
{
	memset(gTracks, 0, sizeof(gTracks));
	gMidi = (const uint8_t*)res::Get(theGroup, theId);
	if (gMidi == nullptr)
		return 1;
	StartTracks();
	gLastTime = RealTime();
	gRemainder = 0;
	gTempo = 0;
	gStop = 0;
	gSentVolume = 0x101;
	gPlaying = 1;
	return 0;
}

// 0x427fe5
char MusicPlaying()
{
	return gPlaying;
}

// 0x427fef
void StopMusic_()
{
	if (gPlaying != 0)
	{
		gStop = 2;
		gPlaying = 0;
	}
}

// 0x428031: pauses (counted) or resumes the music; the original waited for the thread to answer
void PauseMusic(char isPaused)
{
	if (gPlaying != 0)
	{
		if (isPaused == 0)
		{
			if (gPauseCount != 0)
				gPauseCount = gPauseCount - 1;
		}
		else
		{
			gPauseCount = gPauseCount + 1;
		}
	}
}

} // namespace tiptop
