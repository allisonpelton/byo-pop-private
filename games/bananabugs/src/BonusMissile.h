// BonusMissile: the missile (0x40e550..0x40eff0 in the original, 0x318 bytes). The monkey holds it ("CLICK TO SHOOT
// MISSILE") and turns it over with the second button (it points up, right, down or left); the first button fires it, if
// there is no bug right in front of it. It flies in the way it points, trailing swells, until the first bug within 75
// pixels: it bursts in a ring of 10 stars and kills every bug within 128 pixels. A missile that leaves the screen is put
// away; one that is ended while held, or after it burst, is put away 400 or 800 frames later.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;
class StarStream;

const int MISSILE_STARS = 10;

class BonusMissile : public Bonus
{
public:
	int mUnused2d8;          // +0x2d8: nothing sets or reads it
	bool mFired;             // +0x2dc: the missile has been fired, or End has been called (only Init and Setup clear it)
	int mBurstTrack;         // +0x2e0: the missile after its burst: 10 frames, the orbit's radius nothing
	int mHeldTrack;          // +0x2e4: the track the missile rests in while it is held
	int mFlightTrack;        // +0x2e8: the track it flies in; its last key's event 2 puts it away
	int mTimer;              // +0x2ec: frames until the ended or burst missile is put away (it goes when this is 1)
	StarStream* mStars[MISSILE_STARS]; // +0x2f0: the ring the burst throws out (children of the missile)

	static Anim* sMissileAnim; // 0x60e1ac: "images/missile"

	BonusMissile();                           // 0x40e550 (the base's constructor, then the vtable)
	void Init() override;                     // 0x40e590 (slot 1)
	void Setup(int theKind) override;         // 0x40e5d0
	bool Update() override;                   // 0x40e6b0
	void Reset() override;                    // 0x40e810
	void End() override;                      // 0x40e830
	void Start() override;                    // 0x40e860
	bool Place() override;                    // 0x40e920

private:
	void BuildTracks();                       // 0x40eaf0
};

} // namespace Sexy
