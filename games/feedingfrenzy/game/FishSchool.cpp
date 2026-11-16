#include "FishSchool.h"
#include "FishBase.h"
#include "FishFood.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// How far outside the stage a school's destination is when the school leaves
const float LEAVE_DISTANCE = 150.0f;
// The seconds a school keeps a destination it leaves by
const float LEAVE_SECONDS = 10000.0f;
// How far in from the stage's edges the school's destinations are
const float DESTINATION_MARGIN = 20.0f;
const float LEAVE_DEPTH_MARGIN = 40.0f;
// The squared speed under which a school's fish do not follow the others' heading
const float LEAST_ALIGNMENT_SPEED_SQUARED = 30.0f;
// The squared distance that no fish is farther than
const float FARTHEST_SQUARED = 1e+08f;

// 0x41d6f0
FishSchool::FishSchool()
    : mSeparationDistance(20.0f), mSeparationDistanceSquared(400.0f), mNeighbourDistance(0.0f), mNeighbourDistanceSquared(0.0f),
      mDestinationSeconds(-1.0f), mDestinationSecondsRange(1.0f, 3.0f), mSeparationMultiplier(0.0f),
      mAlignmentMultiplier(0.0f), mCohesionDivisor(0.0f), mTargetMultiplier(0.0f), mChaseMultiplier(0.0f),
      mRunMultiplier(0.0f), mAliveCount(0)
{
}

// 0x41d010
void FishSchool::setMultipliers(float theSeparation, float theAlignment, float theCohesionDivisor, float theTarget,
                                float theChase, float theRun)
{
	mSeparationMultiplier = theSeparation;
	mAlignmentMultiplier = theAlignment;
	mCohesionDivisor = theCohesionDivisor;
	mTargetMultiplier = theTarget;
	mRunMultiplier = theRun;
	mChaseMultiplier = theChase;
}

// 0x41cff0
void FishSchool::setSeparationDistance(float theDistance)
{
	mSeparationDistance = theDistance;
	mSeparationDistanceSquared = theDistance * theDistance;
}

// 0x41d060
void FishSchool::setNeighbourDistance(float theDistance)
{
	mNeighbourDistance = theDistance;
	mNeighbourDistanceSquared = theDistance * theDistance;
}

// 0x41d0b0
void FishSchool::recalculate()
{
	mMeanVelocity.set(0.0f, 0.0f);
	mCentre.set(0.0f, 0.0f);
	for (std::vector<FishBase*>::iterator anIt = mFish.begin(); anIt != mFish.end(); ++anIt)
	{
		mCentre.add((*anIt)->getNosePosition());
		mMeanVelocity.add((*anIt)->getVelocity());
	}

	float aCount = (float)mFish.size();
	if (0.0f < aCount)
	{
		mCentre.scale(1.0f / aCount);
		mMeanVelocity.scale(1.0f / aCount);
	}
}

// 0x41d210
FishBase* FishSchool::findNearest(FishBase* theFish)
{
	Point aNose = theFish->getNosePosition();
	float aNearest = FARTHEST_SQUARED;
	FishBase* aResult = nullptr;
	if (mFish.size() > 1)
	{
		for (std::vector<FishBase*>::iterator anIt = mFish.begin(); anIt != mFish.end(); ++anIt)
		{
			if (*anIt != theFish)
			{
				Point aDifference = (*anIt)->getTailPosition();
				aDifference.subtract(aNose);
				float aDistance = aDifference.lengthSquared();
				if (aDistance < aNearest)
				{
					aResult = *anIt;
					aNearest = aDistance;
				}
			}
		}
		return aResult;
	}
	return nullptr;
}

// 0x41d350
void FishSchool::averageNeighbours(FishBase* theFish, Point& theVelocity, Point& thePosition)
{
	Point aNose = theFish->getNosePosition();
	Point aVelocitySum;
	Point aPositionSum;
	int aCount = 0;
	for (std::vector<FishBase*>::iterator anIt = mFish.begin(); anIt != mFish.end(); ++anIt)
	{
		if (*anIt != theFish)
		{
			Point aDifference = (*anIt)->getTailPosition();
			aDifference.subtract(aNose);
			if (aDifference.lengthSquared() < mNeighbourDistanceSquared)
			{
				aVelocitySum.add((*anIt)->getVelocity());
				aPositionSum.add((*anIt)->getNosePosition());
				aCount++;
			}
		}
	}

	if (0 < aCount)
	{
		float anInverse = 1.0f / (float)aCount;
		aVelocitySum.scale(anInverse);
		aPositionSum.scale(anInverse);
	}

	if (aVelocitySum.lengthSquared() < LEAST_ALIGNMENT_SPEED_SQUARED)
		aVelocitySum.scale(0.0f);

	theVelocity = aVelocitySum;
	thePosition = aPositionSum;
}

// 0x41d540
void FishSchool::resetDestination()
{
	recalculate();
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	int aWorldWidth = aStage->getWorldWidth();
	if ((float)(aWorldWidth / 2) <= mCentre.x)
		mDestination.x = -LEAVE_DISTANCE;
	else
		mDestination.x = (float)aWorldWidth + LEAVE_DISTANCE;
	int aWorldHeight = aStage->getWorldHeight();
	mDestination.y = randomFloat(Point(LEAVE_DEPTH_MARGIN, (float)aWorldHeight - LEAVE_DEPTH_MARGIN));
	mDestinationSeconds = LEAVE_SECONDS;
}

// 0x41d5d0
void FishSchool::removeFish(FishBase* theFish)
{
	std::vector<FishBase*>::iterator anIt = mFish.begin();
	while (anIt != mFish.end())
	{
		if (*anIt == theFish)
		{
			(*anIt)->setSchool(nullptr);
			anIt = mFish.erase(anIt);
		}
		else
		{
			++anIt;
		}
	}

	if (mFish.empty())
		FishFood::instance()->getStageMgr()->removeSchool(this);
}

// 0x41d650
void FishSchool::update(UpdateInfo& theInfo)
{
	recalculate();
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	mDestinationSeconds -= theInfo.mSeconds;
	if (mDestinationSeconds < 0.0f)
	{
		int aWorldWidth = aStage->getWorldWidth();
		mDestination.x = randomFloat(Point(DESTINATION_MARGIN, (float)aWorldWidth - DESTINATION_MARGIN));
		int aWorldHeight = aStage->getWorldHeight();
		mDestination.y = randomFloat(Point(0.0f, (float)aWorldHeight - DESTINATION_MARGIN));
		mDestinationSeconds = randomFloat(mDestinationSecondsRange);
	}
}

// 0x41d7b0
void FishSchool::addFish(FishBase* theFish)
{
	mFish.push_back(theFish);
	theFish->setSchool(this);
	mAliveCount++;
}

} // namespace feedingfrenzy
