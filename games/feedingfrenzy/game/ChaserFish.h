#ifndef __FEEDINGFRENZY_CHASERFISH_H__
#define __FEEDINGFRENZY_CHASERFISH_H__

// A fish that chases the player when the player comes near (ChaserFish, 0x390 bytes, constructor 0x406100, vtable
// 0x49f2a8, destructor 0x405f20).

#include "AIFish.h"

namespace feedingfrenzy
{

class ChaserFish : public AIFish
{
public:
	ChaserFish();  // 0x406100

	// slot 0x1bc: 0x4060c0: the player that stuns it is the fish it chases when it wakes
	void stun(float theSeconds, FishBase* theSource, bool theShowEffect) override;
	// slot 0x284: 0x405f50: it chases the fish it was set on if that is in play, else the player if the player is near,
	// else it wanders
	void decide() override;

private:
	FishBase* mChaseSubject;  // +0x384: the fish it chases first; nothing sets it, and a decision clears it
};

} // namespace feedingfrenzy

#endif
