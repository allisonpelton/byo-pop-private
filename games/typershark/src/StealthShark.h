#ifndef __TYPERSHARK_STEALTHSHARK_H__
#define __TYPERSHARK_STEALTHSHARK_H__

// StealthShark.cpp: a shark that swims in from the right half transparent (mStealthy of Enemy is "stealth on") and
// turns solid for good when it passes x = 400 or is hit. While stealthy it is faster than mBaseSpeed and the word is
// drawn with alpha mAlpha. StealthBlackShark and StealthRedShark take it together with BlackShark or RedShark.
// Original: class Sexy::StealthShark : Shark (virtual base; so are Enemy and GameObject), vftable 0x5299d0 (own part,
// 2 virtuals), 0x52999c (GameObject part at +0x38), 0x52998c (Enemy part at +0x130), 0x529984 (the Shark part at
// +0x218), sizeof 0x248; the constructor is 0x410770, the deleting destructor 0x409a10. The field comments give the
// offsets in the original's own part (vptr at +0, vbptr at +8).
//
// port: Shark is a virtual base, as in the original, because StealthBlackShark and StealthRedShark reach it twice.
// StealthPiranha has the same own part and the same four small functions; the linker folded their code into one
// copy (0x404f60, 0x408480, 0x408500, 0x401ab0), so each class here has its own copy of them.

#include "Shark.h"
#include <vector>

namespace Sexy
{

class Graphics;

class StealthShark : public virtual Shark
{
public:
	double					mBaseSpeed;				// +0x10 ctor 1.0; SetVelX stores the speed here, GetSpeed returns it
	bool					mEventSent;				// +0x18 event 8 was pushed (x passed 600)
	int						mAlpha;					// +0x1c ctor 50: the alpha of the word's colours while stealthy
	std::vector<Image*>		mStealthImages;			// +0x20 six slots, the ctor fills 0 (swim), 2 (death), 3 (mirrored swim)

public:
	StealthShark(WinWSApp* theApp);													// 0x410770
	virtual ~StealthShark();														// deleting destructor 0x409a10

	virtual void			SetStealth(bool theStealth);							// own slot 0, 0x404f60
	virtual void			ResetSpeed();											// own slot 1, 0x408480: mVelX = mBaseSpeed

	virtual void			Update() override;										// GameObject slot 1, 0x40c0a0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4029a0
	virtual void			SetState(int theState) override;						// slot 7, 0x404f90
	virtual void			SetVelX(double v) override;								// slot 9, 0x401ab0
	virtual double			GetSpeed() override;									// Enemy slot 1, 0x408500
};

}

#endif
