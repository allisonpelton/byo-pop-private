#include "PlayerShieldFx.h"
#include "FishBase.h"
#include "FishFood.h"
#include "StageMgr.h"

#include <math.h>

using namespace engine;

namespace feedingfrenzy
{

// How far the glow swings, by the height of the fish, when it starts and as it goes round, and how fast
const float ATTACH_RADIUS_FACTOR = 1.1f;
const float UPDATE_RADIUS_FACTOR = 0.9f;
const float START_RADIUS = 100.0f;
const float START_SPEED = 5.0f;
const float ATTACH_SPEED = 6.0f;
// The glow's place across the fish: how far it swings, and where its middle is
const float SWING_WIDTH = 15.0f;
const float SWING_MIDDLE = 7.0f;
// The size the glow takes between its far and near sides, and the share of the sine that the near side starts at
const float GLOW_SCALE_MIDDLE = 0.75f;
const float PHASE_SHARE = 0.5f;
// The particle file, and the name the node answers to
const char* const GLOW_FILE = "config\\FX\\playerShieldGlow.xml";
const char* const GLOW_NAME = "shieldGlowP";
const float TWO_PI = 6.2831855f;
// A glow is shown once in a frame
const int MAX_DRAWS_PER_FRAME = 2;

// 0x419490
PlayerShieldFx::PlayerShieldFx() :
    mPlayer(nullptr), mAngle(0.0f), mInFront(true), mRadius(START_RADIUS), mSpeed(START_SPEED), mDrawCount(1)
{
}

// 0x419360
void PlayerShieldFx::init()
{
	setName(GLOW_NAME);
	mGlow = new ParticleSystem();
	addChild(mGlow);
	mGlow->load(GLOW_FILE);
	mGlow->setVisible(false);
	mGlow->setStreamTarget(0, this);
	clearFlags(NODE_LISTENERS);
	mInFront = true;
}

// 0x418f90
void PlayerShieldFx::attach(FishBase* thePlayer)
{
	mAngle = 0.0f;
	float aHeight = thePlayer->getPlacedHitRect()->getRect().height();
	mPlayer = thePlayer;
	mRadius = aHeight * ATTACH_RADIUS_FACTOR;
	mSpeed = ATTACH_SPEED;
}

// 0x418fe0
void PlayerShieldFx::show()
{
	mGlow->startEmitting();
	mGlow->setVisible(true);
	FishFood::instance()->getStageMgr()->addToInFront(this);
	mInFront = true;
}

// 0x419020
void PlayerShieldFx::hide()
{
	mGlow->stopEmitting();
	mGlow->setVisible(false);
	FishFood::instance()->getStageMgr()->removeFromInFront(this);
	FishFood::instance()->getStageMgr()->removeFromBehind(this);
}

// 0x419060
void PlayerShieldFx::draw(Graphics* g)
{
	mDrawCount++;
	if (mDrawCount < MAX_DRAWS_PER_FRAME)
		Container::draw(g);
}

// 0x4190d0
void PlayerShieldFx::update(UpdateInfo& theInfo)
{
	if (mPlayer == nullptr)
		return;

	const Rect& aHitRect = mPlayer->getPlacedHitRect()->getRect();
	mRadius = aHitRect.height() * UPDATE_RADIUS_FACTOR;
	float aHitRight = aHitRect.right;
	float aHitLeft = aHitRect.left;
	Point aPlace = *mPlayer->getPosition();
	aPlace.y = mPlayer->getNosePosition().y;
	setPosition(aPlace);

	mAngle += theInfo.mSeconds;
	if (mAngle > TWO_PI)
		mAngle = 0.0f;
	float aDepth = (float)cos((double)(mAngle * mSpeed));
	float aSwingY = aDepth * mRadius;
	const Rect& aHitAgain = mPlayer->getPlacedHitRect()->getRect();
	float aHitX = aHitAgain.left;
	float aPlayerX = mPlayer->getPosition()->x;
	float aSwing = (float)sin((double)(PHASE_SHARE * mAngle * mSpeed));
	float aPhase = (float)sin((double)(mAngle * mSpeed)) * PHASE_SHARE;
	if (aPhase >= 0.0f || !mInFront)
	{
		if (aPhase > 0.0f && !mInFront)
		{
			FishFood::instance()->getStageMgr()->removeFromBehind(this);
			FishFood::instance()->getStageMgr()->addToInFront(this);
			mInFront = true;
		}
	}
	else
	{
		FishFood::instance()->getStageMgr()->removeFromInFront(this);
		FishFood::instance()->getStageMgr()->addToBehind(this);
		mInFront = false;
	}
	mGlow->setPosition(aSwing * SWING_WIDTH - SWING_MIDDLE + (aHitX - aPlayerX) + (aHitRight - aHitLeft) * 0.5f,
	                   aSwingY);
	float aScale = (aPhase + 1.0f) * GLOW_SCALE_MIDDLE;
	mGlow->setScale(aScale, aScale);
	mDrawCount = 0;
	Container::update(theInfo);
}

} // namespace feedingfrenzy
