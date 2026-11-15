#include "PrizeBlimp.h"
#include "App.h"
#include "CityMap.h"
#include "ImageNode.h"
#include "Modifiers.h"
#include "PizzaFrenzy.h"
#include "Random.h"
#include "Tip.h"

using namespace pizzafrenzy;

namespace
{
const char* const BLIMP_TYPE = "PrizeBlimp";
const char* const VEHICLE_CLASS = "vehicle";
const char* const SHADOW_IMAGE = "res\vehicles\blimp_dropshadow.jpg";
const char* const ENGINE_SOUND = "bonus_blimpEngine";
const char* const DROP_SOUND = "bonus_blimpDropTip";

// the shadow hangs below and to the left of the blimp, and both rock over six seconds
const float SHADOW_X = -67.0f;
const float SHADOW_Y = 113.0f;
const float ROCK_SECONDS = 6.0f;
const float BLIMP_ROCK_SIZE = 15.0f;
const float SHADOW_ROCK_X = -15.0f;
const float SHADOW_ROCK_Y = -30.0f;
const int SMOOTH_TYPE = 1;

// the blimp is let go when it is this far past the screen's edge
const float OFF_SCREEN_X = 100.0f;
const float OFF_SCREEN_Y = 50.0f;

// the tips come up in a column under the blimp's middle, and the blimp drops them over the middle of the screen
const float DROP_LEFT = 50.0f;
const float DROP_RIGHT = 650.0f;
const float DROP_OFFSET_X = -15.0f;
const float DROP_OFFSET_Y = 45.0f;
const int DROP_COUNT = 3;

// The seconds between drops: 0x53290c is a range that nothing in the exe ever sets, which is nothing to nothing, so
// the blimp drops in every frame it is over the middle (the draw is still made)
const engine::Point DROP_DELAY(0.0f, 0.0f);
}

// 0x431330: the pictures and the engine are made by the slots
PrizeBlimp::PrizeBlimp() : Van(BLIMP_TYPE), mDropTimer(0), mEngine(nullptr), mFlownOff(false)
{
}

// 0x430d50
void PrizeBlimp::loadType()
{
	Vehicle* aType = static_cast<Vehicle*>(PizzaFrenzy::instance()->mTileManifest->getTile(getName(), VEHICLE_CLASS));
	if (aType == nullptr)
		return;
	engine::Image* aShadow = engine::App::instance()->createImage(SHADOW_IMAGE, false);
	aShadow->setPivot(1);
	engine::ImageNode* aShadowNode = new engine::ImageNode(aShadow);
	aShadowNode->setPosition(SHADOW_X, SHADOW_Y);
	addChild(aShadowNode);
	makeImages(aType);
	mLayer = LAYER_AIR;  // above the vehicles on the ground (GameMode::addVehicleNode)
	mSpeed = (float)aType->mSpeed;
	setSmoothType(SMOOTH_TYPE);
	addListener(new engine::WaveModifier(0.0f, ROCK_SECONDS, 0.0f, BLIMP_ROCK_SIZE, 0.0f));
	aShadowNode->addListener(new engine::WaveModifier(ROCK_SECONDS, ROCK_SECONDS, SHADOW_ROCK_X, SHADOW_ROCK_Y, 0.0f));
	// the dot matrix sign under the blimp is left out
}

// 0x430cc0 (the sign's scroll, 0x430b90, is left out)
void PrizeBlimp::updateVehicle(engine::UpdateInfo& theInfo)
{
	fly(theInfo);
	Container::update(theInfo);
	float aTimer = mDropTimer;
	mDropTimer = aTimer - theInfo.mSeconds;
	if (aTimer - theInfo.mSeconds < 0.0f && !mFlownOff)
	{
		float aX = getPosition()->x;
		if (DROP_LEFT < aX && aX < DROP_RIGHT)
		{
			dropTips();
			mDropTimer = engine::randomFloat(DROP_DELAY);
		}
	}
}

// 0x430970
void PrizeBlimp::fly(engine::UpdateInfo& theInfo)
{
	int aRowBefore = (int)getY();
	engine::Point aPosition = *getPosition();
	aPosition.x = mVelocity.x * theInfo.mSeconds + aPosition.x;
	aPosition.y = mVelocity.y * theInfo.mSeconds + aPosition.y;
	setPosition(aPosition);
	if ((int)getY() != aRowBefore)
		mDirty = true;
	if (((float)engine::SCREEN_WIDTH + OFF_SCREEN_X < aPosition.x && 0.0f < mVelocity.x)
	    || (aPosition.x < -OFF_SCREEN_X && mVelocity.x < 0.0f)
	    || ((float)engine::SCREEN_HEIGHT + OFF_SCREEN_Y < aPosition.y && 0.0f < mVelocity.y)
	    || (aPosition.y < -OFF_SCREEN_Y && mVelocity.y < 0.0f))
	{
		leave();
		if (mEngine != nullptr)
			mEngine->stop();
		mFlownOff = true;
	}
}

// 0x430b30: the picture is the first, turned round when the blimp flies to the right
void PrizeBlimp::face(const engine::IntPair& theCell)
{
	mImages->select(0, 0.0f, true);
	float aScaleX = 1.0f;
	if (0.0f < mVelocity.x)
		aScaleX = -1.0f;
	setScale(aScaleX, 1.0f);
}

// 0x431110: a home tile of none
void PrizeBlimp::start(CityMap* theMap, const engine::Point& thePosition)
{
	mHome = nullptr;
	mMap = theMap;
	layout();
	setPosition(thePosition.x, thePosition.y);
	theMap->addVehicle(this);
	mDirty = false;
	mDropTimer = 0;
	mEngine = PizzaFrenzy::instance()->mSounds->getVoice(ENGINE_SOUND);
	if (mEngine != nullptr)
	{
		mEngine->setLoop(true);
		mEngine->play();
	}
	mFlownOff = false;
}

// 0x430ac0
void PrizeBlimp::setDirection(const engine::Point& theDirection)
{
	mVelocity.x = theDirection.x * mSpeed;
	mVelocity.y = theDirection.y * mSpeed;
	face(engine::IntPair{0, 0});
}

// 0x431220: each tip is dropped at the same place, 15 left of the blimp's position and 45 under it
void PrizeBlimp::dropTips()
{
	engine::Point aPoint = *getPosition();
	aPoint.x = aPoint.x + DROP_OFFSET_X;
	aPoint.y = aPoint.y + DROP_OFFSET_Y;
	for (int i = 0; i < DROP_COUNT; i++)
		PizzaFrenzy::instance()->mMode->addTip(nullptr, aPoint, new BalloonTip());
	PizzaFrenzy::instance()->mSounds->play(DROP_SOUND, 1.0f, 1.0f);
}
