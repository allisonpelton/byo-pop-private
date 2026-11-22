// BonusEarthquake: the earthquake (0x40b7c0..0x40bfe0 in the original, 0x360 bytes). When it is won ("EARTHQUAKE") the
// loop of the quake sounds for 605 frames; the backdrop and the score board are shaken, in turn, every frame, and rocks
// fall from above the screen. Every 200 frames, up to three times, one bug that does not fly (chosen as a pea's target
// is) dies with its column. The bugs run at half speed throughout. At the end the shaken things are put back and the
// rocks that are still falling go as they land.
#pragma once

#include "Bonus.h"

namespace Sexy
{

const int QUAKE_ROCKS = 25;

class BonusEarthquake : public Bonus
{
public:
	ObjBase* mRocks[QUAKE_ROCKS]; // +0x2d8: the falling rocks, children of the level's root, each with a track that never ends
	bool mEnded;             // +0x33c: End has been called while the bonus was running
	int mCountdown;          // +0x340: frames to run (605 from Start); -99 for none
	int mUnused344[3];       // +0x344: nothing sets or reads them
	int mRocksLeft;          // +0x350: after End, the rocks that have not yet been put away (25); the last but one ends it
	float mSavedX;           // +0x354: the score board's place, kept at the start and put back at the end
	float mSavedY;           // +0x358
	int mAftershocks;        // +0x35c: the bugs killed by the quake so far (3 at most)

	BonusEarthquake();                        // 0x40b7c0
	void Setup(int theKind) override;         // 0x40b7d0
	bool Update() override;                   // 0x40b810
	void Notify(ObjBase* theObj) override;    // 0x40ba10
	void Reset() override;                    // 0x40baa0
	void End() override;                      // 0x40bb10
	void Start() override;                    // 0x40bb70

private:
	void BuildRocks();                        // 0x40bcc0
};

} // namespace Sexy
