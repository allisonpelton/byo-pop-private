// BonusCannon: the cannon (0x407170..0x4080c0 in the original, 0x2ec bytes). The monkey holds it ("CLICK TO FIRE
// CANNON"), turns it a quarter with the second button and puts it down on the board with the first: the cannon takes
// its place, recoils, and fires its ball in the way it was turned. The ball flies across the screen until it leaves it,
// and every bug it passes within 32 pixels dies in a burst of gold. Four hundred frames after the bonus has ended the
// cannon is put away.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;

class BonusCannon : public Bonus
{
public:
	ObjBase* mCannon;        // +0x2d8: the picture of the cannon, a child of the bonus (the bonus itself is the ball)
	bool mEnded;             // +0x2dc: End has been called (only Init and Setup clear it)
	int mEndCountdown;       // +0x2e0: frames left after End until the bonus is put away
	int mBallTrack;          // +0x2e4: the track the ball rests in while the cannon is held
	int mFireTrack;          // +0x2e8: the track the ball flies in

	static Anim* sCannonAnim;     // 0x60e1b0
	static Anim* sCannonBallAnim; // 0x60e1b4

	BonusCannon();                            // 0x407170 (the base's constructor, then the vtable)
	void Init() override;                     // 0x4071b0 (slot 1)
	void Setup(int theKind) override;         // 0x407300
	bool Update() override;                   // 0x4073b0
	void Notify(ObjBase* theObj) override;    // 0x407440
	void Reset() override;                    // 0x407480
	void End() override;                      // 0x4074a0
	void Start() override;                    // 0x4074d0
	bool Place() override;                    // 0x407570

private:
	void ResetLook();                         // 0x4071f0 (EDI the bonus)
	void Fire();                              // 0x407710
	void BuildCannon();                       // 0x4077f0
	void BuildBall();                         // 0x407c40
};

} // namespace Sexy
