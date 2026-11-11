#ifndef __JB_ANIMATION_H__
#define __JB_ANIMATION_H__

// A strip animation attached to an object (vtable 0x51e068): the frames of an image side by side, stepped every
// period. It swings from a middle frame out to a random frame on either side and back (Setup), waiting a delay at
// the outer frame and, unless set, a random one between 250 and 1250 ms at the start; it stops after the cycles it
// is given.

#include "Object.h"

namespace jb
{

class Animation : public Attachment
{
public:
	int mPeriod = 0;         // +0x08 ms per frame
	int mFrame = 0;          // +0x0c
	int mDelay = 0;          // +0x10 ms to wait before stepping
	int mTarget = -1;        // +0x1c the outer frame
	int mCycles = 0;         // +0x20 left until the delay (-1 for ever)
	int mMiddle = 0;         // +0x24 the middle frame
	int mBelow = 0;          // +0x28 the outer frame is at least this far below the middle
	int mAbove = 0;          // +0x2c and at least this far above it
	bool mForward = false;   // +0x30
	bool mRunning = false;   // +0x31
	uint32_t mLast = 0;      // +0x34 the clock of the last step
	int mHoldStart = 0;      // +0x38
	bool mHolding = false;   // +0x3c
	int mImage = 0;          // +0x40
	bool mPaused = false;    // +0x51
	bool mDrawDelayed = true;   // +0x53 drawn while it waits a delay
	int mMinDelay = 0xfa;    // +0x54
	int mMaxDelay = 0x4e2;   // +0x58
	int mX = 0, mY = 0;      // +0x5c, +0x60
	int mFrameWidth = 0;     // +0x64
	int mHeight = 0;         // +0x68
	int mFrames = 0;         // +0x70
	int mAlpha = 0xff;       // +0x74
	int mSwingDelay = 0;     // +0x7c the wait at the outer frame

	// 0x425640
	Animation() {}
	// 0x4256c0
	~Animation() override {}

	bool Update(int theClock) override;           // 0x4258a0
	void Draw() override;                         // 0x425990
	void SetPaused(bool thePaused) override;      // 0x425810

	// 0x4256d0: the image at (theX, theY) of the owner, in frames theFrameWidth wide
	void SetImage(int theX, int theY, int theImage, int theFrameWidth);
	// 0x425720: from a random frame
	void Setup(int thePeriod, int theMiddle, int theBelow, int theAbove, int theSwingDelay, int theCycles,
		int theDelay, bool theDrawDelayed);
	// 0x4257c0
	void Start();
	// 0x425840: the time it is held or paused does not count
	void UpdateHold();
	// 0x425aa0
	void StepSwing();
	// 0x425b20: a cycle ended at theFrame (-1: the frame stays); false when it stopped
	bool EndCycle(int theFrame);
	// 0x425b70: -1 for a random delay
	void SetDelay(int theDelay);
};

} // namespace jb

#endif
