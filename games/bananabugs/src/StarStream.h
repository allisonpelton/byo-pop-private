// StarStream: a star thrown out of a burst (0x46c9d0..0x46d2b0 in the original), a cel of the particles picture that
// flies off in a direction on decaying speeds as it grows and shows, trailing sparks or dust of its kind.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

// The kinds of star stream: their trail and how they fly
enum StarStreamKind
{
	STAR_STREAM_BLUE = 1,       // blue sparks every third frame (the fireworks')
	STAR_STREAM_DUST = 2,       // dust every third frame
	STAR_STREAM_BIG_DUST = 3,   // dust every third frame, faster and larger
	STAR_STREAM_SLOW_BLUE = 4,  // blue sparks every fifth frame, falling slowly
	STAR_STREAM_RED = 5,        // red sparks every fourth frame (the fireballs')
};

class StarStream : public ObjBase
{
public:
	AnimKey* mFlightKey;     // +0x27c
	int mKind;               // +0x280: a StarStreamKind
	int mTrailInterval;      // +0x284: the frames between puffs of the trail

	explicit StarStream(int theKind); // 0x46c9d0
	void Notify(ObjBase* theObj) override {} // 0x42c290, empty
	bool Update() override;  // 0x46cb50

	void Burst(float theX, float theY, int theDegrees); // 0x46cbf0

private:
	void BuildTrack();       // 0x46ce00
};

} // namespace Sexy
