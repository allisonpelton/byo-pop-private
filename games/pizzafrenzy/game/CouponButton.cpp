#include "CouponButton.h"
#include "App.h"
#include "GameMode.h"
#include "Interpolator.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

using namespace pizzafrenzy;

namespace
{
const char* const FRAME_IMAGE = "res\\hud\\couponFrame.jpg";
const char* const FRAME_LIT_IMAGE = "res\\hud\\couponFrame_on.jpg";
const char* const CAPTION_IMAGE = "res\\menuAssets\\toppingUpgradeCaption.jpg";
const char* const PRICE_FONT = "res\\fonts\\hudFont.xml";
const char* const CAPTION_FONT = "res\\fonts\\bookFont.xml";
const char* const PRICE_PREFIX = "$";
const char* const SOUND_INVALID = "order_invalidClick";
const char* const SOUND_CLICK = "order_click1";
const char* const SOUND_MOUSE_OVER = "order_mouseover1";

const int STRING_COUPON_NAME = 0xe6;

// where the pictures of the button stand
const float COUPON_IMAGE_X = 0.0f;
const float COUPON_IMAGE_Y = -2.0f;
const float TOPPING_IMAGE_X = -3.0f;
const float TOPPING_IMAGE_Y = -10.0f;
const float PRICE_X = 23.0f;
const float PRICE_Y = 17.0f;
const float CAPTION_Y = 50.0f;

const int ALIGN_CENTRE = 1;
const int ALIGN_RIGHT = 2;
const int COLOR_MODE_ADD = 1;
const int COLOR_MODE_REPLACE = 2;
const float DIM_COLOR = -0.3f;  // what is added to a button the tips do not buy

// the slide: two keys, from where the button is to the point, over the seconds the state was given
const float SLIDE_END_TIME = 1.0f;
const float SLIDE_END_IN_TIME = 0.2f;
const float SLIDE_SPEED = 1.0f;

void playSound(const char* theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, 1.0f, 1.0f);
}

// A slide of a coupon button to a point (0x98 bytes, vtable 0x4ffc6c): the button's position follows an ease curve
// from where it was to the point
class CouponMoveState : public engine::State
{
public:
	CouponMoveState(CouponButton* theButton, const engine::Point& thePoint, float theSeconds)  // 0x44ed90
	    : mButton(theButton), mTarget(thePoint), mSeconds(theSeconds)
	{
	}

	// 0x04: 0x44eb20
	void enter() override
	{
		engine::PositionKey aStart;
		aStart.mTime = 0;
		aStart.mValue = *mButton->getPosition();
		aStart.mInTime = 0;
		aStart.mInSpeed = SLIDE_SPEED;
		aStart.mOutTime = SLIDE_END_TIME;
		aStart.mOutSpeed = SLIDE_SPEED;
		mMotion.addKey(aStart);
		engine::PositionKey anEnd;
		anEnd.mTime = SLIDE_END_TIME;
		anEnd.mValue = mTarget;
		anEnd.mInTime = SLIDE_END_IN_TIME;
		anEnd.mInSpeed = SLIDE_SPEED;
		anEnd.mOutTime = 0;
		anEnd.mOutSpeed = SLIDE_SPEED;
		mMotion.addKey(anEnd);
		mRamp.set(mSeconds, SLIDE_END_TIME);
	}

	// 0x0c: 0x44ec10: when it has got there the button shows whether the tips buy it
	void update(engine::UpdateInfo& theInfo) override
	{
		mRamp.update(theInfo.mSeconds);
		mMotion.modify(mRamp.getProgress(), mButton);
		if (mRamp.isFinished())
			mButton->updateAffordable(PizzaFrenzy::instance()->mGameStats->mTipMoney);
	}

private:
	CouponButton* mButton;                 // +0x0c
	engine::PositionInterpolator mMotion;  // +0x10
	engine::Ramp mRamp;                    // +0x60
	engine::Point mTarget;                 // +0x84
	float mSeconds;                        // +0x8c
};
}

// 0x44fa80
CouponButton::CouponButton() : mPrice(0), mAffordable(false), mGone(false), mTopping(nullptr)
{
}

// 0x44f160 (0x44ef50)
CouponButton::~CouponButton()
{
}

// 0x44f500
void CouponButton::setup(Topping* theTopping, int thePrice)
{
	engine::ImageNode* aCouponImage = new engine::ImageNode(theTopping->mCouponImage);
	aCouponImage->setPosition(COUPON_IMAGE_X, COUPON_IMAGE_Y);
	addChild(aCouponImage);
	engine::ImageNode* aToppingImage = new engine::ImageNode(theTopping->mScaledImage);
	aToppingImage->setPosition(TOPPING_IMAGE_X, TOPPING_IMAGE_Y);
	addChild(aToppingImage);
	mFrame = new engine::Selector();
	addChild(mFrame);
	engine::Image* aFrame = engine::App::instance()->createImage(FRAME_IMAGE, false);
	aFrame->setPivot(1);
	engine::Image* aLitFrame = engine::App::instance()->createImage(FRAME_LIT_IMAGE, false);
	aLitFrame->setPivot(1);
	mFrame->addChild(new engine::ImageNode(aFrame));
	mFrame->addChild(new engine::ImageNode(aLitFrame));
	mFrame->select(0, 0.0f, true);
	// the original asks for the topping's upgrade here and does nothing with it (0x41a610)
	mCaptionText = formatMessage(engine::App::instance()->loadString(STRING_COUPON_NAME), theTopping->mDisplayName);
	mPrice = thePrice;
	if (0 < thePrice)
	{
		mPriceText = new engine::TextItem();
		mPriceText->setFont(engine::App::instance()->getFont(PRICE_FONT));
		mPriceText->setHAlign(ALIGN_RIGHT);
		mPriceText->setVAlign(ALIGN_CENTRE);
		mPriceText->setColorMode(COLOR_MODE_REPLACE);
		mPriceText->setColor(0.0f, 0.0f, 0.0f);
		mPriceText->setPosition(PRICE_X, PRICE_Y);
		addChild(mPriceText);
	}
	setPrice(thePrice);
	mCaption = new engine::Container();
	mCaption->setPosition(0.0f, CAPTION_Y);
	addChild(mCaption);
	mNameText = new engine::TextItem();
	mNameText->setFont(engine::App::instance()->getFont(CAPTION_FONT));
	mNameText->setColorMode(COLOR_MODE_REPLACE);
	mNameText->setColor(0.0f, 0.0f, 0.0f);
	mNameText->setHAlign(ALIGN_CENTRE);
	mNameText->setVAlign(ALIGN_CENTRE);
	mNameText->setText(mCaptionText);
	engine::Image* aCaptionImage = engine::App::instance()->createImage(CAPTION_IMAGE, false);
	aCaptionImage->setPivot(1);
	mCaption->addChild(new engine::ImageNode(aCaptionImage));
	mCaption->addChild(mNameText);
	mCaption->setVisible(false);
	mTopping = theTopping;
	mGone = false;
}

// 0x44eaa0
void CouponButton::updateCoupon(engine::UpdateInfo& theInfo)
{
	mStates.update(theInfo);
}

// 0x44f3f0
void CouponButton::buy()
{
	GameStats* aStats = PizzaFrenzy::instance()->mGameStats;
	int aPickCount = PizzaFrenzy::instance()->mMode->getPickCount();
	int aTotal = mPrice;
	if (0 < aPickCount)
		aTotal = aTotal * aPickCount;
	if (aTotal <= aStats->mTipMoney)
	{
		engine::Ref<CouponButton> aKept = this;
		leaveHud();
		PizzaFrenzy::instance()->mMode->pickCoupon(this);
		return;
	}
	PizzaFrenzy::instance()->mHud->flashScoreBackground();
	playSound(SOUND_INVALID);
}

// 0x44ed20
void CouponButton::putBack()
{
	PizzaFrenzy::instance()->mHud->addCoupon(this);
	mFrame->select(0, 0.0f, true);
}

// 0x44ed50
void CouponButton::leaveHud()
{
	PizzaFrenzy::instance()->mHud->removeCoupon(this);
	mCaption->setVisible(false);
	mFrame->select(0, 0.0f, true);
}

// 0x44f0d0
void CouponButton::moveTo(const engine::Point& thePoint, float theSeconds)
{
	if (thePoint.x < getPosition()->x)
		mGone = true;
	mStates.changeState(new CouponMoveState(this, thePoint, theSeconds));
}

// 0x44f180
void CouponButton::setPrice(int thePrice)
{
	mPrice = thePrice;
	if (mPriceText != nullptr)
		mPriceText->setNumber(mPrice, PRICE_PREFIX);
}

// 0x44eac0
void CouponButton::updateAffordable(int theTipMoney)
{
	int aTotal = mPrice * PizzaFrenzy::instance()->mMode->getPickCount();
	if (aTotal <= mPrice)
		aTotal = mPrice;
	mAffordable = aTotal <= theTipMoney;
	if (!mAffordable)
	{
		setColorMode(COLOR_MODE_ADD);
		setColor(DIM_COLOR, DIM_COLOR, DIM_COLOR);
		return;
	}
	setColorMode(0);
}

// 0x44eca0
void CouponButton::layout()
{
	mBounds = mRect;
	mFrame->layout();
	mBounds.unite(*mFrame->getBounds());
	mBounds.scale(mScale.x, mScale.y);
	mBounds.normalize();
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(engine::NODE_LAYOUT);
}

// 0x44f210
void CouponButton::onMouseDownEvent(engine::MouseEvent& theEvent)
{
	if (!mAffordable)
	{
		playSound(SOUND_INVALID);
		PizzaFrenzy::instance()->mHud->flashScoreBackground();
		return;
	}
	buy();
	playSound(SOUND_CLICK);
}

// 0x44f330
void CouponButton::onMouseEnterEvent(engine::MouseEvent& theEvent)
{
	if (mAffordable)
	{
		mCaption->setVisible(true);
		mFrame->select(1, 0.0f, true);
		playSound(SOUND_MOUSE_OVER);
	}
}

// 0x44ec70
void CouponButton::onMouseExitEvent(engine::MouseEvent& theEvent)
{
	mCaption->setVisible(false);
	mFrame->select(0, 0.0f, true);
}
