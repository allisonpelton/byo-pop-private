#include "FishAttachFx.h"
#include "FishBase.h"
#include "FishFood.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// 0x41ad50
FishAttachFx::FishAttachFx() : mFish(nullptr), mStopped(false), mInMidGround(false)
{
}

// 0x41ac10
void FishAttachFx::init(const std::string& theFile, FishBase* theFish, const Point& theOffset, bool theInMidGround)
{
	if (theFish == nullptr)
		return;

	mParticles = new ParticleSystem();
	addChild(mParticles);
	mParticles->load(theFile);
	mOffset = theOffset;
	mFish = theFish;
	clearFlags(NODE_LISTENERS);
	mInMidGround = theInMidGround;
}

// 0x41aab0
void FishAttachFx::start()
{
	Point aPlace = *mFish->getPosition();
	aPlace.add(mOffset);
	setPosition(aPlace);
	setFlags(NODE_LISTENERS);

	StageMgr* aStage = FishFood::instance()->getStageMgr();
	if (mInMidGround)
		aStage->addToMidGround(this);
	else
		aStage->addToBehind(this);
	Container* aPlayGroup = aStage->getPlayGroup();
	for (size_t aStream = 0; aStream < mParticles->mChildren.size(); aStream++)
		mParticles->setStreamTarget((int)aStream, aPlayGroup);
	mParticles->startEmitting();
	mStopped = false;
}

// 0x41aba0
void FishAttachFx::stop()
{
	mParticles->stopEmitting();
	mStopped = true;
}

// 0x41a9f0
void FishAttachFx::update(UpdateInfo& theInfo)
{
	if (!mStopped)
	{
		Point aPlace = *mFish->getPosition();
		aPlace.add(mOffset);
		setPosition(aPlace);
	}
	else if (mParticles->isDone())
	{
		clearFlags(NODE_LISTENERS);
	}
	Container::update(theInfo);
}

} // namespace feedingfrenzy
