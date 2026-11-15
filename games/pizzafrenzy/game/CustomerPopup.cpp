#include "CustomerPopup.h"
#include "App.h"
#include "GameMode.h"
#include "PizzaFrenzy.h"
#include "Tiles.h"

using namespace pizzafrenzy;

namespace
{
const char* const BUTTON_IMAGE = "res/pizza/orderButton.jpg";
const char* const UNKNOWN_IMAGE = "res\\pizza\\unknown.jpg";
const char* const SOUND_POPUP = "popup";
const char* const SOUND_HANGUP = "popup_hangup";
const char* const SOUND_SPECIAL_HANGUP = "special_hangup";
const char* const ACTION_CUSTOMER_HANGUP = "customerHangup";
const char* const ACTION_SPECIAL_HANGUP = "specialHangup";

const int SMOOTH_ON = 1;

// the three modifiers: how long a swing takes and how far it goes
const float SCALE_PERIOD = 0.5f;
const float SCALE_MIN = 0.7f;
const float SCALE_MAX = 1.1f;
const float OPACITY_PERIOD = 0.5f;
const float OPACITY_MIN = 0.6f;
const float OPACITY_MAX = 1.0f;
const float FLASH_PERIOD = 0.5f;
const float FLASH_SHARE = 0.5f;
const int FLASH_COLOR_MODE = 1;

// where the pictures stand on the popup
const float BUTTON_X = -4.0f;
const float BUTTON_Y = 2.0f;
const float FACE_X = -33.0f;
const float FACE_Y = -33.0f;
const float HOLDER_X = -4.0f;
const float HOLDER_Y = -2.0f;
const float EVENT_X = -2.0f;
const float EVENT_Y = -3.0f;
const float EVENT_SCALE = 0.8f;

// the pizzas of an order are in a column, the first one the highest
const float PIZZA_STEP = 3.0f;
const float PIZZA_HALF_STEP = 1.5f;

const float IMPATIENT_SECONDS = 2.0f;      // the wait that is left when the customer flashes
const float HANGUP_SECONDS = 0.5f;
const float CHAMELEON_SECONDS = 3.0f;      // the seconds before a chameleon changes again
const float CHAMELEON_STEP_SECONDS = 0.05f;
const int CHAMELEON_STEPS = 7;
const float PICKED_FADE_SECONDS = 0.5f;
const float DIM = -0.1f;
const int COLOR_MODE_ADD = 1;

// the cards of the order cursor: one for each order picked, five up from the one before, and the indicator of how
// many orders more can be picked, which goes under them
const float CARD_STEP = 5.0f;
const float INDICATOR_DROP = 45.0f;
const float INDICATOR_X = -29.0f;
const float INDICATOR_END_SHIFT_X = 5.0f;
const float INDICATOR_PART_SHIFT_X = 1.0f;
const float INDICATOR_SHIFT_Y = 4.0f;
const char* const INDICATOR_SINGLE_IMAGE = "res/pizza/orderIndicator.jpg";
const char* const INDICATOR_LEFT_IMAGE = "res/pizza/orderIndicatorL.jpg";
const char* const INDICATOR_MIDDLE_IMAGE = "res/pizza/orderIndicatorC.jpg";
const char* const INDICATOR_RIGHT_IMAGE = "res/pizza/orderIndicatorR.jpg";
const char* const INDICATOR_SINGLE_LIT_IMAGE = "res/pizza/orderIndicator_on.jpg";
const char* const INDICATOR_LEFT_LIT_IMAGE = "res/pizza/orderIndicatorL_on.jpg";
const char* const INDICATOR_MIDDLE_LIT_IMAGE = "res/pizza/orderIndicatorC_on.jpg";
const char* const INDICATOR_RIGHT_LIT_IMAGE = "res/pizza/orderIndicatorR_on.jpg";
const char* const INDICATOR_SINGLE_FLASH_IMAGE = "res/pizza/orderIndicator_flash.jpg";
const char* const INDICATOR_LEFT_FLASH_IMAGE = "res/pizza/orderIndicatorL_flash.jpg";
const char* const INDICATOR_MIDDLE_FLASH_IMAGE = "res/pizza/orderIndicatorC_flash.jpg";
const char* const INDICATOR_RIGHT_FLASH_IMAGE = "res/pizza/orderIndicatorR_flash.jpg";
enum { INDICATOR_LEFT = 0, INDICATOR_MIDDLE = 1, INDICATOR_RIGHT = 2, INDICATOR_SINGLE = 3 };

GameMode* getMode()
{
	return PizzaFrenzy::instance()->mMode;
}

CustomerPopup* asCustomer(PizzaPopup* thePopup)
{
	return static_cast<CustomerPopup*>(thePopup);
}

void playSound(const std::string& theName)
{
	PizzaFrenzy::instance()->mSounds->play(theName, 1.0f, 1.0f);
}
}

// 0x452220
CustomerPopup::CustomerPopup()
{
}

// 0x452b10
CustomerPopup::~CustomerPopup()
{
}

// 0x454480
void CustomerPopup::setOwner(BuildingTile* theTile)
{
	PizzaPopup::setOwner(theTile);
	mScaleFlash = new engine::ScaleModifier(SCALE_PERIOD, SCALE_MIN, SCALE_MAX, SCALE_PERIOD, SCALE_MIN, SCALE_MAX, 0.0f);
	mOpacityFlash = new engine::OpacityModifier(OPACITY_PERIOD, OPACITY_MIN, OPACITY_MAX, 0.0f);
	mColorFlash = new engine::ColorFlashModifier(FLASH_PERIOD, engine::Color4(FLASH_SHARE, FLASH_SHARE, FLASH_SHARE, 1.0f),
	                                             0.0f, FLASH_COLOR_MODE);

	mFace = new engine::Selector();
	addChild(mFace);
	mFront = new engine::Container();
	mBack = new engine::Container();
	mFace->addChild(mFront);
	mFace->addChild(mBack);

	engine::Image* aButtonImage = engine::App::instance()->createImage(BUTTON_IMAGE, false);
	aButtonImage->setPivot(1);
	mButton = new engine::ImageNode(aButtonImage);
	mButton->setPosition(BUTTON_X, BUTTON_Y);
	mFront->addChild(mButton);

	mSpecial = new engine::Selector();
	mFront->addChild(mSpecial);
	// the container the button's was; the pictures of the customer and the order go in this one
	mFront = new engine::Container();
	mSpecial->addChild(mFront);
	mSpecial->select(0, 0.0f, true);

	mFaceImage = new engine::ImageNode(nullptr);
	mFaceImage->setPosition(FACE_X, FACE_Y);
	mFront->addChild(mFaceImage);
	mFaceImageBack = new engine::ImageNode(nullptr);
	mFaceImageBack->setPosition(0.0f, 0.0f);
	mBack->addChild(mFaceImageBack);

	mOrderHolder = new engine::Container();
	mFront->addChild(mOrderHolder);
	mOrderHolder->setPosition(HOLDER_X, HOLDER_Y);

	engine::Image* anUnknownImage = engine::App::instance()->createImage(UNKNOWN_IMAGE, false);
	anUnknownImage->setPivot(1);
	mUnknown = new engine::ImageNode(anUnknownImage);
	mFront->addChild(mUnknown);
	mUnknown->setVisible(false);

	mEventImage = new engine::ImageNode(nullptr);
	mEventImage->setScale(EVENT_SCALE, EVENT_SCALE);
	mEventImage->setPosition(EVENT_X, EVENT_Y);
	mEventImage->setSmoothType(SMOOTH_ON);
	mSpecial->addChild(mEventImage);
}

// 0x4527c0
void CustomerPopup::setSelectionVisible(bool theVisible)
{
	PizzaPopup::setSelectionVisible(theVisible);
	if (mSpecial != nullptr && mSpecial->getCount() > 1 && mOrder != nullptr && mOrder->mEvent != nullptr &&
	    getMode()->getLevel()->getDeliverOnlyCharacter() == nullptr)
		mSpecial->select(theVisible ? 1 : 0, 0.0f, true);
}

// 0x453510
void CustomerPopup::rest()
{
	if (!mDimmed)
		mStates.setState(new WaitingState(this));
	else
		mStates.setState(new DimmedState(this));
}

// 0x452e20
void CustomerPopup::hover()
{
	PizzaPopup::hover();
	mStates.setState(new HoverState(this));
}

// 0x452ed0
void CustomerPopup::pick()
{
	mStates.setState(new PickedState(this));
}

// 0x453140
void CustomerPopup::serve()
{
	PizzaPopup::serve();
	removeAllListeners();
	setOpacity(1.0f);
	setColorMode(0);
	setScale(1.0f);
	mStates.setState(new ServedState(this));
}

// 0x452f70
void CustomerPopup::unpick()
{
	PizzaPopup::unpick();
	mButton->setVisible(true);
	mStates.setState(new WaitingState(this));
}

// 0x4532e0
void CustomerPopup::hangUp()
{
	if (mOrder != nullptr)
		mStates.setState(new HangupState(this));
}

// 0x453e50
void CustomerPopup::setOrder(Order* theOrder, bool theDimmed)
{
	mOrder = theOrder;
	engine::Image* aFace = theOrder->mCharacter->mImage;
	mFaceImage->setImage(aFace);
	mFaceImageBack->setImage(aFace);
	showOrder(mOrderHolder, theOrder);
	mFace->select(0, 0.0f, true);
	Level* aLevel = getMode()->getLevel();
	if (mOrder->mEvent != nullptr || aLevel->getDeliverOnlyCharacter() != nullptr)
		mEventImage->setImage(mOrder->mCharacter->mImage);
	if (aLevel->getDeliverOnlyCharacter() != nullptr)
		mSpecial->select(1, 0.0f, true);
	playGreeting();
	mChameleon = mOrder->mCharacter->mSpecial == Character::SPECIAL_CHAMELEON && getMode()->isSpeedMode();
	mChameleonTimer = mChameleon ? CHAMELEON_SECONDS : 0.0f;
	mDimmed = theDimmed;
	mWaitLeft = mOrder->mWaitTime;
	mStates.setState(new AppearState(this, !theDimmed));
}

// 0x452c10
void CustomerPopup::replaceOrder(Order* theOrder, bool theWait)
{
	mOrder = theOrder;
	showOrder(mOrderHolder, theOrder);
	mChameleon = false;
	mChameleonTimer = 0.0f;
	revealOrder();
	if (theWait)
		mStates.setState(new WaitingState(this));
}

// 0x452d20
void CustomerPopup::startChameleon(Order* theOrder, bool theQuiet)
{
	mOrder = theOrder;
	mChameleon = false;
	mChameleonTimer = 0.0f;
	revealOrder();
	mStates.setState(new ChameleonState(this, theOrder->mTopping, theQuiet));
}

// 0x452b30
void CustomerPopup::setChameleon(bool theChameleon)
{
	mChameleon = theChameleon;
	mStates.setState(new WaitingState(this));
	mChameleonTimer = mChameleon ? CHAMELEON_SECONDS : 0.0f;
}

// 0x453210
void CustomerPopup::finish()
{
	if (mOrder != nullptr)
		getMode()->orderFinished(mOwner, mOrder, mElapsed);
	mStates.setState(new ClosingState(this));
}

// 0x453020
void CustomerPopup::setImpatient(bool theImpatient)
{
	if (mOrder == nullptr)
		return;
	if (!theImpatient)
		mStates.setState(new HangupState(this));
	else
		mStates.setState(new ImpatientState(this));
}

// 0x452840
void CustomerPopup::flash()
{
	if (mOrder != nullptr && isClickable())
		addListener(mColorFlash);
}

// 0x453390
void CustomerPopup::hideOrder()
{
	mUnknown->setVisible(true);
	mOrderHolder->setVisible(false);
	setOpacity(1.0f);
	setScale(1.0f);
	mStates.setState(new WaitingState(this));
}

// 0x4528a0
void CustomerPopup::revealOrder()
{
	mUnknown->setVisible(false);
	mOrderHolder->setVisible(true);
}

// 0x452660
void CustomerPopup::showOrder(engine::Container* theHolder, Order* theOrder)
{
	theHolder->removeAllChildren();
	if (mUnknown->isVisible())
	{
		engine::Image* anImage = engine::App::instance()->createImage(UNKNOWN_IMAGE, false);
		anImage->setPivot(1);
		theHolder->addChild(new engine::ImageNode(anImage));
		return;
	}
	Topping* aTopping = theOrder->mTopping;
	if (aTopping == nullptr)
		return;
	float aY = (float)(theOrder->mPizzaCount - 1) * PIZZA_HALF_STEP;
	for (int i = 0; i < theOrder->mPizzaCount; i++)
	{
		engine::ImageNode* aPizza = new engine::ImageNode(aTopping->mPreviewImage);
		aPizza->setPosition(0.0f, aY);
		theHolder->addChild(aPizza);
		aY = aY - PIZZA_STEP;
	}
}

// 0x453930
void CustomerPopup::playGreeting()
{
	if (mOrder->mEvent == nullptr && PizzaFrenzy::instance()->mGameMode != 1)
	{
		playSound(SOUND_POPUP);
		return;
	}
	playSound(mOrder->mCharacter->mGreeting);
}

// 0x453460
void CustomerPopup::tickChameleon(engine::UpdateInfo& theInfo)
{
	if (0.0f < mChameleonTimer)
	{
		mChameleonTimer = mChameleonTimer - theInfo.mSeconds;
		if (mChameleonTimer <= 0.0f)
			mStates.setState(new ChameleonState(this, nullptr, false));
	}
}

// 0x4528c0
void WaitingState::enter()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	if (aPopup->mFace != nullptr)
	{
		aPopup->mFace->select(0, 0.0f, true);
		aPopup->mFace->setVisible(true);
	}
}

// 0x453620
void WaitingState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mWaitLeft = aPopup->mWaitLeft - theInfo.mSeconds;
	aPopup->mElapsed = aPopup->mElapsed + theInfo.mSeconds;
	if (aPopup->mWaitLeft <= IMPATIENT_SECONDS)
		aPopup->mStates.setState(new ImpatientState(aPopup));
	else
		aPopup->tickChameleon(theInfo);
}

// 0x4528f0: the opacity flashes, unless the level takes one customer's pizzas only or the customer of an event wants
// none
void ImpatientState::enter()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	if (getMode()->getLevel()->getDeliverOnlyCharacter() == nullptr)
	{
		SpecialEvent* anEvent = aPopup->getOrder()->mEvent;
		if (anEvent == nullptr || anEvent->mDeliver)
			aPopup->addListener(aPopup->mOpacityFlash);
	}
}

// 0x452950
void ImpatientState::exit()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->removeListener(aPopup->mOpacityFlash);
	aPopup->setOpacity(1.0f);
}

// 0x453730
void ImpatientState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mWaitLeft = aPopup->mWaitLeft - theInfo.mSeconds;
	aPopup->mElapsed = aPopup->mElapsed + theInfo.mSeconds;
	if (aPopup->mWaitLeft <= 0.0f)
		aPopup->mStates.setState(new HangupState(aPopup));
	else
		aPopup->tickChameleon(theInfo);
}

// 0x453a10: the mode is told the customer has hung up, and the sound is the popup's; the customer of an event has
// its own sound and signal; a level that takes one customer's pizzas only makes no sound for another's
void HangupState::enter()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->setLeaving(true);
	mTimeLeft = HANGUP_SECONDS;
	SpecialEvent* anEvent = aPopup->getOrder()->mEvent;
	if (anEvent == nullptr || anEvent->mDeliver)
	{
		getMode()->action(ACTION_CUSTOMER_HANGUP);
		Character* anOnly = getMode()->getLevel()->getDeliverOnlyCharacter();
		if (anOnly != nullptr && anOnly != aPopup->getOrder()->mCharacter)
			return;
		playSound(SOUND_HANGUP);
	}
	else
	{
		playSound(SOUND_SPECIAL_HANGUP);
		getMode()->action(ACTION_SPECIAL_HANGUP);
	}
}

// 0x452100
void HangupState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
		mTimeLeft = 0.0f;
	mOwner->setOpacity(mTimeLeft / HANGUP_SECONDS);
	mOwner->setScale((mTimeLeft / HANGUP_SECONDS) * 0.5f + 0.5f);
	if (mTimeLeft <= 0.0f)
		mOwner->leave();
}

// 0x452980
void PickedState::enter()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	if (aPopup->mFace != nullptr)
	{
		aPopup->mFace->select(1, PICKED_FADE_SECONDS, false);
		aPopup->mFace->setVisible(true);
	}
	aPopup->PizzaPopup::setSelectionVisible(false);
}

// 0x452060
void PickedState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mWaitLeft = aPopup->mWaitLeft - theInfo.mSeconds;
	aPopup->mElapsed = aPopup->mElapsed + theInfo.mSeconds;
}

// 0x452090
void ServedState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mElapsed = aPopup->mElapsed + theInfo.mSeconds;
}

// 0x452060
void HoverState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mWaitLeft = aPopup->mWaitLeft - theInfo.mSeconds;
	aPopup->mElapsed = aPopup->mElapsed + theInfo.mSeconds;
}

// 0x452a40
ChameleonState::ChameleonState(PizzaPopup* theOwner, Topping* theTarget, bool theQuiet) :
	PopupState(theOwner),
	mTarget(theTarget),
	mQuiet(theQuiet)
{
}

// 0x453840: the popup is plain, and the next topping is the one the order's own is followed by in the map's list, which
// is where the order depends on the toppings' map (the kitchens' toppings, each once, in the order the map has them)
void ChameleonState::enter()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->removeAllListeners();
	aPopup->setOpacity(1.0f);
	aPopup->setColorMode(0);
	aPopup->setScale(1.0f);
	mStepTime = 0.0f;
	std::vector<engine::Ref<Topping>>& aList = PizzaFrenzy::instance()->mCityMap->mKitchenToppings;
	if (mTarget != nullptr)
	{
		mNext = 0;
		while (mNext != aList.size() && aList[mNext].get() != mTarget)
			mNext++;
		mStepsLeft = CHAMELEON_STEPS;
		return;
	}
	mNext = 0;
	while (mNext != aList.size() && aList[mNext].get() != aPopup->getOrder()->mTopping)
		mNext++;
	mNext++;
	if (mNext >= aList.size())
		mNext = 0;
	mTarget = aList.empty() ? nullptr : aList[mNext].get();
	mStepsLeft = CHAMELEON_STEPS;
}

// 0x453c10: every twentieth of a second the order shows the next topping of the list, until seven have shown
void ChameleonState::update(engine::UpdateInfo& theInfo)
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	aPopup->mWaitLeft = aPopup->mWaitLeft - theInfo.mSeconds;
	mStepTime = mStepTime - theInfo.mSeconds;
	if (mStepTime > 0.0f)
		return;
	mStepTime = CHAMELEON_STEP_SECONDS;
	std::vector<engine::Ref<Topping>>& aList = PizzaFrenzy::instance()->mCityMap->mKitchenToppings;
	Topping* aTopping = mNext < aList.size() ? aList[mNext].get() : nullptr;
	mNext++;
	if (aTopping != nullptr)
	{
		Order aShown(*aPopup->getOrder());
		aShown.mTopping = aTopping;
		aPopup->showOrder(aPopup->mOrderHolder, &aShown);
		if (!mQuiet)
			playSound(SOUND_POPUP);
	}
	if (mNext == aList.size())
		mNext = 0;
	mStepsLeft--;
	if (mStepsLeft > 0)
		return;
	if (!aPopup->mDimmed)
		aPopup->mStates.setState(new WaitingState(aPopup));
	else
		aPopup->mStates.setState(new DimmedState(aPopup));
}

// 0x453dd0: the order has the topping the changes stopped on (when they went all the way), and the next change is 3
// seconds away for a chameleon
void ChameleonState::exit()
{
	CustomerPopup* aPopup = asCustomer(mOwner);
	if (mStepsLeft < 1)
	{
		aPopup->getOrder()->mTopping = mTarget;
		aPopup->showOrder(aPopup->mOrderHolder, aPopup->getOrder());
	}
	else
	{
		aPopup->showOrder(aPopup->mOrderHolder, aPopup->getOrder());
		aPopup->playGreeting();
	}
	aPopup->mChameleonTimer = aPopup->mChameleon ? CHAMELEON_SECONDS : 0.0f;
}

// 0x4520b0
void DimmedState::enter()
{
	mOwner->setColorMode(COLOR_MODE_ADD);
	mOwner->setColor(DIM, DIM, DIM);
}

// 0x4520e0
void DimmedState::exit()
{
	mOwner->setColorMode(0);
	mOwner->setColor(0.0f, 0.0f, 0.0f);
}

// 0x454050
void CustomerPopup::buildPickCards(engine::Container* theCursor)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	int aPickCount = anApp->mMode->getPickCount();
	engine::Image* aCard = engine::App::instance()->createImage(BUTTON_IMAGE, false);
	aCard->setPivot(1);
	float aY = 0;
	for (int i = 0; i < aPickCount; i++)
	{
		engine::ImageNode* aNode = new engine::ImageNode(aCard);
		aNode->setPosition(0.0f, aY);
		aY = aY - CARD_STEP;
		theCursor->addChild(aNode);
	}
	engine::Container* aPizzas = new engine::Container();
	theCursor->addChild(aPizzas);
	aPizzas->setPosition(0.0f, aY);
	showOrder(aPizzas, mOrder);
	float anIndicatorY = aY - INDICATOR_DROP;
	engine::Container* aFlashHolder = new engine::Container();
	int aLimit = anApp->mGameStats->getPickLimit(mOrder->mTopping);
	if (aLimit == 1)
	{
		engine::Image* aSingle = engine::App::instance()->createImage(INDICATOR_SINGLE_IMAGE, false);
		engine::ImageNode* aNode = new engine::ImageNode(aSingle);
		aNode->setPosition(INDICATOR_X, anIndicatorY);
		theCursor->addChild(aNode);
		if (0 < aPickCount)
			addIndicator(theCursor, aFlashHolder, INDICATOR_X, anIndicatorY, INDICATOR_SINGLE);
	}
	else
	{
		engine::Image* aLeft = engine::App::instance()->createImage(INDICATOR_LEFT_IMAGE, false);
		engine::ImageNode* aLeftNode = new engine::ImageNode(aLeft);
		aLeftNode->setPosition(INDICATOR_X, anIndicatorY);
		theCursor->addChild(aLeftNode);
		if (0 < aPickCount)
			addIndicator(theCursor, aFlashHolder, INDICATOR_X, anIndicatorY, INDICATOR_LEFT);
		float aX = (float)aLeft->getWidth() + INDICATOR_X;
		for (int i = 0; i < aLimit - 2; i++)
		{
			engine::Image* aMiddle = engine::App::instance()->createImage(INDICATOR_MIDDLE_IMAGE, false);
			engine::ImageNode* aMiddleNode = new engine::ImageNode(aMiddle);
			aMiddleNode->setPosition(aX, anIndicatorY);
			theCursor->addChild(aMiddleNode);
			if (i + 1 < aPickCount)
				addIndicator(theCursor, aFlashHolder, aX, anIndicatorY, INDICATOR_MIDDLE);
			aX = (float)aMiddle->getWidth() + aX;
		}
		engine::Image* aRight = engine::App::instance()->createImage(INDICATOR_RIGHT_IMAGE, false);
		engine::ImageNode* aRightNode = new engine::ImageNode(aRight);
		aRightNode->setPosition(aX, anIndicatorY);
		theCursor->addChild(aRightNode);
		if (aPickCount == aLimit)
			addIndicator(theCursor, aFlashHolder, aX, anIndicatorY, INDICATOR_RIGHT);
	}
	theCursor->addChild(aFlashHolder);
	anApp->mMode->getScreen()->setFlashIndicators(aFlashHolder);
}

// 0x451e50
void CustomerPopup::addIndicator(engine::Container* theCursor, engine::Container* theFlashHolder, float theX,
                                 float theY, int theKind)
{
	const char* aLitImage = nullptr;
	const char* aFlashImage = nullptr;
	switch (theKind)
	{
	case INDICATOR_LEFT:
		aLitImage = INDICATOR_LEFT_LIT_IMAGE;
		aFlashImage = INDICATOR_LEFT_FLASH_IMAGE;
		theX = theX + INDICATOR_END_SHIFT_X;
		theY = theY + INDICATOR_SHIFT_Y;
		break;
	case INDICATOR_MIDDLE:
		aLitImage = INDICATOR_MIDDLE_LIT_IMAGE;
		aFlashImage = INDICATOR_MIDDLE_FLASH_IMAGE;
		theX = theX + INDICATOR_PART_SHIFT_X;
		theY = theY + INDICATOR_SHIFT_Y;
		break;
	case INDICATOR_RIGHT:
		aLitImage = INDICATOR_RIGHT_LIT_IMAGE;
		aFlashImage = INDICATOR_RIGHT_FLASH_IMAGE;
		theX = theX + INDICATOR_PART_SHIFT_X;
		theY = theY + INDICATOR_SHIFT_Y;
		break;
	case INDICATOR_SINGLE:
		aLitImage = INDICATOR_SINGLE_LIT_IMAGE;
		aFlashImage = INDICATOR_SINGLE_FLASH_IMAGE;
		theX = theX + INDICATOR_END_SHIFT_X;
		theY = theY + INDICATOR_SHIFT_Y;
		break;
	default:
		return;
	}
	engine::ImageNode* aLit = new engine::ImageNode(engine::App::instance()->createImage(aLitImage, false));
	aLit->setPosition(theX, theY);
	theCursor->addChild(aLit);
	engine::ImageNode* aFlash = new engine::ImageNode(engine::App::instance()->createImage(aFlashImage, false));
	aFlash->setPosition(theX, theY);
	theFlashHolder->addChild(aFlash);
}
