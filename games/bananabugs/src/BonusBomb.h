// BonusBomb: the coconut bomb (0x4080c0..0x408ed0 in the original, 0x364 bytes). The monkey holds it ("CLICK TO DROP
// COCOBOMB"), turns it with the second button and drops it with the first: it is thrown a little way in the way it was
// turned, and bursts, killing every bug within 138 pixels, in a spray of 32 drops.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;

const int BOMB_DROPS = 32;

class BonusBomb : public Bonus
{
public:
	bool mDropped;           // +0x2d8: the bomb has been dropped, or End has been called
	int mHeldTrack;          // +0x2dc: the track the bomb rests in while it is held
	int mDropTrack;          // +0x2e0: the track it is thrown in, which ends with it bursting
	ObjBase* mDrops[BOMB_DROPS]; // +0x2e4: the spray, each a child of the bomb

	static Anim* sCocoBombAnim; // 0x60e1a8
	static Anim* sMilkDropAnim; // 0x60e1c8: "images/bonus_milkdrop", the picture of the bomb's drops and of the rain's (BonusRain reads it too)

	BonusBomb();                              // 0x4080c0
	void Setup(int theKind) override;         // 0x408100
	bool Update() override;                   // 0x40c760 (in the second half's range; the same as BonusFlyPaper's)
	void Notify(ObjBase* theObj) override;    // 0x408130
	void Reset() override;                    // 0x4081d0
	void End() override;                      // 0x4081f0
	void Start() override;                    // 0x408210
	bool Place() override;                    // 0x4082c0

private:
	void Burst();                             // 0x4083c0 (the bomb on the stack)
	void BuildDrops();                        // 0x408420
	void BuildTracks();                       // 0x408760
};

} // namespace Sexy
