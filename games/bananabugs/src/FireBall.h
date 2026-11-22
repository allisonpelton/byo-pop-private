// FireBall: a fireball thrown at a bug (0x42ad60..0x42b410 in the original). Like a pea it picks a bug at random and
// flies at it, turned toward it, trailing a short swell; it takes at least 60 frames. If the level is still short of
// its target when it lands, the bug is zapped and the ball bursts into twelve star streams, waiting hidden 400 frames
// for them before it goes.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class GridBug;
class StarStream;

const int FIREBALL_STAR_COUNT = 12;

class FireBall : public ObjBase
{
public:
	GridBug* mTarget;        // +0x27c: the bug flown at
	AnimKey* mFlightKey;     // +0x280: from the place it was thrown
	AnimKey* mArrivalKey;    // +0x284: at the bug, where it was when the ball was thrown
	StarStream* mStars[FIREBALL_STAR_COUNT]; // +0x28c: the ball's children
	bool mBurst;             // +0x2bc: the ball has burst, and leaves no trail

	static Anim* sFireBallAnim; // 0x60e50c

	FireBall();              // 0x42ad60
	void Notify(ObjBase* theObj) override; // 0x42ae80
	bool Update() override;  // 0x42af70

	void Throw(float theX, float theY); // 0x42afc0

private:
	void BuildTracks();      // 0x42b150
};

void PreloadFireBall(); // 0x42b3f0

} // namespace Sexy
