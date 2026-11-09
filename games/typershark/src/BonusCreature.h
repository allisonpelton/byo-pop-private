#ifndef __TYPERSHARK_BONUSCREATURE_H__
#define __TYPERSHARK_BONUSCREATURE_H__

// BonusCreature.cpp: the jellyfish that drifts up the board with a word on it; typing the word earns a bonus. It
// swims up (mVelY is the speed), plays an 8-frame death when its word is finished or it has left the top of the
// screen, and reports it to the board with events (6 then 5).
// Original: class Sexy::BonusCreature : Enemy, GameObject (virtual bases), the Enemy part at +0 (vftable 0x529bb0, the
// three Enemy virtuals, no own virtuals), vbptr at +4, own fields from +0xe8, the GameObject part at +0x108
// (vftable 0x529b7c, 0xf0 bytes), sizeof 0x1f8; constructor 0x40db40 (its last argument says whether it builds the
// GameObject base itself), destructor of the part 0x40dda0, deleting destructor 0x40dd60. The field comments give the
// offsets in the original's object (the Enemy part's are in Enemy.h).
//
// States (GameObject::mState): 0 drifting up, 3 dying (the 8-frame death picture plays, then the creature stays on its
// last frame, 7, while the board removes it).
//
// port: the single chain GameObject, Enemy, BonusCreature; nothing derives from it through a second path, so no
// virtual inheritance. The vtordisp thunks only adjust the pointer: 0x40dd00 (slot 0, to 0x40dd60), 0x40dd10 and
// 0x40dd20 (slots 3 and 4, to the Enemy level 0x402180 and 0x4021a0, which return the frame size like GameObject's),
// 0x40dd30 (slot 10, to 0x401fb0), 0x40dd40 (slot 7, to 0x406180), 0x40dd50 (slot 2, to 0x408e10) and 0x40e060 (slot 1,
// to 0x40dde0).

#include "Enemy.h"

namespace Sexy
{

class Graphics;

class BonusCreature : public Enemy
{
public:
	int						mField0e8;				// +0xe8 constructor 0xff (not read by this class)
	double					mQuarterVelY;			// +0xf0 constructor 0; SetVelY stores a quarter of the speed (not read by this class)
	int						mFrameOffset;			// +0xf8 mFrame * mFrameWidth: the x of the source rectangle in the picture

public:
	BonusCreature(WinWSApp* theApp);												// 0x40db40
	virtual ~BonusCreature();														// destructor of the part 0x40dda0, deleting 0x40dd60

	virtual void			Update() override;										// slot 1, 0x40dde0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x408e10
	virtual void			SetState(int theState) override;						// slot 7, 0x406180
	virtual void			SetVelY(double theVelY) override;						// slot 10, 0x401fb0
};

}

#endif
