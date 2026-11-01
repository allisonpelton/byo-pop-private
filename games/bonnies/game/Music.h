#ifndef __BONNIES_MUSIC_H__
#define __BONNIES_MUSIC_H__

// The music (0x40ddc0-0x40e250, 0x24 bytes, the object 0x477440): the tracks of the table 0x473c70 (10 of 0x2c bytes,
// filled by the exe's static initializers), chosen by kind (1 the logo, 2 the menu, 3 the game) and faded between.

#include "Str.h"

namespace bonnies
{

// A track (0x473c70 + 0x2c each)
struct MusicTrack
{
	int mPlayed;              // +0x00: the music's count when it last played
	int mKind;                // +0x04
	int mModule;              // +0x08: a part of the music module (else the stream mName)
	int mBegin;               // +0x0c
	int mEnd;                 // +0x10
	int mStartOrder;          // +0x14
	int mStopOrder;           // +0x18
	Str mName;                // +0x1c
	Str mStr24;               // +0x24
};

extern MusicTrack gTracks[10];  // 0x473c70

class Music
{
public:
	// 0x40dec0
	Music();
	// 0x42f0a0
	~Music();

	// 0x40ddc0: plays track theIndex
	void play(int theIndex);
	// 0x40de90
	void stop();
	// 0x40def0: a track of theKind (the current one fades out first unless theNow)
	void select(int theKind, int theNow);
	// 0x40e020: the music's volume
	void setVolume(float theFactor);
	// 0x40e080: the fades, and a new track when the last ended
	void update();

	int mField00;             // +0x00 (1)
	int mKind;                // +0x04: the kind playing (0 none)
	int mIndex;               // +0x08: the track playing
	int mCount;               // +0x0c: the tracks played
	uint32_t mTime;           // +0x10
	uint32_t mEndedTime;      // +0x14: when the music was found ended
	uint32_t mFadeStart;      // +0x18: a fade out (0 none)
	uint32_t mFadeEnd;        // +0x1c
	int mNextKind;            // +0x20: played after the fade
};

} // namespace bonnies

#endif
