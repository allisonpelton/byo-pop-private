#ifndef __SEVENSEAS_ANIMS_H__
#define __SEVENSEAS_ANIMS_H__

// Anims.h: the animation definitions of data\anims.txt (WinSS.exe 0x41c960 reads them into maps in the app).
// A definition is a line "NAME image flagA flagB loop" followed by lines "hr <count> <x> <y> <w> <h> <delay>"
// (a strip of count frames of w x h pixels, delay in updates) and ended by an empty line.
//   flagA, flagB: both 0 for the animations that the turn waits for (explosions, splashes); the others are
//                 positioned by their corner when the flag is 1, by their centre when it is 0 (Board::Draw)
//   loop:         the animation starts over after its last frame

#include <string>
#include <vector>

namespace Sexy
{

class Image;

struct AnimFrame
{
	int					mX;
	int					mY;
	int					mW;
	int					mH;
	int					mDelay;
};

struct AnimDef
{
	Image*				mImage;         // +0
	std::vector<AnimFrame>	mFrames;    // +8 .. +0xc
	int					mFlagA;         // +0x14
	int					mFlagB;         // +0x18
	bool				mLoop;          // +0x1c
};

}

#endif
