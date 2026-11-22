// BonusMine: the mine (0x40d7c0..0x40e550 in the original, 0x380 bytes). The monkey holds it ("CLICK TO DROP MINE"),
// turns it with the second button and drops it with the first: it is thrown a little way in the way it was turned and
// buries itself (event 5: armed). The first bug within 70 pixels of an armed mine sets it off: it swells, bursts (event
// 7) killing every bug within 256 pixels in a ring of 36 stars, and is put away 340 frames later (event 8).
#pragma once

#include "BonusDropper.h"

namespace Sexy
{

class Anim;
class StarStream;

const int MINE_STARS = 36;

class BonusMine : public BonusDropper
{
public:
	int mTriggerTrack;       // +0x2e0: the mine swells before it bursts; its last key's event 7 is the burst
	int mBurstTrack;         // +0x2e4: after the burst: the mine waits 340 frames; its last key's event 8 puts the bonus away
	bool mArmed;             // +0x2e8: the mine has landed (event 5) and is waiting for a bug
	int mUnused2ec;          // +0x2ec: nothing sets or reads it
	StarStream* mStars[MINE_STARS]; // +0x2f0: the ring the burst throws out (children of the mine)

	static Anim* sMineAnim;  // 0x60e1e0: "images/bonus_mine"

	BonusMine();                              // 0x40d7c0 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x40d810
	bool Update() override;                   // 0x40d8b0
	void Notify(ObjBase* theObj) override;    // 0x40d910
	void Reset() override;                    // 0x40da10
	void Start() override;                    // 0x40da30

private:
	void BuildTracks();                       // 0x40dc30
};

} // namespace Sexy
