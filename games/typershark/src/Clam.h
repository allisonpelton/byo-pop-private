#ifndef __TYPERSHARK_CLAM_H__
#define __TYPERSHARK_CLAM_H__

// Clam.cpp: the clam on the sea floor that holds a treasure (one of three kinds). It pulses between two frames until
// the board opens it (state 1) or closes it (state 2); the picture is a sheet of 10 frames across and 4 rows (the row
// of a state 1 to 3 clam depends on its kind). The board tells it apart from the other creatures by its events (1: it
// has opened fully, 2: it has closed, 3: it began to open and is not of kind 0).
// Original: class Sexy::Clam : Enemy, GameObject (virtual bases), the Enemy part at +0 (vftable 0x529bfc, the three Enemy
// virtuals, no own virtuals), vbptr at +4, own fields from +0xe8, the GameObject part at +0x100 (vftable 0x529bc8,
// 0xf0 bytes), sizeof 0x1f0; constructor 0x40e100 (its last argument says whether it builds the GameObject base itself),
// destructor of the part 0x40e3d0, deleting destructor 0x40e390. The field comments give the offsets in the original's
// object (the Enemy part's are in Enemy.h).
//
// States (GameObject::mState): 0 and 4 the pulsing frames of the first row (4 also draws the word, 41 under the clam,
// in the 14 point font), 1 opening, 2 closing, 3 pulsing in the row of the kind.
//
// port: the single chain GameObject, Enemy, Clam; nothing derives from it through a second path, so no virtual
// inheritance. The vtordisp thunks only adjust the pointer: 0x40e350 (slot 0, to 0x40e390), 0x40e360 and 0x40e370 (slots
// 3 and 4, to the Enemy level 0x402180 and 0x4021a0), 0x40e380 (slot 2, to 0x403660), 0x40e3e0 (slot 1, to 0x40a2d0) and
// 0x40e3f0 (slot 7, to 0x40a270).

#include "Enemy.h"

namespace Sexy
{

class Graphics;

class Clam : public Enemy
{
public:
	int						mType;					// +0xe8 constructor argument 2: the kind of clam (0, 1 or 2)
	int						mSourceY;				// +0xec the row of the picture drawn in states 1 to 3: 2, 1 or 3 frame heights for kind 0, 1, 2 (not set for another kind)
	bool					mOpened;				// +0xf0 the opening (state 1) has reached its last frame; SetState(2) clears it
	bool					mPulseUp;				// +0xf1 the pulse (states 0, 3, 4) is counting the frames up

public:
	Clam(WinWSApp* theApp, int theType);											// 0x40e100
	virtual ~Clam();																// destructor of the part 0x40e3d0, deleting 0x40e390

	virtual void			Update() override;										// slot 1, 0x40a2d0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x403660
	virtual void			SetState(int theState) override;						// slot 7, 0x40a270
};

}

#endif
