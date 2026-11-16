#include "Copter.h"
#include "CityMap.h"
#include "GameScreen.h"
#include "ImageNode.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const char* const VEHICLE_CLASS = "vehicle";
const char* const SOUND_LEAVE_COPTER = "vehicle_leave_copter";
const int SMOOTH_TYPE = 1;

// the lift takes this many seconds and raises the pictures this many pixels
const float LIFT_SECONDS = 0.5f;
const float LIFT_HEIGHT = 100.0f;
}

// 0x431dc0
Copter::Copter(const std::string& theTypeName) : Van(theTypeName), mLiftTime(0.0f)
{
}

// 0x432670
void Copter::loadType()
{
	Vehicle* aType = static_cast<Vehicle*>(PizzaFrenzy::instance()->mTileManifest->getTile(getName(), VEHICLE_CLASS));
	if (aType == nullptr)
		return;
	makeImages(aType);
	removeChild(mImages);
	mBody = new engine::Container();
	addChild(mBody);
	mBody->addChild(mImages);
	if (mEmitter != nullptr)
	{
		removeChild(mEmitter);
		mBody->addChild(mEmitter);
	}
	// the hovering picture comes before the parked one
	if (aType->mHoverImage != nullptr)
	{
		aType->mHoverImage->setPivot(1);
		mImages->addChild(new engine::ImageNode(aType->mHoverImage));
	}
	if (aType->mParkedImage != nullptr)
	{
		aType->mParkedImage->setPivot(1);
		mImages->addChild(new engine::ImageNode(aType->mParkedImage));
	}
	mLayer = LAYER_GROUND;
	setSmoothType(SMOOTH_TYPE);
	mSpeed = (float)aType->mSpeed;
	park();
}

// 0x42f780
void Copter::updateVehicle(engine::UpdateInfo& theInfo)
{
	if (mPhase == PHASE_TAKING_OFF || mPhase == PHASE_LANDING)
	{
		lift(theInfo);
		Container::update(theInfo);
	}
	else
		Van::updateVehicle(theInfo);
}

// 0x4323f0
void Copter::goTo(CustomerTile* theTile)
{
	mTarget = theTile;
	engine::IntPair aCell = theTile->getRoadCell();
	if (mPath.empty() && !isAtCell(aCell))
	{
		PizzaFrenzy::instance()->mSounds->play(SOUND_LEAVE_COPTER, 1.0f, 1.0f);
		startPath(aCell);
		if (!mPath.empty())
		{
			mPhase = PHASE_TAKING_OFF;
			beginLift(true);
		}
	}
}

// 0x4328d0
std::vector<engine::IntPair> Copter::findWay(const engine::IntPair& theCell)
{
	std::vector<engine::IntPair> aPath;
	aPath.push_back(mCell);
	aPath.push_back(theCell);
	return aPath;
}

// 0x431e40
void Copter::face(const engine::IntPair& theCell)
{
	faceAlong(theCell);
}

// 0x431f20: the picture of the parked copter comes after the placing, which shows the first picture
void Copter::park()
{
	mPhase = PHASE_PARKED;
	mPath.clear();
	if (mEmitter != nullptr)
		mEmitter->stop();
	placeAt(mHomeCell);
	if (IMAGE_PARKED < mImages->getCount())
		mImages->select(IMAGE_PARKED, 0.0f, true);
	mDirty = true;
}

// 0x432320
void Copter::reachGoal()
{
	// the flying state has nothing to leave
	mPhase = PHASE_LANDING;
	beginLift(false);
}

// 0x432060
void Copter::beginLift(bool theTakeOff)
{
	if (mEmitter != nullptr)
		mEmitter->start();
	mLiftTime = LIFT_SECONDS;
	mImages->select(IMAGE_ARRIVED, 0.0f, true);
	if (!theTakeOff)
	{
		PizzaFrenzy::instance()->mGameScreen->moveVehicle(this, LAYER_AIR, LAYER_GROUND);
		mLayer = LAYER_GROUND;
		mMap->addVehicle(this);
	}
}

// 0x431d00: the pictures are raised by the share of the lift that has passed (a landing's is the share left)
void Copter::lift(engine::UpdateInfo& theInfo)
{
	mLiftTime = mLiftTime - theInfo.mSeconds;
	if (mLiftTime < 0.0f)
	{
		bool aTookOff = mPhase == PHASE_TAKING_OFF;
		endLift();
		if (aTookOff)
		{
			// the flying state's entry
			mPhase = PHASE_DRIVING;
			if (mEmitter != nullptr)
				mEmitter->start();
		}
		else
			arrive();
		return;
	}
	float aShare = mLiftTime / LIFT_SECONDS;
	if (mPhase == PHASE_TAKING_OFF)
		aShare = 1.0f - aShare;
	mBody->setPosition(0.0f, aShare * -LIFT_HEIGHT);
}

// 0x432550
void Copter::endLift()
{
	if (mPhase == PHASE_TAKING_OFF)
	{
		face(mPath[mNext]);
		PizzaFrenzy::instance()->mGameScreen->moveVehicle(this, LAYER_GROUND, LAYER_AIR);
		mLayer = LAYER_AIR;
		mMap->addVehicle(this);
	}
}
