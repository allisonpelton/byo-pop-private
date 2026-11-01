#ifndef __BONNIES_OPTIONS_H__
#define __BONNIES_OPTIONS_H__

// The player's preferences (0x404140-0x404420, 0x78 bytes, the object 0x475a68): the last player's name, the volumes,
// full screen and the window's position, kept in SavedGames/General_Preferences.cfg. Also the letter helpers that
// follow them in the exe.

#include "Str.h"

namespace bonnies
{

class Options
{
public:
	// 0x404260: the defaults, then the file when there is one of version 100 or older
	Options();

	// 0x404140: the file's path
	static Str path();
	// 0x404200
	void reset();
	// 0x404260
	void load();
	// 0x404320
	void save();

	Str mName;                // +0x00: the player whose game is loaded
	float mSoundVolume;       // +0x08 (1)
	float mMusicVolume;       // +0x0c (1)
	int mFullScreen;          // +0x10 (Portal.cfg's)
	int mWindowX;             // +0x14 (50): the window (windowMoved's first pair)
	int mWindowY;             // +0x18 (70)
	int mField1C;             // +0x1c
	int mClientX;             // +0x20 (50): windowMoved's second pair
	int mClientY;             // +0x24 (70)
	int mInterstitial;        // +0x28: the interstitial shown last between levels (0, 1)
	uint8_t mBytes[0x4c];     // +0x2c: written to the file, never read back
};

// 0x4043f0, 0x404420: an upper-case vowel, an upper-case consonant
int isVowel(char theChar);
int isConsonant(char theChar);

} // namespace bonnies

#endif
