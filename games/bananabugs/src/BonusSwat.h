// BonusSwat: the fly swatter (0x410e10..0x411c60 in the original, 0x300 bytes). The monkey holds it ("CLICK TO SWAT
// BUGS"), turns it with the second button and, with the first, swings it (three times): at the bottom of the swing
// (event 5) every bug within 96 pixels of a place 110 pixels from the swatter, in the way it was turned, dies. The
// bonus ends when the last swing is used (or when a swing kills a bug) and the swatter fades out (event 6: put away).
#pragma once

#include "BonusDropper.h"

namespace Sexy
{

class Anim;

class BonusSwat : public BonusFadingDropper
{
public:
	// +0x2d8 mHeldTrack: the swatter appears and sways for ever; +0x2dc mDropTrack: the swing (the key list is in
	// BuildTracks); +0x2e0 mEndTrack: the fade out (BonusFadingDropper)
	int mUnused2e4;          // +0x2e4: nothing sets or reads it
	AnimKey* mSwingKeys[6];  // +0x2e8: the first six keys of the swing; nothing reads them after the tracks are built

	static Anim* sSwatterAnim; // 0x60e1dc: "images/bonus_swatter"

	BonusSwat();                              // 0x410e10 (the base's constructor, then the vtable)
	void Setup(int theKind) override;         // 0x410e50
	// 0x410e70 is only the base's Update, so it is not overridden
	void Notify(ObjBase* theObj) override;    // 0x410e80
	void Reset() override;                    // 0x412530 (shared with BonusZapper: Bonus::DropAndReset)
	void Start() override;                    // 0x410f90
	bool Place() override;                    // 0x411080: the swing

private:
	void BuildTracks();                       // 0x4110c0
};

} // namespace Sexy
