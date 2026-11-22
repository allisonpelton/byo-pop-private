// BananaBonus: the banana bunches a level gives out (0x4048b0..0x405770 in the original). Eight bunches are made once
// and taken in turn: a bunch appears where it was won, springs out 48 pixels in a random direction, lifts a little and
// flies off to the banana counter at the top, where it bursts in a ring of stars and adds the bunch's bananas. Each
// bunch has a glowing corona behind it.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;

const int BANANA_BUNCH_COUNT = 8;

class BananaBonus : public ObjBase
{
public:
	ObjBase* mBunches[BANANA_BUNCH_COUNT];   // +0x27c
	ObjBase* mCoronas[BANANA_BUNCH_COUNT];   // +0x29c: each is its bunch's child
	AnimKey* mStartKeys[BANANA_BUNCH_COUNT]; // +0x2c0: a bunch's first key, at the place it was won
	AnimKey* mScatterKeys[BANANA_BUNCH_COUNT]; // +0x2e0: the place it springs out to
	AnimKey* mLiftKeys[BANANA_BUNCH_COUNT];  // +0x300: 8 pixels above that, where its flight starts
	int mNextBunch;                          // +0x324: the bunch the next Spawn takes

	static Anim* sBunchAnim;                 // 0x60e1a0
	static Anim* sCoronaAnim;                // 0x60e1a4

	BananaBonus();                           // 0x4048b0
	void Notify(ObjBase* theObj) override;   // 0x404950
	// The original's Update (0x4219c0, shared with other classes) only returns ObjBase's

	void Spawn(float theX, float theY);      // 0x4049a0

private:
	void BuildBunches();                     // 0x404b00
};

void PreloadBananas(); // 0x405730

} // namespace Sexy
