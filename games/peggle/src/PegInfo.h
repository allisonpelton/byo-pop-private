// PegInfo.h: what makes a piece of a level a peg (the original's Sexy::PegInfo): which kind it is (the colour of the peg),
// whether it has been hit, and the counters of its flash and of crumbling away. A piece may have none. The drawing functions
// (DrawLight, DrawBigPulse, DrawBall, DrawBrick, DrawGlow) are translated with the rest of the drawing. The editor's hook
// (0x47b800) is left out.

#ifndef __PEGINFO_H__
#define __PEGINFO_H__

#include "SharedPtr.h"

namespace Sexy
{

class DataSync;
class PhysObj;

class PegInfo : public RefCounted
{
public:
	PegInfo();		   // 0x4765f0
	~PegInfo() override {}

	void SyncState(DataSync& theSync);	   // 0x479560
	void Update(PhysObj* thePiece);		   // 0x479680: moves the counters on; a crumbling peg's piece goes inactive at the end
	void SetHit(bool theHit);			   // 0x476650
	void DoFlash(int theFlashType);		   // 0x476670: starts a flash
	void DoFlashBlink(int theBlinks);	   // 0x476690: starts a flash of that many blinks

	int mKind;			// +0x10 (peg_type): 1 blue, 2 orange (to clear), 3 green, 4 purple; 0 for a piece that is not a peg
	bool mHit;			// +0x14: the ball has touched it
	bool mVariable;		// +0x15 (peg_variable): true unless the peg's kind is fixed
	bool mCrumble;		// +0x16 (peg_crumble): the peg crumbles away after it is hit
	bool mUnknown17;	// +0x17: drawn with the flower when set (see DrawLight)
	int mHitTime;		// +0x18: updates since the hit, counting from 1; 0 until it is hit
	int mCrumbleTime;	// +0x1c: how far the crumbling has got, from 3 up to 100, when the piece goes inactive
	int mFlashTime;		// +0x20: how far the flash has got, 0 for none
	int mFlashType;		// +0x24: 10 is a blink, which counts down; the other kinds count up to 100
};

} // namespace Sexy

#endif
