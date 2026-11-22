// FireWorks: one rocket of the fireworks (0x42b410..0x42b8f0 in the original). Launched from a place, it rises at a
// random steep angle and speed, trailing dust, until it climbs past a random height; then it
// bursts in a ring of stars and a rising swell, and two times in nine also in eight star streams around it.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class StarStream;

const int FIREWORKS_STAR_COUNT = 8;

class FireWorks : public ObjBase
{
public:
	StarStream* mStars[FIREWORKS_STAR_COUNT]; // +0x27c: the rocket's children
	bool mRising;            // +0x29c: the rocket trails dust
	AnimKey* mRiseKey;       // +0x2a0
	AnimKey* mBurstKey;      // +0x2a4
	int mTrackIndex;         // +0x2ac

	FireWorks();             // 0x42b410
	void Notify(ObjBase* theObj) override; // 0x42b6b0
	bool Update() override;  // 0x42b770

	void Launch(float theX, float theY); // 0x42b550

private:
	void BuildTrack();       // 0x42b7b0
};

} // namespace Sexy
