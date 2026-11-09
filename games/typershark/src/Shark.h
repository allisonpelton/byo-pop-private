#ifndef __TYPERSHARK_SHARK_H__
#define __TYPERSHARK_SHARK_H__

// Shark.cpp: the sharks that swim across the screen with their word over them. BlackShark, RedShark, StealthShark and
// ToxicShark derive from it.
// Original: class Sexy::Shark : Enemy, GameObject (virtual bases), own part with 1 virtual (0x401200), vbptr at +8,
// the GameObject part at +0x30, the Enemy part at +0x120, sizeof 0x210; constructor 0x4101d0. The field comments give
// the offsets in the original's own part.
//
// States (GameObject::mState): 0 swimming left across the screen (speeding up near the left edge, slowing after the
// dash at the right edge), 1 swimming back from the left (mirrored picture), 2 dead (not drawn), 3 dying (falls up),
// 4 chased off to the left, 5 shocked (flashing), 6 hit (swims like 0 and flashes red). SetState(6) is ignored unless
// the shark swims, SetState(5) while it is chased off.

#include "Enemy.h"

namespace Sexy
{

class Graphics;

class Shark : public Enemy
{
public:
	int						mFlashCount;			// +0x10 flashes left in state 6 (SetState(6) sets 10), 0 otherwise
	int						mFlashPeriod;			// +0x14 ticks between two flashes (5)
	double					mSavedVelX;				// +0x18 the speed to come back to after the dash; -1 = none yet
	bool					mFlashed;				// +0x20 the red picture is showing (state 6)
	bool					mEntered;				// +0x21 state 1 has brought the shark on to the screen
	int						mFrameOffset;			// +0x24 mFrame * mFrameWidth: the x of the source rectangle in the picture

public:
	Shark(WinWSApp* theApp);														// 0x4101d0
	virtual ~Shark();																// deleting destructor

	virtual bool			IsSwimming();											// own slot 0, 0x401200: state 0 or 6
	virtual void			Update() override;										// slot 1, 0x40b930
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4024e0
	virtual void			SetState(int theState) override;						// slot 7, 0x404c20
};

}

#endif
