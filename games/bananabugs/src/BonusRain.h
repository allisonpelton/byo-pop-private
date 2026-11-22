// BonusRain: the rainstorm (0x410660..0x410ca0 in the original, 0x3bc bytes). When it is won ("RAINSTORM") the backdrop
// darkens, the sound of rain loops for 700 frames, 50 drops fall across the screen (each a milk drop picture, falling by
// its own orbit radius and starting again from above when it lands) and the bugs are slowed (they skip three updates
// in four). At the end the backdrop is put back and the drops that are still falling go as they land.
#pragma once

#include "Bonus.h"

namespace Sexy
{

const int RAIN_DROPS = 50;

class BonusRain : public Bonus
{
public:
	ObjBase* mDrops[RAIN_DROPS]; // +0x2d8: the falling drops, children of the level's root, each with a track that never ends
	bool mEnded;             // +0x3a0: End has been called while the bonus was running
	int mCountdown;          // +0x3a4: frames to run (700 from Start); -99 for none
	int mUnused3a8[3];       // +0x3a8: nothing sets or reads them
	bool mDarkened;          // +0x3b4: the backdrop has been darkened by this bonus
	int mDropsLeft;          // +0x3b8: after End, the drops that have not yet been put away (50); the last but one ends it

	BonusRain();                              // 0x410660 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x410690
	bool Update() override;                   // 0x4106d0
	void Notify(ObjBase* theObj) override;    // 0x410720
	void Reset() override;                    // 0x4107b0
	void End() override;                      // 0x410830
	void Start() override;                    // 0x4108c0

private:
	void BuildDrops();                        // 0x4109f0
};

} // namespace Sexy
