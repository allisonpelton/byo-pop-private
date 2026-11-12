// The cupid: it flies in when the player has waited too long, looks for a jewel in the air, catches it and carries it to the
// highest slot of its colour that is open, and drops it there. It makes up to three catches and then flies away.

#ifndef VENICE_CUPID_H
#define VENICE_CUPID_H

#include "SceneObject.h"

class Slot;

// 0x15c bytes in the original
class Cupid : public SceneObject
{
public:
	Slot* mTarget = nullptr; // 0x14c: the slot the cupid is taking its jewel to (none while it waits for a jewel)
	int mCatches = 0; // 0x154: the jewels the cupid has caught
	int mWaitTicks = 0; // 0x158: the updates the cupid has waited for a jewel

	Cupid(); // 0x403520
	bool Update() override; // 0x403890
	void OnCustomEvent() override; // 0x403f50: the cupid has reached its slot and lets go of the jewel

private:
	void CatchJewel(); // 0x403a00: a jewel that was shot and is within reach of the cupid is caught
	void UpdateWaiting(); // 0x403bf0
	Slot* FindTarget(int theKind); // 0x403cd0: the highest open slot of the colour (any slot for a wild jewel)
	void UpdateSearching(); // 0x403e20: the cupid's flight follows its slot
	void FaceTarget(); // 0x403910: the cupid turns to the way it flies
};

#endif
