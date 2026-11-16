#include "ChaserFish.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The player is chased when the squared distance to it is under this multiple of the squared sight (the sight's twice)
const double CHASE_RANGE_FACTOR = 4.0;

// 0x406100
ChaserFish::ChaserFish() : mChaseSubject(nullptr) {}

// 0x4060c0
void ChaserFish::stun(float theSeconds, FishBase* theSource, bool theShowEffect)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theSource == aPlayer)
		mTarget = theSource;
	AIFish::stun(theSeconds, theSource, theShowEffect);
}

// 0x405f50
void ChaserFish::decide()
{
	if (mChaseSubject != nullptr && mChaseSubject->isInPlay())
	{
		mTarget = mChaseSubject;
		beginChase();
	}
	else
	{
		PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
		Point aToPlayer = aPlayer->getNosePosition();
		aToPlayer.subtract(mNose);
		if (mState == AI_CHASE || (double)mVisionRange * mVisionRange * CHASE_RANGE_FACTOR <= aToPlayer.lengthSquared())
		{
			beginWander();
		}
		else
		{
			mTarget = aPlayer;
			beginChase();
		}
	}
	mChaseSubject = nullptr;
}

} // namespace feedingfrenzy
