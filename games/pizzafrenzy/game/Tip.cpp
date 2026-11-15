#include "Tip.h"
#include "App.h"
#include "CityMap.h"
#include "GameMode.h"
#include "Modifiers.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

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

// what a tip is worth: the upgrade's multiplier times five, or twenty-five for a topping without upgrades and ten for no
// topping at all, times the tip's multiplier
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
	engine::Image* aPicture = engine::App::instance()->createImage(formatText(TIP_IMAGE, (int)theMultiplier).c_str(), false);
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

// 0x42f160: the sounds, and then the tip is taken: it stops waiting and is hidden, and its money is added (the original
// adds it when the splat of the amount has flown to the tip jar: left out, see the file's comment)
void TipPopup::take()
{
	playSound(SOUND_CLICK, 1.0f);
	playSound(SOUND_CASH, 1.0f);
	mWaiting = false;
	setVisible(false);
	setEnabled(false);
	finish();
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

// 0x42d830: the splats of the award ("cashTipSplat" with the text "Citizen Award! $%d" and "tipSplat" with the award's
// picture) are left out, see the file's comment
void CitizenAward::take()
{
	playSound(SOUND_CLICK, 1.0f);
	playSound(SOUND_AWARD, AWARD_SOUND_VOLUME);
	mWaiting = false;
	setVisible(false);
	setEnabled(false);
	finish();
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
