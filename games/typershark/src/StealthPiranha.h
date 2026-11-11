#ifndef __TYPERSHARK_STEALTHPIRANHA_H__
#define __TYPERSHARK_STEALTHPIRANHA_H__

// StealthPiranha.cpp: a piranha that swims in from the right half transparent (mStealthy of Enemy is "stealth on") and
// turns solid for good when it passes x = 400 or is hit; StealthShark's behaviour on a piranha (the same own part).
// StealthWhitePiranha takes it together with WhitePiranha.
// Original: class Sexy::StealthPiranha : Piranha (virtual base; so are Enemy and GameObject), vftable 0x529a28 (own
// part, 2 virtuals), 0x5299f4 (GameObject part at +0x38), 0x5299e4 (Enemy part at +0x130), 0x5299dc (the Piranha part
// at +0x218), sizeof 0x248; the constructor is 0x410530, the deleting destructor 0x409ab0. The field comments give
// the offsets in the original's own part (vptr at +0, vbptr at +8).
//
// port: Piranha is a virtual base, as in the original, because StealthWhitePiranha reaches it twice. SetStealth,
// ResetSpeed, GetSpeed and SetVelX are the same code as StealthShark's (the linker folded the two copies).

#include "Piranha.h"
#include <vector>

namespace Sexy
{

class Graphics;

class StealthPiranha : public virtual Piranha
{
public:
	double					mBaseSpeed;				// +0x10 ctor 1.6; SetVelX stores the speed here, GetSpeed returns it
	bool					mEventSent;				// +0x18 event 8 was pushed (x passed 610)
	int						mAlpha;					// +0x1c ctor 50: the alpha of the word's colour while stealthy
	std::vector<Image*>		mStealthImages;			// +0x20 six slots, the ctor fills 0 (swim), 2 (death), 3 (mirrored swim)

public:
	StealthPiranha(WinWSApp* theApp, int theType);									// 0x410530
	virtual ~StealthPiranha();														// deleting destructor 0x409ab0

	virtual void			SetStealth(bool theStealth);							// own slot 0, 0x404f60
	virtual void			ResetSpeed();											// own slot 1, 0x408480: mVelX = mBaseSpeed

	virtual void			Update() override;										// GameObject slot 1, 0x40bfa0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4027e0
	virtual void			SetState(int theState) override;						// slot 7, 0x404ed0
	virtual void			SetVelX(double v) override;								// slot 9, 0x401ab0
	virtual double			GetSpeed() override;									// Enemy slot 1, 0x408500
};

}

#endif
