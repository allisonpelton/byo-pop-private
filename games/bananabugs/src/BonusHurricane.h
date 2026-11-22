// BonusHurricane: the hurricane (0x40d090..0x40d740 in the original, 0x3c0 bytes). When it is won ("HURRICANE") the
// backdrop darkens, the wind loops for 601 frames and 50 pieces of the pipes' picture blow across the screen from the
// left, each over and over; every 250 frames, up to three times, one bug at random is blown off the screen. The bugs
// run at half speed throughout. At the end the backdrop is put back and the pieces that are still blowing go as they
// leave the screen.
#pragma once

#include "Bonus.h"

namespace Sexy
{

const int HURRICANE_LEAVES = 50;

class BonusHurricane : public Bonus
{
public:
	ObjBase* mLeaves[HURRICANE_LEAVES]; // +0x2d8: the blowing pieces, children of the level's root, each with a track that never ends
	bool mEnded;             // +0x3a0: End has been called while the bonus was running
	int mCountdown;          // +0x3a4: frames to run (601 from Start); -99 for none
	int mUnused3a8[3];       // +0x3a8: nothing sets or reads them
	bool mDarkened;          // +0x3b4: the backdrop has been darkened by this bonus
	int mLeavesLeft;         // +0x3b8: after End, the pieces that have not yet been put away (50); the last but one ends it; -99 from Start
	int mBugsBlown;          // +0x3bc: the bugs blown off so far (3 at most)

	BonusHurricane();                         // 0x40d090 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x40d0a0
	bool Update() override;                   // 0x40d0e0
	void Notify(ObjBase* theObj) override;    // 0x40d160
	void Reset() override;                    // 0x40d1f0
	void End() override;                      // 0x40d290
	bool EndWhenBugsGone() override;          // 0x412570 (the base's EndOnLastBug)
	void Start() override;                    // 0x40d320

private:
	void BuildLeaves();                       // 0x40d490
};

} // namespace Sexy
