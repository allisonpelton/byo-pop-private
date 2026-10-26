#ifndef __PIXELUS_TEMPLES_H__
#define __PIXELUS_TEMPLES_H__

// The ten temples (Pixelus.exe data 0x53d3a0-0x53d750, 0x543550): where each stands on the island, how many puzzles
// have to be solved (outside the tutorial) before it opens, and the colour of its god's glow. Index 0 is the
// tutorial's cave, 1 the first temple, 9 Jupiter's.

namespace Sexy
{

class PixelusApp;

struct TempleInfo
{
	const char*		mName;			// 0x53d740 the level's name in levels/levels.txt
	int				mImage;			// the picture of the temple on the island (two frames: not complete, complete)
	int				mX, mY;			// 0x53d3a0 where that picture goes
	int				mLightX, mLightY;	// 0x53d3f0 the middle of its glow
	int				mUnlock;		// 0x53d718 puzzles solved outside the tutorial that open it
	int				mRed, mGreen, mBlue;	// 0x543550 the colour of its panel and glow
};

enum { NUM_TEMPLES = 10 };

extern const TempleInfo gTemples[NUM_TEMPLES];

// 0x433640: the temple's door is open (the first temple waits for the tutorial)
bool IsTempleOpen(PixelusApp* theApp, int theTemple);
// 0x433570: fewer puzzles solved than the temple asks for (the dialog on the door says so)
bool IsTempleLocked(PixelusApp* theApp, int theTemple);

}

#endif
