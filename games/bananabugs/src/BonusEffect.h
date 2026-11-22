// BonusEffect: what is shown where a bonus is won (0x40bfe0..0x40c700 in the original, 0x2c4 bytes): a ring of 18
// small stars (cel 2 of the particles picture) that fly out from the place, swell and spin, and fade. The coin has one
// of its own, and the monkey has one (Player::mBonusEffect) for a coin that flew to it. Every star has the same
// track; the effect is invisible and inactive until it is played.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

const int EFFECT_PICTURES = 18;

class BonusEffect : public ObjBase
{
public:
	ObjBase* mPictures[EFFECT_PICTURES]; // +0x27c: the stars, children of the effect, 20 degrees apart

	BonusEffect();                       // 0x40bfe0
	void Play();                         // 0x40c040 (EBX the effect): starts its 18 pictures and shows it

private:
	void BuildPictures();                // 0x40c080
};

} // namespace Sexy
