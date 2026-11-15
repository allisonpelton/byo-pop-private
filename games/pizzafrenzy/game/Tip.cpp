#include "Tip.h"
#include "App.h"
#include "CityMap.h"
#include "GameMode.h"
#include "GameText.h"
#include "ImageNode.h"
#include "Modifiers.h"
#include "PizzaFrenzy.h"
#include "Random.h"
#include "ScreenNodes.h"

#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const TIP_IMAGE = "res\\tips\\cash-%d.jpg";
const char* const AWARD_IMAGE = "res\\tips\\citizenAward.jpg";
const char* const BOARD_IMAGE = "res\\tips\\tipBackground.jpg";
const char* const BOARD_LIT_IMAGE = "res\\tips\\tipBackground_on.jpg";
const char* const SOUND_CLICK = "order_click1";
const char* const SOUND_CASH = "tip_cash";
const char* const SOUND_AWARD = "tip_citizenAward";
const char* const SOUND_LOST = "tip_lost";
const char* const SOUND_MOUSE_OVER = "order_mouseover1";

// the splats of a taken tip: the amount's text, which comes up, and the picture, which flies to the tip jar
const char* const AMOUNT_SPLAT = "cashTipSplat";
const char* const PICTURE_SPLAT = "tipSplat";

// what a tip is worth: the upgrade's multiplier times five, or twenty-five for a topping without upgrades and ten for
// no topping at all, times the tip's multiplier
const int TIP_VALUE_PER_MULTIPLIER = 5;
const int TIP_VALUE_NO_UPGRADE = 25;
const int TIP_VALUE_NO_TOPPING = 10;
const int CITIZEN_AWARD_MONEY = 500;

// where the pictures stand on the tip
const float PICTURE_X = -1.0f;
const float PICTURE_Y = 3.0f;
const float AWARD_X = -1.0f;
const float AWARD_Y = -6.0f;

const float APPEAR_SECONDS = 0.2f;
const float WAIT_SECONDS = 5.0f;
const float FLASH_SECONDS = 5.0f;
const float LOST_SECONDS = 0.2f;
const float APPEAR_START_SCALE = 0.5f;
const float FADE_RATE = 5.0f;
const float LOST_SOUND_VOLUME = 0.8f;
const float AWARD_SOUND_VOLUME = 0.8f;
const float FLASH_PERIOD = 0.5f;
const float FLASH_MIN = 0.6f;
const float FLASH_MAX = 1.0f;

// the balloon tip: what it is worth for each of the multiplier, the pictures and where the shadow hangs
const int BALLOON_VALUE_PER_MULTIPLIER = 10;
const char* const BALLOON_SHADOW_IMAGE = "res\\tips\\balloonTip_shadow.jpg";
const char* const BALLOON_IMAGE = "res\\tips\\balloonTip.jpg";
const char* const BALLOON_LIT_IMAGE = "res\\tips\\balloonTip_on.jpg";
const char* const BALLOON_CLOSED_IMAGE = "res\\tips\\balloonTip_closed.jpg";
const char* const SOUND_BALLOON = "bonus_blimpDropTip";
const float BALLOON_SHADOW_X = -67.0f;
const float BALLOON_SHADOW_Y = 113.0f;
const int BALLOON_CLOSED = 2;
// the balloon flies off, to either side and at first down, and falls faster as it goes; it is let go off these edges
const float FLY_SIDE_SPEED = 100.0f;
const float FLY_START_SPEED = 50.0f;
const float FLY_MIN_SECONDS = 0.3f;
const float FLY_MAX_SECONDS = 0.6f;
const float FLY_PULL = 700.0f;
const float OFF_SCREEN_BOTTOM = 700.0f;
const float OFF_SCREEN_LEFT = -50.0f;
const float OFF_SCREEN_RIGHT = 850.0f;
// the balloon then drifts: to either side slowly and down, swaying across over a period of a second and a half
const float DRIFT_SIDE_SPEED = 75.0f;
const float DRIFT_MIN_SPEED = 50.0f;
const float DRIFT_MAX_SPEED = 75.0f;
const float SWAY_PERIOD = 1.5f;
const float SWAY_SIZE = 20.0f;

void playSound(const char* theName, float theVolume)
{
	PizzaFrenzy::instance()->mSounds->play(theName, theVolume, 1.0f);
}
}

// 0x42f050: the tip takes no mouse until it is shown (the mask's bit 2)
TipPopup::TipPopup() : mMultiplier(0), mAmount(0), mMap(nullptr), mWaiting(false)
{
	setMouseMasked(true);
}

// 0x42ef80
TipPopup::~TipPopup()
{
}

// 0x42ec80
void TipPopup::setValue(float theMultiplier, Topping* theTopping)
{
	GameStats* aStats = PizzaFrenzy::instance()->mGameStats;
	mMultiplier = theMultiplier;
	int aBase = TIP_VALUE_NO_TOPPING;
	if (theTopping != nullptr)
	{
		Upgrade* anUpgrade = aStats->getUpgrade(theTopping);
		aBase = anUpgrade == nullptr ? TIP_VALUE_NO_UPGRADE : anUpgrade->mBonusMultiplier * TIP_VALUE_PER_MULTIPLIER;
	}
	mAmount = (int)((float)aBase * theMultiplier);
	aStats->mTipsEarned += mAmount;
	makeBoard();
	engine::Image* aPicture = engine::App::instance()->createImage(formatText(TIP_IMAGE, (int)theMultiplier).c_str(),
	                                                               false);
	if (aPicture != nullptr)
	{
		aPicture->setPivot(1);
		engine::ImageNode* aNode = new engine::ImageNode(aPicture);
		aNode->setPosition(PICTURE_X, PICTURE_Y);
		addChild(aNode);
	}
	mWaiting = true;
	mStates.changeState(new TipAppearState(this));
}

// 0x42e940
void TipPopup::makeBoard()
{
	mBoard = new engine::Selector();
	engine::Image* aBoard = engine::App::instance()->createImage(BOARD_IMAGE, false);
	aBoard->setPivot(1);
	mBoard->addChild(new engine::ImageNode(aBoard));
	engine::Image* aLitBoard = engine::App::instance()->createImage(BOARD_LIT_IMAGE, false);
	aLitBoard->setPivot(1);
	mBoard->addChild(new engine::ImageNode(aLitBoard));
	mBoard->select(0, 0.0f, true);
	addChild(mBoard);
}

// 0x42eac0
void TipPopup::start()
{
	mStates.changeState(new TipWaitState(this));
}

// 0x42f160: the sounds, and then the tip is taken: it stops waiting and is hidden, and the amount comes up as a splat
// while the cash's picture flies to the tip jar, where the tip's money is added
void TipPopup::take()
{
	playSound(SOUND_CLICK, 1.0f);
	playSound(SOUND_CASH, 1.0f);
	mWaiting = false;
	flyToTipJar(GT(0x4fc698), formatText(TIP_IMAGE, (int)mMultiplier));
	setVisible(false);
	setEnabled(false);
}

// 0x42f160, 0x42d830: the two splats of a taken tip, both where the tip is: the text over the amount, and the picture
// in place of the definition's, which flies to the tip jar and finishes the tip there
void TipPopup::flyToTipJar(const std::string& theAmountText, const std::string& thePicture)
{
	PizzaFrenzy* aGame = PizzaFrenzy::instance();
	engine::Point aPosition = *getPosition();
	engine::Splat* anAmount = aGame->mSplatFactory->create(AMOUNT_SPLAT, aPosition.x, aPosition.y);
	anAmount->setText(formatText(theAmountText, mAmount));
	engine::Splat* aPicture = aGame->mSplatFactory->create(PICTURE_SPLAT, aPosition.x, aPosition.y);
	aPicture->setChild(new engine::ImageNode(engine::App::instance()->createImage(thePicture.c_str(), false)));
	const engine::Point& aTipJar = aGame->mHud->getTipJarCountPosition();
	aPicture->setDestination(aTipJar.x, aTipJar.y);
	aPicture->connectArrived(this, &TipPopup::finish);
}

// 0x42e4c0
void TipPopup::finish()
{
	GameMode* aMode = PizzaFrenzy::instance()->mMode;
	aMode->releaseTip(this);
	aMode->addTips(mAmount);
}

// 0x42e510: the map keeps the tip, which stands at the point
void TipPopup::setMap(CityMap* theMap, const engine::Point& thePosition)
{
	mMap = theMap;
	setPosition(thePosition);
	theMap->addTip(this);
}

// 0x42e560
void TipPopup::updateTip(engine::UpdateInfo& theInfo)
{
	mStates.update(theInfo);
	Container::update(theInfo);
}

// 0x42e540
void TipPopup::clearParent(engine::Node* theParent)
{
	Container::clearParent(theParent);
	mMap->removeTip(this);
}

// 0x42e4f0
void TipPopup::onMouseDownEvent(engine::MouseEvent& theEvent)
{
	if (mWaiting)
		take();
}

// 0x42ee40
void TipPopup::onMouseEnterEvent(engine::MouseEvent& theEvent)
{
	if (mWaiting)
	{
		mBoard->select(1, 0.0f, true);
		playSound(SOUND_MOUSE_OVER, 1.0f);
	}
}

// 0x42eb30
void TipPopup::onMouseExitEvent(engine::MouseEvent& theEvent)
{
	mBoard->select(0, 0.0f, true);
}

// 0x42d630: the award's picture hangs over the tip, and the money is 500
void CitizenAward::setValue(float theMultiplier, Topping* theTopping)
{
	mAmount = CITIZEN_AWARD_MONEY;
	PizzaFrenzy::instance()->mGameStats->mTipsEarned += mAmount;
	makeBoard();
	engine::Image* anAward = engine::App::instance()->createImage(AWARD_IMAGE, false);
	if (anAward != nullptr)
	{
		anAward->setPivot(1);
		engine::ImageNode* aNode = new engine::ImageNode(anAward);
		aNode->setPosition(AWARD_X, AWARD_Y);
		addChild(aNode);
	}
	mWaiting = true;
	mStates.changeState(new TipAppearState(this));
}

// 0x42d830: as a tip's, with the award's text and picture
void CitizenAward::take()
{
	playSound(SOUND_CLICK, 1.0f);
	playSound(SOUND_AWARD, AWARD_SOUND_VOLUME);
	mWaiting = false;
	flyToTipJar(GT(0x4fc248), AWARD_IMAGE);
	setVisible(false);
	setEnabled(false);
}

// 0x42e590
void TipAppearState::enter()
{
	mTimeLeft = APPEAR_SECONDS;
	mTip->setOpacity(0.0f);
	mTip->setScale(APPEAR_START_SCALE);
}

// 0x42e5c0
void TipAppearState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft -= theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
		mTimeLeft = 0;
	float aRemaining = mTimeLeft * FADE_RATE;
	mTip->setOpacity(1.0f - aRemaining);
	float aScale = 1.0f - aRemaining * APPEAR_START_SCALE;
	mTip->setScale(aScale, aScale);
	if (mTimeLeft <= 0.0f)
		mTip->start();
}

// 0x42e650
void TipWaitState::enter()
{
	mTimeLeft = WAIT_SECONDS;
}

// 0x42eb50
void TipWaitState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft -= theInfo.mSeconds;
	if (mTimeLeft <= 0.0f && mTip->isWaiting())
		mTip->mStates.changeState(new TipFlashState(mTip));
}

// 0x42e660: the tip's opacity swings while it flashes
void TipFlashState::enter()
{
	mTip->addListener(new engine::OpacityModifier(FLASH_PERIOD, FLASH_MIN, FLASH_MAX, 0.0f));
	mTimeLeft = FLASH_SECONDS;
}

// 0x42e6f0
void TipFlashState::exit()
{
	mTip->removeAllListeners();
}

// 0x42ebf0
void TipFlashState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft -= theInfo.mSeconds;
	if (mTimeLeft <= 0.0f)
		mTip->mStates.changeState(new TipLostState(mTip));
}

// 0x42eef0: the tip stops waiting, and a sound plays
void TipLostState::enter()
{
	mTimeLeft = LOST_SECONDS;
	mTip->setWaiting(false);
	playSound(SOUND_LOST, LOST_SOUND_VOLUME);
}

// 0x42e700
void TipLostState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft -= theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
		mTimeLeft = 0;
	mTip->setOpacity(mTimeLeft * FADE_RATE);
	float aScale = mTimeLeft * FADE_RATE * APPEAR_START_SCALE + APPEAR_START_SCALE;
	mTip->setScale(aScale, aScale);
	if (mTimeLeft <= 0.0f)
		PizzaFrenzy::instance()->mMode->releaseTip(mTip);
}

// 0x42e090: the bounds are the balloon's alone (the shadow and the picture of the money are not in them)
void BalloonTip::layout()
{
	mBounds = mRect;
	mBoard->layout();
	mBounds.unite(*mBoard->getBounds());
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(engine::NODE_LAYOUT);
}

// 0x42e110
void BalloonTip::setValue(float theMultiplier, Topping* theTopping)
{
	mMultiplier = theMultiplier;
	mAmount = (int)((float)BALLOON_VALUE_PER_MULTIPLIER * theMultiplier);
	PizzaFrenzy::instance()->mGameStats->mTipsEarned += mAmount;
	makeBoard();
	addOwnFlags(engine::NODE_MOUSE);
	mWaiting = false;
	mStates.changeState(new TipAppearState(this));
}

// 0x42de60
void BalloonTip::makeBoard()
{
	engine::Image* aShadow = engine::App::instance()->createImage(BALLOON_SHADOW_IMAGE, false);
	aShadow->setPivot(1);
	engine::ImageNode* aShadowNode = new engine::ImageNode(aShadow);
	aShadowNode->setPosition(BALLOON_SHADOW_X, BALLOON_SHADOW_Y);
	addChild(aShadowNode);
	mBoard = new engine::Selector();
	const char* const aBoards[3] = {BALLOON_IMAGE, BALLOON_LIT_IMAGE, BALLOON_CLOSED_IMAGE};
	for (int i = 0; i < 3; i++)
	{
		engine::Image* aBoard = engine::App::instance()->createImage(aBoards[i], false);
		aBoard->setPivot(1);
		mBoard->addChild(new engine::ImageNode(aBoard));
	}
	mBoard->select(BALLOON_CLOSED, 0.0f, true);
	addChild(mBoard);
}

// 0x42e1d0
void BalloonTip::start()
{
	mStates.changeState(new BalloonFlyState(this));
}

// 0x42dbc0
void BalloonFlyState::enter()
{
	mVelocity.x = engine::randomFloat(engine::Point(-FLY_SIDE_SPEED, FLY_SIDE_SPEED));
	mVelocity.y = FLY_START_SPEED;
	mTimeLeft = engine::randomFloat(engine::Point(FLY_MIN_SECONDS, FLY_MAX_SECONDS));
}

// 0x42e240
void BalloonFlyState::update(engine::UpdateInfo& theInfo)
{
	engine::Point aPosition = *mTip->getPosition();
	mVelocity.y = theInfo.mSeconds * FLY_PULL + mVelocity.y;
	aPosition.x = mVelocity.x * theInfo.mSeconds + aPosition.x;
	aPosition.y = mVelocity.y * theInfo.mSeconds + aPosition.y;
	mTip->setPosition(aPosition);
	if (OFF_SCREEN_BOTTOM < aPosition.y || aPosition.x < OFF_SCREEN_LEFT || OFF_SCREEN_RIGHT < aPosition.x)
		PizzaFrenzy::instance()->mMode->releaseTip(mTip);
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mTimeLeft <= 0.0f)
		mTip->mStates.changeState(new BalloonDriftState(mTip));
}

// 0x42e360
void BalloonDriftState::enter()
{
	mTip->openBoard();
	mVelocity.x = engine::randomFloat(engine::Point(-DRIFT_SIDE_SPEED, DRIFT_SIDE_SPEED));
	mVelocity.y = engine::randomFloat(engine::Point(DRIFT_MIN_SPEED, DRIFT_MAX_SPEED));
	playSound(SOUND_BALLOON, 1.0f);
	mTip->setWaiting(true);
	float aPeriod = engine::randomFloat(engine::Point(SWAY_PERIOD, SWAY_PERIOD));
	float aSize = engine::randomFloat(engine::Point(SWAY_SIZE, SWAY_SIZE));
	mTip->addListener(new engine::WaveModifier(aPeriod, 0.0f, aSize, 0.0f, 0.0f));
}

// 0x42db30
void BalloonDriftState::update(engine::UpdateInfo& theInfo)
{
	engine::Point aPosition = *mTip->getPosition();
	aPosition.x = mVelocity.x * theInfo.mSeconds + aPosition.x;
	aPosition.y = mVelocity.y * theInfo.mSeconds + aPosition.y;
	mTip->setPosition(aPosition);
	if (OFF_SCREEN_BOTTOM < aPosition.y || aPosition.x < OFF_SCREEN_LEFT || OFF_SCREEN_RIGHT < aPosition.x)
		PizzaFrenzy::instance()->mMode->releaseTip(mTip);
}
