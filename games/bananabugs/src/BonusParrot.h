// BonusParrot: the parrot helper (0x40eff0..0x410660 in the original, 0x328 bytes). The monkey holds it ("PARROT
// HELPER"), turns it to face either way with the second button and puts it down with the first: it flies in from where
// it was put to the nearest piece of wall that runs across, lands, and walks along the wall (to the end, or from the
// place it pecked). Each time a bug that is alive stands near (within a few places of the grid, ahead of it) it stops
// and pecks it: the bug dies. When the bonus ends (or the last bug is gone) the parrot flies off the screen.
#pragma once

#include "Bonus.h"

namespace Sexy
{

class Anim;
class BugObj;
class PipeObj;

class BonusParrot : public Bonus
{
public:
	bool mEnded;             // +0x2d8: End has been called (the parrot flies off)
	bool mHasTarget;         // +0x2d9: FindTarget has chosen a bug (Update starts the peck and clears it)
	bool mLanded;            // +0x2da: the parrot has landed on the wall (event 5): from then on it looks for bugs
	PipeObj* mPipe;          // +0x2dc: the piece of wall it stands at, or is flying to
	PipeObj* mNextPipe;      // +0x2e0: the piece it walks to next (the end of the wall, ahead)
	BugObj* mTarget;         // +0x2e4: the bug it is pecking, or none
	AnimKey* mFlyInKey;      // +0x2e8: the flight in from where it was put (event 4); its place and length are set by Place
	AnimKey* mFlyInEndKey;   // +0x2ec: the landing (event 5)
	AnimKey* mWalkKey;       // +0x2f0: a walk along the wall (event 8); its ends and length are set by each walk
	AnimKey* mWalkNextKey;   // +0x2f4: the same (its next is the first)
	AnimKey* mPeckKey;       // +0x2f8: the peck (the cels of its picture run on)
	AnimKey* mPeckHitKey;    // +0x2fc: the beak lands (event 9: the bug dies)
	AnimKey* mPeckEndKey;    // +0x300: the peck is over (event 10: the walk starts again)
	AnimKey* mFlyAwayKey;    // +0x304: the flight off the screen; its ends are set by End
	AnimKey* mFlyAwayEndKey; // +0x308: the end of it (event 11: the bonus is put away)
	int mHeldTrack;          // +0x30c: the parrot rests in it while it is held
	int mFlyInTrack;         // +0x310: the flight to the wall, started by Place
	int mWalkTrack;          // +0x314: the walk
	int mPeckTrack;          // +0x318: the peck
	int mFlyAwayTrack;       // +0x31c: the flight off the screen, started by End
	int mColumn;             // +0x320: the place of the grid it is over (Update)
	int mRow;                // +0x324

	static Anim* sFlyAnim;   // 0x60e1b8: "images/parrot_fly"
	static Anim* sWalkAnim;  // 0x60e1bc: "images/parrot_walk"
	static Anim* sPeckAnim;  // 0x60e1c0: "images/parrot_peck"

	BonusParrot();                            // 0x40eff0 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x40f030
	bool Update() override;                   // 0x40f060
	void Notify(ObjBase* theObj) override;    // 0x40f100
	void Reset() override;                    // 0x40f550
	void End() override;                      // 0x40f580
	bool EndWhenBugsGone() override;          // 0x412570 (the base's EndOnLastBug)
	void Start() override;                    // 0x40f690
	bool Rotate() override;                   // 0x40f630: the second button turns the parrot to face the other way
	bool Place() override;                    // 0x40f760: the parrot is put down, and flies to the wall

private:
	float FindNearestPipe(float theX, float theY); // 0x40f900: mPipe is the nearest piece of wall across; its distance
	void FindTarget();                        // 0x40f9d0 (EBX the parrot): the first live bug ahead of it, in reach
	void BuildTracks();                       // 0x40fb10 (EBX the parrot)
};

} // namespace Sexy
