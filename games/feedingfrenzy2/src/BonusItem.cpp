#include "BonusItem.h"

#include "Draws.h"
#include "Engine.h"
#include "FeedingFrenzyTwo.h"
#include "GameData.h"
#include "GameHud.h"
#include "Params.h"
#include "Spawn.h"
#include "Stage.h"
#include "StringTable.h"
#include "Theme.h"
#include "Visual.h"

using namespace engine;

namespace
{

// The speeds' ranges before a behaviour gives its own (0x483a90 (feedingfrenzy2)), and the behaviour's defaults
// (0x4822b0 (feedingfrenzy2))
const FloatRange INITIAL_SPEED_RANGE_X(-10.0f, 0.0f);
const FloatRange INITIAL_SPEED_RANGE_Y(-60.0f, 10.0f);
const FloatRange DEFAULT_SPEED_RANGE_X(-10.0f, 10.0f);
const FloatRange DEFAULT_SPEED_RANGE_Y(-60.0f, -140.0f);
// The fade in as it spawns (0x407a30 (feedingfrenzy2))
const float SPAWN_FADE_SECONDS = 0.4f;
// The pop's run: its frames' count and rate (0x40e120 (feedingfrenzy2))
const unsigned int POP_FRAME_COUNT = 3;
const float POP_FRAME_RATE = 15.0f;
// The seconds a time bubble adds to the stage's clock (0x494590 (feedingfrenzy2))
const double TIME_BONUS_SECONDS = 3.0;

} // namespace

BonusItem::BonusItem()
    : mPoppable(true), mSpeedRangeX(INITIAL_SPEED_RANGE_X), mSpeedRangeY(INITIAL_SPEED_RANGE_Y)
{
}

void BonusItem::create(Stage* theStage)
{
	mVisual = new Visual();
	mVisual->setup();
	if (!mEffectName.empty())
	{
		mEffect = theStage->getTheme()->getParticleSystem(mEffectName);
		mVisual->getBehindGroup()->addChild(mEffect.get());
	}
}

void BonusItem::setVisualData(VisualData* theData)
{
	Actor::setVisualData(theData);
	addPopFrames();
	if (mEffect)
		mEffect->setPosition(mShapes[ACTOR_SHAPE_VISUAL].mShape->getCenter());
}

void BonusItem::addPopFrames()
{
	if (mVisualData->getAnimationCount() != 1)
		return;
	const AnimationRun& aRun = mVisualData->getAnimation(0);
	unsigned int aFirst = aRun.mEnd - aRun.mStart;
	Engine* anEngine = getEngine();
	mVisualData->addFrame(anEngine->getImage("resource/bonus/BubblePop0.jpg"));
	mVisualData->addFrame(anEngine->getImage("resource/bonus/BubblePop1.jpg"));
	mVisualData->addFrame(nullptr);
	mVisualData->addAnimation("pop", aFirst, aFirst + POP_FRAME_COUNT, POP_FRAME_RATE);
}

void BonusItem::addToStage(Stage* theStage, int theLocation)
{
	Actor::addToStage(theStage, theLocation);
	theStage->addBonus(this);
	Visual* aVisual = getVisual();
	aVisual->setOpacity(1.0f);
	aVisual->setVisible(true);
	aVisual->getImage()->setRun(0, 0);
	if (mBehavior)
	{
		mSpeedRangeX = mBehavior->getRange("speedRangeX", DEFAULT_SPEED_RANGE_X);
		mSpeedRangeY = mBehavior->getRange("speedRangeY", DEFAULT_SPEED_RANGE_Y);
	}
	if (mEffect)
		mEffect->start();
	if (theLocation == SPAWN_LOCATION_POINT)
	{
		mDesiredVelocity = Vector2f(0.0f, 0.0f);
		mVelocity = Vector2f(0.0f, 0.0f);
	}
	else if (theLocation == SPAWN_LOCATION_BOTTOM)
	{
		mDesiredVelocity.x = drawIn(mSpeedRangeX);
		mDesiredVelocity.y = drawIn(mSpeedRangeY);
	}
	mVelocity = mDesiredVelocity;
	mStateMgr.changeState(new BonusSpawnState(this));
}

void BonusItem::remove()
{
	mStateMgr.changeState(nullptr);
	mStage->removeBonus(this);
	if (mEffect)
		mEffect->stop();
	Actor::remove();
}

bool BonusItem::isEdibleBy(Fish* theEater)
{
	BonusItemState* aState = mStateMgr.getState();
	if (aState == nullptr)
		return false;
	return aState->isEdible();
}

void BonusItem::beginBeingEaten(Fish* theEater, float theSeconds)
{
	if (mEffect)
		mEffect->stop();
	mStateMgr.changeState(new BonusEatenState(this, theEater, theSeconds));
}

void BonusItem::evacuate()
{
	BonusItemState* aState = mStateMgr.getState();
	if (mEffect)
		mEffect->stop();
	if (aState != nullptr)
		aState->evacuate();
}

void BonusItem::update(UpdateContext& theContext)
{
	BonusItemState* aState = mStateMgr.getState();
	if (aState != nullptr)
		aState->update(&theContext);
}

bool BonusItem::checkFloat()
{
	Shape* aSurface = nullptr;
	if (0.0f != static_cast<shape::Edge*>(mStage->getEdge(STAGE_EDGE_BELOW_WATER))->mDistance)
		aSurface = mStage->getEdge(STAGE_EDGE_BELOW_WATER);
	if (mPoppable && aSurface != nullptr && aSurface->overlaps(*getTransformedShape(ACTOR_SHAPE_BODY)))
	{
		mStateMgr.changeState(new BonusPopState(this));
		return false;
	}
	bool isInView = Actor::isInView();
	if (mSeen && !isInView)
	{
		mStateMgr.changeState(nullptr);
		return false;
	}
	mSeen = isInView;
	return true;
}

void BonusItem::pop()
{
	mStateMgr.changeState(new BonusPopState(this));
}

void BonusSpawnState::enter()
{
	mFade.start(SPAWN_FADE_SECONDS);
	mBonus->getVisual()->setOpacity(0.0f);
}

void BonusSpawnState::update(UpdateContext* theContext)
{
	mBonus->move(*theContext);
	if (!mBonus->checkFloat())
		return;
	if (!mFade.step(theContext->mElapsed))
	{
		mBonus->getVisual()->setOpacity(mFade.getShare());
		return;
	}
	mBonus->getVisual()->setOpacity(1.0f);
	mBonus->getStateMgr().changeState(new BonusFloatState(mBonus));
}

void BonusSpawnState::evacuate()
{
	mBonus->pop();
}

void BonusFloatState::update(UpdateContext* theContext)
{
	mBonus->move(*theContext);
	mBonus->checkFloat();
}

void BonusFloatState::evacuate()
{
	mBonus->pop();
}

void BonusPopState::enter()
{
	Visual* aVisual = mBonus->getVisual();
	aVisual->play("pop", 1.0f, 0, false);
	aVisual->mFinished.connect(this, &BonusPopState::onPopped);
}

void BonusPopState::exit()
{
	mBonus->getVisual()->mFinished.disconnect(this);
}

void BonusPopState::onPopped()
{
	mBonus->getStateMgr().changeState(nullptr);
}

void BonusEatenState::enter()
{
	mStart = mBonus->getPosition();
	mTime.start(mSeconds);
	if (0.0f == mSeconds)
		mBonus->getVisual()->setVisible(false);
}

void BonusEatenState::exit()
{
	Visual* aVisual = mBonus->getVisual();
	aVisual->setVisible(false);
	aVisual->setOpacity(1.0f);
}

void BonusEatenState::update(UpdateContext* theContext)
{
	if (mTime.step(theContext->mElapsed))
	{
		mBonus->getVisual()->setVisible(false);
		return;
	}
	Vector2f aMouth = mEater->getMouthPosition();
	float aShare = mTime.getShare();
	float anX = (float)(((double)aMouth.x - mStart.x) * aShare + mStart.x);
	float aY = (float)(((double)aMouth.y - mStart.y) * aShare + mStart.y);
	mBonus->setPosition(anX, aY);
	mBonus->getVisual()->setOpacity((float)(1.0 - aShare));
}

BonusTime::BonusTime()
{
	mEffectName = "bonusTimeFx";
}

void BonusTime::eatenByPlayer(PlayerFish* thePlayer)
{
	Actor::eatenByPlayer(thePlayer);
	// The stage's theme's sound bonusTimePickup played: not yet translated, as the sounds are not
	mStage->getHud()->showTimeBonus(mStage->toScreen(mLayer, getPosition()));
	mStage->setTimeLeft((float)((double)mStage->getTimeLeft() + TIME_BONUS_SECONDS));
}

Bonus1Up::Bonus1Up()
{
	mEffectName = "bonus1UpFx";
}

void Bonus1Up::eatenByPlayer(PlayerFish* thePlayer)
{
	Actor::eatenByPlayer(thePlayer);
	GameData* aGameData = getApp()->mGameData.get();
	aGameData->setLives(aGameData->getLives() + 1);
	// The stage's theme's sound bonus1Up played: not yet translated, as the sounds are not
	mStage->getHud()->showPlacedText(getApp()->mStrings->getString("extraLife"),
		mStage->toScreen(mLayer, getPosition()));
}
