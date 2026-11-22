// Pea: a pea shot at a bug (0x455aa0..0x455f80 in the original). It waits hidden; fired from a place, it picks a bug
// at random and flies at it for 40 frames, following it as it moves and trailing a short swell of particles, and the
// bug it reaches is zapped. With no bug to pick it flies off the left edge.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class GridBug;

class Pea : public ObjBase
{
public:
	AnimKey* mFlightKey;     // +0x27c: from the place it was fired
	AnimKey* mArrivalKey;    // +0x280: at the bug, moved with it each frame
	GridBug* mTarget;        // +0x288: the bug flown at, or null

	Pea();                   // 0x455aa0
	void Notify(ObjBase* theObj) override; // 0x455b70
	bool Update() override;  // 0x455be0

	void Fire(float theX, float theY); // 0x455c60

private:
	void BuildTracks();      // 0x455db0
};

} // namespace Sexy
