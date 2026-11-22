// BonusZapper: the bug zapper (0x4123e0..0x412f20 in the original, 0x2e4 bytes). The monkey holds it ("CLICK TO DROP
// ZAPPER"), turns it with the second button and drops it with the first: it lies on the board and its light flickers;
// its zap area (80 by 100 pixels around it) kills the bugs that walk into it. When the bonus ends the light fades out and
// the zapper is put away (event 7).
#pragma once

#include "BonusDropper.h"

namespace Sexy
{

class Anim;

class BonusZapper : public BonusFadingDropper
{
public:
	// +0x2d8 mHeldTrack, +0x2dc mDropTrack, +0x2e0 mEndTrack (BonusFadingDropper)

	static Anim* sZapperAnim; // 0x60e1d8: "images/bonus_zapper"

	BonusZapper();                            // 0x4123e0 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x412420
	bool Update() override;                   // 0x412440
	void Notify(ObjBase* theObj) override;    // 0x412480
	void Reset() override;                    // 0x412530 (shared with BonusSwat: Bonus::DropAndReset)
	void End() override;                      // 0x412550: the light fades out (only when the bonus is running)
	bool EndWhenBugsGone() override;          // 0x412570 (the base's EndOnLastBug)
	void Start() override;                    // 0x412580
	bool Place() override;                    // 0x412670: put down at once, with no sound

private:
	void BuildTracks();                       // 0x412770
};

} // namespace Sexy
