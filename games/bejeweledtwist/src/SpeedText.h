// Sexy::SpeedText (0x178 bytes, a TwistWidget; 0x74e1d8-0x74e560): the speed bonus's banner. It sweeps in from the
// right at the top of the screen and its mirror at the bottom, holds in the middle, and sweeps out to the left,
// trailing copies of its glow behind it.
#pragma once

#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Image;

class SpeedText : public TwistWidget {
public:
	std::vector<int> mTrail;  // +0x148: the last places of the banner, oldest first (20 at most)
	Image* mGlow;             // +0x158: drawn additively, and in the trail
	Image* mText;             // +0x15c
	float mHold = 0;          // +0x164: in the middle until it runs out
	int mX;                   // +0x168
	int mY;                   // +0x16c: the top banner's; the bottom one mirrors it
	int mState = 0;           // +0x170: 0 sweeps in, 1 holds, 2 sweeps out, 3 done
	bool mGone = false;       // +0x174: off the screen, the trail no longer grows

	// 0x74e371: theLevel 0 the speed bonus, 1 hyper speed, 2 giga speed (more is giga). The original also takes a
	// height (a fifth of the board's) that it never uses
	SpeedText(int theLevel);
	~SpeedText() override;   // 0x74e336

	void Update() override;          // 0x74e445
	void Draw(Graphics* g) override; // 0x74e1d8
};

}  // namespace Sexy
