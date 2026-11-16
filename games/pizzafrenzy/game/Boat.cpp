#include "Boat.h"
#include "App.h"
#include "CityMap.h"
#include "PizzaFrenzy.h"

#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const VEHICLE_CLASS = "vehicle";
const char* const KIND_BRIDGE = "bridge";
const int SMOOTH_TYPE = 1;

// a click: how many frames the boat is faster for, and how much
const int BOOST_FRAMES_PER_CLICK = 10;
const float BOOST_FACTOR = 3.0f;
// the length of the boat's heading, how far ahead it looks for a bridge
const float HEADING_LENGTH = 25.0f;
}

// 0x431810: the pictures are made by the slot loadType
Boat::Boat(const std::string& theTypeName) :
	Van(theTypeName), mVelocity(0.0f, 0.0f), mBoostFrames(0), mHeading(0.0f, 0.0f)
{
}

// 0x4318f0: the original takes the mouse for the boat before it looks up the type
void Boat::loadType()
{
	Vehicle* aType = static_cast<Vehicle*>(PizzaFrenzy::instance()->mTileManifest->getTile(getName(), VEHICLE_CLASS));
	addOwnFlags(engine::NODE_MOUSE);
	if (aType == nullptr)
		return;
	makeImages(aType);
	// the wake is put over the pictures
	if (mEmitter != nullptr)
	{
		removeChild(mEmitter);
		addChild(mEmitter);
	}
	mLayer = LAYER_GROUND;
	mSpeed = (float)aType->mSpeed;
	setSmoothType(SMOOTH_TYPE);
}

// 0x4318d0
void Boat::updateVehicle(engine::UpdateInfo& theInfo)
{
	sail(theInfo);
	Container::update(theInfo);
}

// 0x431980
void Boat::face(const engine::IntPair& theCell)
{
	if (mVelocity.x == 0.0f)
		mImages->select(mVelocity.y < 0.0f ? IMAGE_NORTH : IMAGE_SOUTH, 0.0f, true);
	else
		mImages->select(IMAGE_EAST_WEST, 0.0f, true);
	setScale(mVelocity.x < 0.0f ? -1.0f : 1.0f, 1.0f);
}

// 0x431550
void Boat::start(CityMap* theMap, const engine::IntPair& thePoint)
{
	mHome = nullptr;
	mMap = theMap;
	layout();
	setPosition((float)thePoint.mFirst, (float)thePoint.mSecond);
	theMap->addVehicle(this);
	mDirty = false;
	mBoostFrames = 0;
}

// 0x431760
void Boat::setDirection(const engine::Point& theDirection)
{
	mVelocity.x = mSpeed * theDirection.x;
	mVelocity.y = mSpeed * theDirection.y;
	face(engine::IntPair{0, 0});
	mHeading = mVelocity;
	float aLength = std::sqrt(mHeading.x * mHeading.x + mHeading.y * mHeading.y);
	if (aLength != 0.0f)
	{
		float aFactor = 1.0f / aLength;
		mHeading.x = aFactor * mHeading.x;
		mHeading.y = aFactor * mHeading.y;
	}
	mHeading.x = HEADING_LENGTH * mHeading.x;
	mHeading.y = HEADING_LENGTH * mHeading.y;
}

// 0x431750
void Boat::handleMouseDownEvent(engine::MouseEvent& theEvent)
{
	mBoostFrames += BOOST_FRAMES_PER_CLICK;
}

// 0x4315b0
void Boat::sail(engine::UpdateInfo& theInfo)
{
	int aRowBefore = (int)getY();
	engine::Point aPosition = *getPosition();
	float aStepX = theInfo.mSeconds * mVelocity.x;
	float aStepY = theInfo.mSeconds * mVelocity.y;
	if (0 < mBoostFrames)
	{
		mBoostFrames--;
		aStepX = BOOST_FACTOR * aStepX;
		aStepY = BOOST_FACTOR * aStepY;
	}
	aPosition.x = aStepX + aPosition.x;
	aPosition.y = aStepY + aPosition.y;
	if (!canSail(engine::Point(mHeading.x + aPosition.x, mHeading.y + aPosition.y)) || !canSail(aPosition))
		return;
	setPosition(aPosition);
	if ((int)getY() != aRowBefore)
		mDirty = true;
	bool aLeftScreenX = ((float)engine::SCREEN_WIDTH < aPosition.x && 0.0f < mVelocity.x) ||
	                    (aPosition.x < 0.0f && mVelocity.x < 0.0f);
	bool aLeftScreenY = ((float)engine::SCREEN_HEIGHT < aPosition.y && 0.0f < mVelocity.y) ||
	                    (aPosition.y < 0.0f && mVelocity.y < 0.0f);
	if (aLeftScreenX || aLeftScreenY)
		leave();
}

// 0x431a10
bool Boat::canSail(const engine::Point& thePoint)
{
	engine::IntPair aCell = mMap->pointToCell(engine::IntPair{(int)thePoint.x, (int)thePoint.y});
	if (-1 < aCell.mFirst && -1 < aCell.mSecond)
	{
		Tile* aTile = mMap->getTileAt(aCell);
		if (aTile != nullptr && aTile->getKind() == KIND_BRIDGE)
		{
			if (!static_cast<BridgeTile*>(aTile)->raise(true))
				return false;
		}
	}
	return true;
}
