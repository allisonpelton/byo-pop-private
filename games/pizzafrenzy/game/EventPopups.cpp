#include "EventPopups.h"
#include "App.h"
#include "GameText.h"
#include "PizzaFrenzy.h"
#include "RectangleItem.h"
#include "ScreenNodes.h"
#include "CustomerPopup.h"
#include "Tiles.h"

#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const BAR_FONT = "res\\fonts\\dialogFontBig.xml";
const char* const GRAFFITI_IMAGE = "res\\specialEvents\\graffitiArt.jpg";
const char* const GRAFFITI_SOUND = "special_graffiti";

// the splats the events make (the definitions of res\manifests\splats.xml): the portrait that flies from the customer's
// house to the bar, the money that flies between the tip jar and the portrait, and the happiness that flies to it
const char* const PORTRAIT_SPLAT = "portraitSplat";
const char* const BILLS_SPLAT = "StealBills";
const char* const SATISFACTION_SPLAT = "StealSatisfaction";

// the bar at the bottom of the screen: its shade, and where the text and the portrait stand
const float BAR_LEFT = 0.0f;
const float BAR_TOP = 515.0f;
const float BAR_RIGHT = 800.0f;
const float BAR_BOTTOM = 600.0f;
const float BAR_SHADE = 0.25f;
const float CAPTION_X = 110.0f;
const float CAPTION_Y = 525.0f;
const float CAPTION_WRAP = 580.0f;
const float CAPTION_RATE = 120.0f;
const float CAPTION_SENTENCE_PAUSE = 0.0f;
const float PORTRAIT_X = 45.0f;
const float PORTRAIT_Y = 530.0f;
const float BAR_FADE_IN_SECONDS = 0.5f;
const float BAR_FADE_OUT_SECONDS = 0.3f;
const float BAR_FADE_TARGET = 1.0f;
const float SOUND_VOLUME = 1.0f;
const float SOUND_PITCH = 1.0f;

// the event types the factory knows
const char* const TYPE_THIEF = "Thief";
const char* const TYPE_GOSSIP = "Gossip";
const char* const TYPE_MOVIE_STAR = "MovieStar";
const char* const TYPE_CRANK_CALLER = "CrankCaller";
const char* const TYPE_MONK = "Monk";
const char* const TYPE_CLOWN = "Clown";
const char* const TYPE_BANKER = "TreasureHunter";
const char* const TYPE_GRAFFITI_ARTIST = "GraffitiArtist";

// the Thief takes ten hundreds, a tenth of a second apart and a second after the last
const int THIEF_BILLS = 10;
const int THIEF_BILL_VALUE = 100;
const float THIEF_BILL_SECONDS = 0.1f;
const float LAST_BILL_SECONDS = 1.0f;
// the Movie Star gives five hundreds, in the same time
const int STAR_TIPS = 5;
const int STAR_TIP_VALUE = 100;
// how long the events that only say something stay after they have
const float TALK_SECONDS = 2.0f;
// the Monk slows the time to a third for eight seconds
const float MONK_SLOW_SECONDS = 8.0f;
const float MONK_SLOW_FACTOR = 0.33f;
// the Banker takes up a tip every half second, ten times
const int BANKER_TIPS = 10;
const float BANKER_TIP_SECONDS = 0.5f;
// the Graffiti Artist: the picture stands under the portrait's text, and shows from left to right in two seconds
const float GRAFFITI_X = 110.0f;
const float GRAFFITI_Y = 500.0f;
const float GRAFFITI_SHOW_RATE = -0.5f;
const int GRAFFITI_HAPPINESS = -10;
const float GRAFFITI_LAST_SECONDS = 1.0f;

enum GraffitiStage
{
	GRAFFITI_START = 0,
	GRAFFITI_PAINT = 1,
	GRAFFITI_STEAL = 2,
	GRAFFITI_DONE = 3
};

GameMode* getMode()
{
	return PizzaFrenzy::instance()->mMode;
}

// the customers' houses whose popup takes a click
std::vector<CustomerTile*> getClickableHouses()
{
	std::vector<CustomerTile*> aHouses;
	CityMap* aMap = PizzaFrenzy::instance()->mCityMap;
	for (size_t i = 0; i < aMap->mCustomers.size(); i++)
	{
		CustomerTile* aHouse = static_cast<CustomerTile*>(aMap->mCustomers[i].get());
		if (aHouse->isClickable())
			aHouses.push_back(aHouse);
	}
	return aHouses;
}
}

// 0x44a490
SpecialEventPopup* pizzafrenzy::makeEventPopup(const std::string& theType)
{
	if (theType == TYPE_THIEF)
		return new ThiefPopup();
	if (theType == TYPE_GOSSIP)
		return new GossipPopup();
	if (theType == TYPE_MOVIE_STAR)
		return new MovieStarPopup();
	if (theType == TYPE_CRANK_CALLER)
		return new CrankCallerPopup();
	if (theType == TYPE_MONK)
		return new MonkPopup();
	if (theType == TYPE_CLOWN)
		return new ClownPopup();
	if (theType == TYPE_BANKER)
		return new BankerPopup();
	if (theType == TYPE_GRAFFITI_ARTIST)
		return new GraffitiArtistPopup();
	return nullptr;
}

// 0x44a890
void EventWaitState::update(engine::UpdateInfo& theInfo)
{
	mPopup->mWaitLeft = mPopup->mWaitLeft - theInfo.mSeconds;
	if (mPopup->mWaitLeft <= 0.0f)
		mPopup->act();
}

// 0x44a970
void EventFinishState::enter()
{
	mPopup->fadeBarOut();
}

// 0x44a990
void EventFinishState::update(engine::UpdateInfo& theInfo)
{
	if (mPopup->isBarGone())
		getMode()->removeEventPopup(mPopup);
}

// 0x44b100: the state starts empty; the timer is set by the event that speaks
SpecialEventPopup::SpecialEventPopup() : mWaitLeft(0)
{
}

// 0x44aea0: the references the popup holds are let go with its members
SpecialEventPopup::~SpecialEventPopup()
{
}

// 0x44ab20
void SpecialEventPopup::begin(BuildingTile* theKitchen, CustomerTile* theTile)
{
	SpecialEvent* anEvent = theTile->getOrder()->mEvent;
	mSound = anEvent->mSound;
	mPortrait = anEvent->mPortrait;
	mKitchen = theKitchen;
	mTile = theTile;
	mBar = new engine::FadeContainer();
	addChild(mBar);
	engine::RectangleItem* aShade = new engine::RectangleItem(BAR_LEFT, BAR_TOP, BAR_RIGHT, BAR_BOTTOM);
	aShade->setOpacity(BAR_SHADE);
	aShade->setColor(0.0f, 0.0f, 0.0f);
	mBar->addChild(aShade);
	mCaption = new engine::TextTyper();
	mCaption->setFont(std::string(BAR_FONT));
	mCaption->setWrapAndTiming(CAPTION_WRAP, CAPTION_RATE, CAPTION_SENTENCE_PAUSE);
	mCaption->setPosition(CAPTION_X, CAPTION_Y);
	mBar->addChild(mCaption);
	mPortraitNode = new engine::ImageNode(mPortrait);
	mPortraitNode->setPosition(PORTRAIT_X, PORTRAIT_Y);
	mPortraitNode->setVisible(false);
	mBar->addChild(mPortraitNode);
}

// 0x44b230
void SpecialEventPopup::show()
{
	mBar->fade(true, BAR_FADE_IN_SECONDS, BAR_FADE_TARGET, false);
	// the splat "portraitSplat" carries the portrait from the customer's house to the bar, and the popup goes on
	// when it has arrived
	engine::Point aHouse = *mTile->getPosition();
	engine::Splat* aSplat = PizzaFrenzy::instance()->mSplatFactory->create(PORTRAIT_SPLAT, aHouse.x, aHouse.y);
	aSplat->setChild(new engine::ImageNode(mPortrait));
	aSplat->setDestination(PORTRAIT_X, PORTRAIT_Y);
	aSplat->connectArrived(this, &SpecialEventPopup::onPortraitArrived);
	PizzaFrenzy::instance()->mSounds->play(mSound, SOUND_VOLUME, SOUND_PITCH);
	getMode()->deliver(mKitchen, mTile, true);
}

// 0x44a880
void SpecialEventPopup::updatePopup(engine::UpdateInfo& theInfo)
{
	mStates.update(theInfo);
}

// 0x44aa90
void SpecialEventPopup::onPortraitArrived()
{
	mPortraitNode->setVisible(true);
	speak();
	mStates.changeState(new EventWaitState(this));
}

// 0x44a930
void SpecialEventPopup::setCaption(const std::string& theText)
{
	mCaption->setTypedText(theText);
	mBar->setVisible(true);
	mCaption->start();
}

// 0x448a40's constructor, with the machine's change of state
void SpecialEventPopup::finish()
{
	mStates.changeState(new EventFinishState(this));
}

void SpecialEventPopup::fadeBarOut()
{
	mBar->fade(false, BAR_FADE_OUT_SECONDS, BAR_FADE_TARGET, false);
}

// 0x449a70
void ThiefPopup::begin(BuildingTile* theKitchen, CustomerTile* theTile)
{
	SpecialEventPopup::begin(theKitchen, theTile);
	mBillsLeft = THIEF_BILLS;
}

// 0x449aa0: the combo is broken
void ThiefPopup::speak()
{
	setCaption(GT(0x4fece4));
	mWaitLeft = 0.0f;
	getMode()->resetCombo();
}

// 0x449b50: while the player has money and there are bills to take, a bill comes from the tip jar to the portrait (the
// splat "StealBills") and a hundred is taken
void ThiefPopup::act()
{
	if (mBillsLeft > 0 && PizzaFrenzy::instance()->mGameStats->mTipMoney > 0)
	{
		mBillsLeft--;
		mWaitLeft = mBillsLeft < 1 ? LAST_BILL_SECONDS : THIEF_BILL_SECONDS;
		PizzaFrenzy* aGame = PizzaFrenzy::instance();
		const engine::Point& aTipJar = aGame->mHud->getTipJarCountPosition();
		engine::Splat* aBill = aGame->mSplatFactory->create(BILLS_SPLAT, aTipJar.x, aTipJar.y);
		aBill->setDestination(mPortraitNode->getPosition()->x, mPortraitNode->getPosition()->y);
		aGame->mGameStats->addTips(-THIEF_BILL_VALUE);
		return;
	}
	finish();
}

// 0x448bf0: every customer that takes a click stops being a chameleon; with each of them asking for the Gossip's
// topping already, nothing is left to say
void GossipPopup::show()
{
	mTopping = mTile->getOrder()->mTopping;
	bool aOtherTopping = false;
	std::vector<CustomerTile*> aHouses = getClickableHouses();
	for (size_t i = 0; i < aHouses.size(); i++)
	{
		PizzaPopup* aPopup = aHouses[i]->getOccupant();
		if (aPopup->getKind() == POPUP_CUSTOMER)
		{
			static_cast<CustomerPopup*>(aPopup)->setChameleon(false);
			if (aPopup->getOrder()->mTopping != mTopping)
				aOtherTopping = true;
		}
	}
	if (aOtherTopping)
	{
		SpecialEventPopup::show();
		return;
	}
	getMode()->deliver(mKitchen, mTile, true);
	getMode()->removeEventPopup(this);
}

// 0x448e00
void GossipPopup::speak()
{
	setCaption(formatText(GT(0x4febe8), mTopping->mDisplayName));
	mWaitLeft = 0.0f;
}

// 0x448cd0: the customers change to the topping, the first one with its sound
void GossipPopup::act()
{
	if (mTopping == nullptr)
	{
		finish();
		return;
	}
	bool aFirst = true;
	std::vector<CustomerTile*> aHouses = getClickableHouses();
	for (size_t i = 0; i < aHouses.size(); i++)
	{
		PizzaPopup* aPopup = aHouses[i]->getOccupant();
		if (aPopup->getKind() == POPUP_CUSTOMER)
		{
			CustomerPopup* aCustomer = static_cast<CustomerPopup*>(aPopup);
			Order* anOrder = aCustomer->getOrder();
			anOrder->mTopping = mTopping;
			aCustomer->startChameleon(anOrder, !aFirst);
			aFirst = false;
		}
	}
	getMode()->ordersChanged();
	mTopping = nullptr;
	mWaitLeft = TALK_SECONDS;
}

// 0x449440
void MovieStarPopup::begin(BuildingTile* theKitchen, CustomerTile* theTile)
{
	SpecialEventPopup::begin(theKitchen, theTile);
	mTipsLeft = STAR_TIPS;
}

// 0x449530
void MovieStarPopup::speak()
{
	setCaption(GT(0x4fec88));
	mWaitLeft = 0.0f;
}

// 0x449650: a hundred flies from the portrait to the tip jar (the splat "StealBills") and is added when it is there
void MovieStarPopup::act()
{
	if (mTipsLeft < 1)
	{
		finish();
		return;
	}
	mTipsLeft--;
	mWaitLeft = mTipsLeft < 1 ? LAST_BILL_SECONDS : THIEF_BILL_SECONDS;
	PizzaFrenzy* aGame = PizzaFrenzy::instance();
	engine::Point aPortrait = *mPortraitNode->getPosition();
	engine::Splat* aBill = aGame->mSplatFactory->create(BILLS_SPLAT, aPortrait.x, aPortrait.y);
	const engine::Point& aTipJar = aGame->mHud->getTipJarCountPosition();
	aBill->setDestination(aTipJar.x, aTipJar.y);
	aBill->connectArrived(this, &MovieStarPopup::addTip);
}

// 0x449470
void MovieStarPopup::addTip()
{
	PizzaFrenzy::instance()->mGameStats->addTips(STAR_TIP_VALUE);
}

// 0x448b40: the combo is broken
void CrankCallerPopup::speak()
{
	setCaption(GT(0x4febd0));
	getMode()->resetCombo();
	mWaitLeft = TALK_SECONDS;
}

// 0x448ad0
void CrankCallerPopup::act()
{
	finish();
}

// 0x449380
void MonkPopup::speak()
{
	setCaption(GT(0x4fec5c));
	getMode()->slowTime(MONK_SLOW_SECONDS, MONK_SLOW_FACTOR);
	mWaitLeft = TALK_SECONDS;
}

// 0x449310
void MonkPopup::act()
{
	finish();
}

// 0x4497d0: with no customer but the Clown's own that takes a click, nothing is left to scramble
void ClownPopup::show()
{
	std::vector<CustomerTile*> aHouses = getClickableHouses();
	for (size_t i = 0; i < aHouses.size(); i++)
	{
		if (aHouses[i] != mTile.get())
		{
			SpecialEventPopup::show();
			return;
		}
	}
	getMode()->deliver(mKitchen, mTile, true);
	getMode()->removeEventPopup(this);
}

// 0x4499e0
void ClownPopup::speak()
{
	setCaption(GT(0x4fecd0));
	mWaitLeft = 0.0f;
	mSwitched = false;
}

// 0x449850: each customer takes the topping after its own in the list of the kitchens' toppings (the first after the
// last), the first customer's change with its sound
void ClownPopup::act()
{
	if (mSwitched)
	{
		finish();
		return;
	}
	const std::vector<engine::Ref<Topping>>& aToppings = PizzaFrenzy::instance()->mCityMap->mKitchenToppings;
	bool aFirst = true;
	std::vector<CustomerTile*> aHouses = getClickableHouses();
	for (size_t i = 0; i < aHouses.size(); i++)
	{
		PizzaPopup* aPopup = aHouses[i]->getOccupant();
		if (aPopup->getKind() == POPUP_CUSTOMER)
		{
			CustomerPopup* aCustomer = static_cast<CustomerPopup*>(aPopup);
			Order* anOrder = aCustomer->getOrder();
			size_t aPlace = 0;
			while (aPlace < aToppings.size() && aToppings[aPlace].get() != anOrder->mTopping)
				aPlace++;
			// a topping that is not in the list takes the one after it in the original (past the list's end), and here
			// the first
			aPlace++;
			if (aPlace >= aToppings.size())
				aPlace = 0;
			anOrder->mTopping = aToppings[aPlace];
			aCustomer->startChameleon(anOrder, !aFirst);
			aFirst = false;
		}
	}
	getMode()->ordersChanged();
	mSwitched = true;
	mWaitLeft = TALK_SECONDS;
}

// 0x449d90: the helper picture goes off the screen with the popup (0x449cd0)
BankerPopup::~BankerPopup()
{
	if (mHelper != nullptr)
		mHelper->addOwnFlags(engine::NODE_REMOVE);
}

// 0x449ed0: the Banker's own picture stands on the game screen, hidden until it has a tip to take
void BankerPopup::speak()
{
	setCaption(GT(0x4fee50));
	mWaitLeft = BANKER_TIP_SECONDS;
	mTips = &PizzaFrenzy::instance()->mCityMap->mTips;
	mTipsLeft = BANKER_TIPS;
	mHelper = new engine::ImageNode(mTile->getOrder()->mCharacter->mImage);
	PizzaFrenzy::instance()->mGameScreen->addChild(mHelper);
	mHelper->setVisible(false);
}

// 0x449db0: the first tip that waits for a click is taken, the helper standing on it; after the last of the ten the
// helper goes
void BankerPopup::act()
{
	mTipsLeft--;
	std::vector<engine::Ref<TipPopup>>::iterator aTip = mTips->begin();
	while (aTip != mTips->end() && !(*aTip)->isWaiting())
		++aTip;
	if (aTip != mTips->end())
	{
		mHelper->setPosition(*(*aTip)->getPosition());
		mHelper->setVisible(true);
		(*aTip)->take();
	}
	mWaitLeft = BANKER_TIP_SECONDS;
	if (mTipsLeft == 0)
	{
		mHelper->addOwnFlags(engine::NODE_REMOVE);
		mHelper = nullptr;
		finish();
	}
}

// 0x44a410: the picture is let go with the member (0x44a430)
GraffitiArtistPopup::~GraffitiArtistPopup()
{
}

// 0x448eb0
void GraffitiArtistPopup::clearParent(engine::Node* theParent)
{
	if (mPaintSound != nullptr)
	{
		mPaintSound->stop();
		mPaintSound = nullptr;
	}
	engine::Container::clearParent(theParent);
}

// 0x448ee0: the combo is broken; the picture stands on the bar with none of it shown, and the sound of the painting is
// made
void GraffitiArtistPopup::speak()
{
	mWaitLeft = 0.0f;
	getMode()->resetCombo();
	mStage = GRAFFITI_START;
	engine::Image* anImage = engine::App::instance()->createImage(GRAFFITI_IMAGE, false);
	mPicture = new engine::ImageNode(anImage);
	mPicture->layout();
	mPicture->setClipped(true);
	mBar->addChild(mPicture);
	mPicture->setPosition(GRAFFITI_X, GRAFFITI_Y);
	mPicture->setClipRect(engine::Rect(0.0f, 0.0f, 0.0f, (float)anImage->getHeight()));
	mPaintSound = PizzaFrenzy::instance()->mSounds->getVoice(GRAFFITI_SOUND);
	if (mPaintSound != nullptr)
		mPaintSound->setLoop(true);
}

// 0x4490a0
void GraffitiArtistPopup::act()
{
	switch (mStage)
	{
	case GRAFFITI_START:
		if (mPaintSound != nullptr)
			mPaintSound->play();
		mWaitLeft = 0.0f;
		mStage = GRAFFITI_PAINT;
		break;
	case GRAFFITI_PAINT:
	{
		// the wait counts down from 0 through the seconds, so the part shown grows with the time passed: all of it
		// after two seconds
		float aPart = mWaitLeft * GRAFFITI_SHOW_RATE;
		float aShown = (float)mPicture->getImageWidth() * aPart;
		float aHeight = (float)mPicture->getImageHeight();
		mPicture->setClipRect(engine::Rect(0.0f, 0.0f, (float)(int)aShown, aHeight));
		if ((float)mPicture->getImageWidth() < aShown)
		{
			if (mPaintSound != nullptr)
				mPaintSound->stop();
			mWaitLeft = 0.0f;
			mStage = GRAFFITI_STEAL;
		}
		break;
	}
	case GRAFFITI_STEAL:
	{
		// the splat "StealSatisfaction" flies from the happiness to the portrait
		PizzaFrenzy* aGame = PizzaFrenzy::instance();
		engine::Point aHappiness = aGame->mHud->getHappinessPosition();
		engine::Splat* aSatisfaction = aGame->mSplatFactory->create(SATISFACTION_SPLAT, aHappiness.x, aHappiness.y);
		aSatisfaction->setDestination(mPortraitNode->getPosition()->x, mPortraitNode->getPosition()->y);
		getMode()->addHappiness(GRAFFITI_HAPPINESS);
		mStage = GRAFFITI_DONE;
		mWaitLeft = GRAFFITI_LAST_SECONDS;
		break;
	}
	case GRAFFITI_DONE:
		mBar->removeChild(mPicture);
		finish();
		break;
	}
}
