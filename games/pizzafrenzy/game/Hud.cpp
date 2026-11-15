#include "Hud.h"
#include "App.h"
#include "GameMode.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const char* const HUD_LAYOUT_FILE = "res\\screenLayouts\\hudScreen.xml";
const char* const STAR_IMAGE = "res\\hud\\star-full.jpg";
const char* const WARNING_STAR_IMAGE = "res\\hud\\star-warning.jpg";
const char* const ORDER_SLIP_IMAGE = "res\\hud\\orderSlip.jpg";
const char* const MONEY_PREFIX = "$";
const char* const ROLL_SOUND = "tip_jarRollup";
const char* const SOUND_HAPPINESS_UP = "satisfaction_up";
const char* const SOUND_HAPPINESS_DOWN = "satisfaction_down";
const char* const SOUND_HAPPINESS_FULL = "satisfaction_full";

// the stars stand one tenth of the group's height apart, the first one at a tenth
const float STAR_SPACING = 0.1f;
const int STAR_FIRST_STEP = 1;
const int STAR_STEP_INCREMENT = 2;
const int STAR_HAPPINESS = 20;             // the happiness a star shows
const float STAR_HAPPINESS_SHARE = 0.05f;  // the share of a star that one point of happiness fills
const int FULL_HAPPINESS = 100;
const int FULL_HAPPINESS_TIPS = 100;       // what a full happiness gives in tips

const int ORDERS_PER_SLIP = 15;
const int ORDER_SLIP_ROUNDING = 18;        // added before the division by the orders a slip stands for
const float ORDER_SLIP_STEP = 2.0f;        // how far each slip stands from the one under it

const float TIP_JAR_ROLL_SECONDS = 1.0f;   // the seconds the money takes to roll the goal's worth
const float TIP_JAR_CONTENTS_X = 3.0f;

const float WARNING_PERIOD = 0.5f;         // the warning's and the flash's blinks
const float FLASH_SECONDS = 2.0f;

const float COUPON_ROW_RIGHT = 650.0f;     // where the row of coupon buttons ends, at the right
const float COUPON_ROW_STEP = 72.0f;
const float COUPON_ROW_Y = 47.0f;
const float COUPON_ROW_OFFSCREEN = 700.0f;  // a new coupon button comes in from this far to the left
const float COUPON_COME_SECONDS = 0.5f;
const float COUPON_CLOSE_SECONDS = 0.3f;
const int COUPON_PRICE = 200;               // what a coupon button costs when the level starts
const float COUPON_SLIDE_OUT = 800.0f;     // how far the coupon buttons slide to the left at a level's end
const float COUPON_SLIDE_SECONDS = 0.4f;
const float COUPON_GONE_X = -100.0f;       // a coupon button that has gone this far to the left is let go of
}

// 0x438ac0
Hud::Hud()
    : mScoreFlashTime(0),
      mShownOrderCount(0),
      mShownTipMoney(0),
      mShownHappiness(0),
      mShownPickCount(0),
      mTipJarRolling(false),
      mRollingTipMoney(0),
      mRollSound(nullptr),
      mHappinessFade(0),
      mPreviousHappiness(0)
{
}

// 0x439ba0
void Hud::setup(GameStats* theStats)
{
	loadLayout(HUD_LAYOUT_FILE);
	mStats = theStats;
	mOrderCount = static_cast<engine::TextItem*>(getNode("orderCount"));
	mProgressText = getNode("progressText");
	mTipJarCount = static_cast<engine::TextItem*>(getNode("tipJarCount"));
	mTipJarMax = static_cast<engine::TextItem*>(getNode("tipJarMax"));
	mScoreBackground = getNode("scoreBkg");
	mScoreBackground->setVisible(false);
	mTipJarCountPosition = *mTipJarCount->getPosition();
	mTipJarContents = static_cast<engine::ImageNode*>(getNode("tipJarContents"));
	mTipJarContents->setClipped(true);
	engine::Node* aTipJar = getNode("tipJar");
	mTipJarRect = *aTipJar->getBounds();
	// the tip jar's coins (the particle effect "TipJarCoins") are made here and added to the group: left out, see the
	// class
	mRollSound = PizzaFrenzy::instance()->mSounds->getVoice(ROLL_SOUND);
	mProgressStack = static_cast<engine::Container*>(getNode("progressStack"));
	engine::Node* aStars = getNode("stars");
	mStarsRect = *aStars->getBounds();
	mWarningStars = new engine::Container();
	mWarningStars->setVisible(false);
	addChild(mWarningStars);
	engine::Image* aStarImage = engine::App::instance()->createImage(STAR_IMAGE, false);
	aStarImage->setPivot(1);
	engine::Image* aWarningImage = engine::App::instance()->createImage(WARNING_STAR_IMAGE, false);
	aWarningImage->setPivot(1);
	int aStep = STAR_FIRST_STEP;
	for (int i = 0; i < STAR_COUNT; i++)
	{
		float aStarX = mStarsRect.left;
		float aStarY = getStarY(aStep);
		mStars[i] = new engine::ImageNode(aStarImage);
		mStars[i]->setPosition(aStarX, aStarY);
		mStars[i]->setClipped(true);  // the part of the star drawn is updateStars'
		addChild(mStars[i]);
		engine::ImageNode* aWarningStar = new engine::ImageNode(aWarningImage);
		aWarningStar->setPosition(aStarX, aStarY);
		mWarningStars->addChild(aWarningStar);
		aStep += STAR_STEP_INCREMENT;
	}
	// the burst effect ("BurstFx") is made and put in the middle of the stars' top here: left out, see the class
}

// 0x438fd0
void Hud::updateHud(engine::UpdateInfo& theInfo)
{
	if (!mTipJarRolling)
	{
		if (mStats->mTipMoney != mShownTipMoney)
		{
			startRoll(mShownTipMoney);
			mShownTipMoney = mStats->mTipMoney;
		}
	}
	else
	{
		rollTipJar(theInfo);
		updateTipJar();
	}
	if (mStats->mOrderCount != mShownOrderCount)
	{
		updateOrders();
		mShownOrderCount = mStats->mOrderCount;
	}
	if (mStats->mHappiness != mShownHappiness)
	{
		mPreviousHappiness = mShownHappiness;
		mHappinessFade = 1.0f;
		mShownHappiness = mStats->mHappiness;
		const char* aSound = mPreviousHappiness < mShownHappiness ? SOUND_HAPPINESS_UP : SOUND_HAPPINESS_DOWN;
		PizzaFrenzy::instance()->mSounds->play(aSound, 1.0f, 1.0f);
	}
	int aPickCount = PizzaFrenzy::instance()->mMode->getPickCount();
	if (aPickCount != mShownPickCount)
	{
		updateCoupons();
		mShownPickCount = aPickCount;
	}
	if (0.0f < mHappinessFade)
	{
		mHappinessFade = mHappinessFade - theInfo.mSeconds;
		if (mHappinessFade < 0.0f)
			mHappinessFade = 0;
		updateStars();
	}
	if (0.0f < mScoreFlashTime)
	{
		mScoreFlashTime = mScoreFlashTime - theInfo.mSeconds;
		if (mScoreFlashTime < 0.0f)
		{
			mScoreFlashTime = 0;
			mScoreBackground->removeAllListeners();
			mScoreBackground->setVisible(false);
		}
	}
	// A coupon button that has gone off to the left is let go of. The original takes the one after it as the next
	// and goes on from the one after that: the one next to a button let go of is not updated in that frame, and
	// after the last one the loop starts again from the first.
	std::list<engine::Ref<CouponButton>>::iterator aCouponIt = mCoupons.begin();
	while (aCouponIt != mCoupons.end())
	{
		engine::Ref<CouponButton> aCoupon = *aCouponIt;
		aCoupon->updateCoupon(theInfo);
		std::list<engine::Ref<CouponButton>>::iterator aNext = aCouponIt;
		++aNext;
		if (aCoupon->isGone() && aCoupon->getPosition()->x < COUPON_GONE_X)
		{
			removeChild(aCoupon);
			mCoupons.erase(aCouponIt);
			if (aNext == mCoupons.end())
				aCouponIt = mCoupons.begin();
			else
				aCouponIt = ++aNext;
		}
		else
		{
			aCouponIt = aNext;
		}
	}
	GenericScreen::update(theInfo);
}

// 0x4382d0
void Hud::updateStars()
{
	int aHappiness = mStats->mHappiness;
	int aShown = aHappiness - (int)((double)(aHappiness - mPreviousHappiness) * (double)mHappinessFade);
	int aStep = STAR_FIRST_STEP;
	for (int i = 0; i < STAR_COUNT; i++)
	{
		engine::ImageNode* aStar = mStars[i];
		int aLowest = i * STAR_HAPPINESS;
		int aHighest = aLowest + STAR_HAPPINESS;
		float aStarX = mStarsRect.left;
		float aStarY = getStarY(aStep);
		int aWidth = aStar->getImageWidth();
		int aHeight = aStar->getImageHeight();
		if (aShown >= aHighest)
		{
			// a star that the happiness fills is shown whole
			aStar->setClipRect(engine::Rect(0, 0, (float)aWidth, (float)aHeight));
			aStar->setPosition(aStarX, aStarY);
		}
		else if (aShown > aLowest)
		{
			// the star the happiness ends in shows its lower part, as much of it as is filled
			float aEmpty = (float)(1.0 - (double)(aShown - aLowest) * (double)STAR_HAPPINESS_SHARE);
			int aTop = (int)((double)aHeight * (double)aEmpty);
			aStar->setClipRect(engine::Rect(0, (float)aTop, (float)aWidth, (float)aHeight));
			aStar->setPosition(aStarX, (float)((double)aTop + (double)aStarY));
		}
		else
		{
			aStar->setClipRect(engine::Rect(0, 0, 0, 0));
		}
		aStep += STAR_STEP_INCREMENT;
	}
}

// 0x4393f0
void Hud::updateTipJar()
{
	int aMoney = mTipJarRolling ? mRollingTipMoney : mStats->mTipMoney;
	float aFill = (float)((double)aMoney / (double)mStats->mLevelGoal);
	if (1.0f < aFill)
		aFill = 1.0f;
	else if (!(0.0f < aFill))
		aFill = 0.0f;
	// the picture is moved up by the part of the jar that is empty, and its top is clipped away when that is under
	// the jar's top
	double aJarHeight = (double)mTipJarRect.bottom - (double)mTipJarRect.top;
	int anImageHeight = mTipJarContents->getImageHeight();
	double anEmptyHeight = aJarHeight - (double)anImageHeight * (double)aFill;
	float anOffset = (float)anEmptyHeight;
	float aClipTop = -anEmptyHeight < 0.0 ? 0.0f : (float)-anEmptyHeight;
	float aScaled = (float)((double)anImageHeight * (double)aFill);
	double aShownHeight = (double)aScaled < aJarHeight ? (double)aScaled : aJarHeight;
	int aClipBottom = (int)(aShownHeight + (double)aClipTop);
	mTipJarContents->setClipRect(engine::Rect(0, (float)(int)aClipTop, (float)mTipJarContents->getImageWidth(),
	                                          (float)aClipBottom));
	mTipJarContents->setPosition(TIP_JAR_CONTENTS_X, anOffset <= 0.0f ? 0.0f : anOffset);
	// the coins of the jar's effect follow the contents while the money rolls: left out, see the class
	mTipJarCount->setNumber(aMoney, MONEY_PREFIX);
	updateCoupons();
}

// 0x4388a0
void Hud::updateCoupons()
{
	std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
	for (aCouponIt = mCoupons.begin(); aCouponIt != mCoupons.end(); ++aCouponIt)
		(*aCouponIt)->updateAffordable(mStats->mTipMoney);
}

// 0x439650
void Hud::updateOrders()
{
	int anOrderCount = mStats->mOrderCount;
	mProgressStack->removeAllChildren();
	int aSlips = (anOrderCount + ORDER_SLIP_ROUNDING) / ORDERS_PER_SLIP;
	if (anOrderCount < 1)
	{
		aSlips = 0;
		mProgressText->setVisible(false);
	}
	else
	{
		mOrderCount->setNumber(anOrderCount, "");
		mProgressText->setVisible(true);
	}
	// the slips are put from the back one forward: each stands a little lower and to the right of the one before it
	for (int aSlip = aSlips - 1; aSlip >= 0; aSlip--)
	{
		engine::Image* anImage = engine::App::instance()->createImage(ORDER_SLIP_IMAGE, false);
		engine::ImageNode* aNode = new engine::ImageNode(anImage);
		aNode->setPosition(ORDER_SLIP_STEP * (float)aSlip, ORDER_SLIP_STEP * (float)aSlip);
		mProgressStack->addChild(aNode);
	}
}

// 0x43a5b0
void Hud::showCoupons()
{
	std::vector<engine::Ref<Topping>>& aToppings = PizzaFrenzy::instance()->mCityMap->mKitchenToppings;
	for (size_t i = 0; i < aToppings.size(); i++)
		makeCoupon(aToppings[i], COUPON_PRICE);
}

// 0x439b00
void Hud::makeCoupon(Topping* theTopping, int thePrice)
{
	CouponButton* aCoupon = new CouponButton();
	if (theTopping == nullptr)
		theTopping = PizzaFrenzy::instance()->mCityMap->getRandomKitchenTopping();
	aCoupon->setup(theTopping, thePrice);
	aCoupon->updateAffordable(mStats->mTipMoney);
	addCoupon(aCoupon);
}

// 0x4386d0
void Hud::startRoll(int theFrom)
{
	mTipJarRolling = true;
	mRollingTipMoney = theFrom;
	// a jar that loses money shows its coins (the jar's effect, 0x478fe0): left out, see the class
	if (mRollSound != nullptr)
	{
		mRollSound->setLoop(true);
		mRollSound->play();
	}
}

// 0x438720
void Hud::rollTipJar(engine::UpdateInfo& theInfo)
{
	int aStep = (int)((double)mStats->mLevelGoal / (double)TIP_JAR_ROLL_SECONDS * (double)theInfo.mSeconds);
	if (!mTipJarRolling)
		return;
	if (mRollingTipMoney > mStats->mTipMoney)
	{
		mRollingTipMoney = mRollingTipMoney - aStep;
		if (mRollingTipMoney > mStats->mTipMoney)
			return;
	}
	else
	{
		mRollingTipMoney = mRollingTipMoney + aStep;
		if (mRollingTipMoney < mStats->mTipMoney)
			return;
	}
	mRollingTipMoney = mStats->mTipMoney;
	mShownTipMoney = mStats->mTipMoney;
	mTipJarRolling = false;
	if (mRollSound != nullptr)
		mRollSound->stop();
	// the jar's coins stop (0x478ef0): left out, see the class
}

// 0x438270
void Hud::pauseTipJarSound(bool thePaused)
{
	if (!thePaused)
	{
		if (mRollSound != nullptr && mTipJarRolling)
			mRollSound->play();
	}
	else if (mRollSound != nullptr && mRollSound->isPlaying())
	{
		mRollSound->stop();
	}
}

// 0x4387f0
void Hud::stopTipJar()
{
	if (mRollSound != nullptr && mRollSound->isPlaying())
		mRollSound->stop();
	// the coins' emitter (+0x284) is stopped here: not translated, see the file's comment
}

// 0x4388d0
void Hud::slideCouponsOut()
{
	for (std::list<engine::Ref<CouponButton>>::iterator anIt = mCoupons.begin(); anIt != mCoupons.end(); ++anIt)
	{
		CouponButton* aCoupon = *anIt;
		engine::Point aTarget(aCoupon->getPosition()->x - COUPON_SLIDE_OUT, COUPON_ROW_Y);
		aCoupon->moveTo(aTarget, COUPON_SLIDE_SECONDS);
	}
}

// 0x4392e0
void Hud::resetHud()
{
	mWarningStars->removeAllListeners();
	mWarningStars->setVisible(false);
	mTipJarRolling = false;
	mShownOrderCount = 0;
	mShownTipMoney = 0;
	mHappinessFade = 0;
	mScoreFlashTime = 0;
	mShownPickCount = 0;
	mShownHappiness = mStats->mHappiness;
	updateTipJar();
	updateOrders();
	updateStars();
	mTipJarMax->setNumber(mStats->mLevelGoal, MONEY_PREFIX);
	clearCoupons();
}

// 0x4397d0
void Hud::updateHappiness()
{
	if (mStats->mHappiness < FULL_HAPPINESS)
		return;
	// the splat "happinessBonus" bursts at the stars' left and their ninth tenth (0x474a20): left out, see the class
	PizzaFrenzy::instance()->mSounds->play(SOUND_HAPPINESS_FULL, 1.0f, 1.0f);
	PizzaFrenzy::instance()->mMode->getStats()->addTips(FULL_HAPPINESS_TIPS);
}

// 0x438610
void Hud::setWarning(bool theShown)
{
	mWarningStars->removeAllListeners();
	if (theShown)
	{
		engine::Ref<engine::OpacityModifier> aFlash = new engine::OpacityModifier(WARNING_PERIOD, 0.0f, 1.0f, 0.0f);
		mWarningStars->addListener(aFlash);
		mWarningStars->setVisible(true);
		return;
	}
	mWarningStars->setVisible(false);
}

// 0x438570
void Hud::flashScoreBackground()
{
	engine::Ref<engine::OpacityModifier> aFlash = new engine::OpacityModifier(WARNING_PERIOD, 0.0f, 1.0f, 0.0f);
	mScoreBackground->setVisible(true);
	mScoreBackground->addListener(aFlash);
	mScoreFlashTime = FLASH_SECONDS;
}

// 0x4399d0
void Hud::addCoupon(CouponButton* theCoupon)
{
	int aPlaces = 0;
	std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
	for (aCouponIt = mCoupons.begin(); aCouponIt != mCoupons.end(); ++aCouponIt)
	{
		if (!(*aCouponIt)->isGone())
			aPlaces++;
	}
	float aRowX = COUPON_ROW_RIGHT - (float)(aPlaces * (int)COUPON_ROW_STEP);
	theCoupon->setPosition(aRowX - COUPON_ROW_OFFSCREEN, COUPON_ROW_Y);
	theCoupon->moveTo(engine::Point(aRowX, COUPON_ROW_Y), COUPON_COME_SECONDS);
	addChild(theCoupon);
	mCoupons.push_back(theCoupon);
}

// 0x438970
void Hud::removeCoupon(CouponButton* theCoupon)
{
	engine::Ref<CouponButton> aKept = theCoupon;
	std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
	for (aCouponIt = mCoupons.begin(); aCouponIt != mCoupons.end(); ++aCouponIt)
	{
		if (aCouponIt->get() == theCoupon)
		{
			mCoupons.erase(aCouponIt);
			break;
		}
	}
	removeChild(theCoupon);
	// the others close the gap: each goes to its place from the right
	int aOffset = 0;
	for (aCouponIt = mCoupons.begin(); aCouponIt != mCoupons.end(); ++aCouponIt)
	{
		(*aCouponIt)->moveTo(engine::Point(COUPON_ROW_RIGHT - (float)aOffset, COUPON_ROW_Y), COUPON_CLOSE_SECONDS);
		aOffset += (int)COUPON_ROW_STEP;
	}
}

// 0x438a40
void Hud::clearCoupons()
{
	std::list<engine::Ref<CouponButton>>::iterator aCouponIt;
	for (aCouponIt = mCoupons.begin(); aCouponIt != mCoupons.end(); ++aCouponIt)
		removeChild(*aCouponIt);
	mCoupons.clear();
}

// 0x4382d0, 0x439ba0: the star's y, the stars' bottom less the step-th tenth of their height (worked at the double
// precision the original's processor used)
float Hud::getStarY(int theStep) const
{
	double aHeight = (double)mStarsRect.bottom - (double)mStarsRect.top;
	return (float)((double)mStarsRect.bottom - aHeight * (double)STAR_SPACING * (double)theStep);
}
