#include "BonusGame.h"
#include "App.h"
#include "BonusStates.h"
#include "FadeContainer.h"
#include "GameText.h"
#include "ModeStates.h"
#include "PizzaFrenzy.h"
#include "ScreenNodes.h"

#include <cmath>

using namespace pizzafrenzy;

namespace
{
const char* const NODE_TITLES = "titles";
const char* const NODE_USER_PIZZA = "userPizza";
const char* const NODE_USER_TOPPINGS = "userToppings";
const char* const NODE_TARGET_PIZZA = "targetPizza";
const char* const NODE_TARGET_TOPPINGS = "targetToppings";
const char* const NODE_TIMER = "timer";
const char* const NODE_PIZZA_COUNT = "pizzaCount";
const char* const NODE_LORENZO_TEXT = "LorenzoText";
const char* const NODE_LORENZO = "Lorenzo";
const char* const NODE_TOPPING_BUTTONS = "toppingButtons";
const char* const NODE_CAPTION = "caption";
const char* const NODE_TIMER_GROUP = "timerGroup";

const char* const SOUND_PLACE = "order_click1";
const char* const SOUND_INVALID_CLICK = "order_invalidClick";
const char* const SOUND_MATCH = "bonus_toppingMatch";
const char* const SOUND_MISS = "bonus_toppingMiss";
const char* const SPLAT_MATCH = "matchToppingSplat";
const char* const SPLAT_MISS = "missToppingSplat";

// Lorenzo's text is typed at 360 pixels wide and 120 letters a second
const float LORENZO_WRAP = 360.0f;
const float LORENZO_RATE = 120.0f;

// The pizzas of a round, as a set of five is ordered
const int PIZZAS_ORDERED = 5;

// The places the pizzas rest at (the layout's, which the round does not read: the nodes are left away from them when
// a round ends)
const float USER_PIZZA_X = 205.0f;
const float TARGET_PIZZA_X = 605.0f;
const float PIZZA_Y = 280.0f;

// The motions' seconds, and how far the pizzas come from and the timer group starts
const float MOTION_SECONDS = 0.5f;
const float MOTION_TOP = 1.0f;
const float PIZZA_SLIDE = 400.0f;
const float TIMER_GROUP_X = 410.0f;
const float TIMER_GROUP_START_Y = -160.0f;
const float TIMER_GROUP_Y = 90.0f;
// A motion's keys: the first key's curve, and the last key's, which slows into it
const float KEY_END_SLOW_TIME = 0.3f;

// The cursor and what a click does, by the pixels from the player's pizza's centre
const float CURSOR_NEAR = 140.0f;
const float CURSOR_FAR_SCALE = 0.5f;
const float CLICK_FAR = 200.0f;
// A topping is taken back when the right click is within this of it
const float TAKE_BACK_RANGE = 15.0f;

// What a topping is worth by how far it lies from its target
const float DISTANCE_NONE = 1000.0f;
const float DISTANCE_PERFECT = 15.0f;
const float DISTANCE_GOOD = 30.0f;
const float DISTANCE_OKAY = 60.0f;

// The buttons' places, and the share of the dimmed ones
const float BUTTON_SPACING = 80.0f;

double distanceOf(float theX, float theY)
{
	return std::sqrt((double)theX * theX + (double)theY * theY);
}

void playSound(const std::string& theName, float theVolume = 1.0f, float thePitch = 1.0f)
{
	PizzaFrenzy::instance()->mSounds->play(theName, theVolume, thePitch);
}

// A key of the pizzas' and the timer's way: the first comes in at once, the last slows into its place
void addPositionKey(engine::PositionInterpolator& theMotion, float theTime, const engine::Point& thePlace, bool theLast)
{
	engine::PositionKey aKey;
	aKey.mTime = theTime;
	aKey.mValue = thePlace;
	aKey.mInTime = theLast ? KEY_END_SLOW_TIME : 0.0f;
	aKey.mInSpeed = theLast ? 0.0f : 1.0f;
	aKey.mOutTime = theLast ? KEY_END_SLOW_TIME : 0.0f;
	aKey.mOutSpeed = 1.0f;
	theMotion.addKey(aKey);
}

engine::Container* getGroup(engine::ScreenLayout* theScreen, const char* theName)
{
	return static_cast<engine::Container*>(theScreen->getNode(theName));
}

engine::TextItem* getText(engine::ScreenLayout* theScreen, const char* theName)
{
	return static_cast<engine::TextItem*>(theScreen->getNode(theName));
}
}

// 0x45f830
BonusGame::BonusGame() :
	mNextPizza(0), mTimeLeft(0), mPizzasLeft(0), mPlaying(false)
{
}

// 0x45fa80
BonusGame::~BonusGame()
{
}

// 0x45bd20
void BonusGame::update(engine::UpdateInfo& theInfo)
{
	mTimerRamp.update(theInfo.mSeconds);
	mTimerMotion.modify(mTimerRamp.getProgress(), mTimerGroup);
	mStats->mElapsedTime = mStats->mElapsedTime + theInfo.mSeconds;
	mStates.update(theInfo);
}

// 0x45bcc0
void BonusGame::action(const std::string& theAction)
{
	mStates.action(theAction);
}

// 0x45c700
void BonusGame::setLevel(Level* theLevel)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mTimeLeft = (float)theLevel->mNumOrders;
	mPizzasLeft = (int)theLevel->mPizzas.size();
	mTimerText->setTime((int)mTimeLeft, true);
	mPizzaCountText->setNumber(mPizzasLeft, "");
	mLevel = theLevel;
	mNextPizza = 0;
	mLastTopping = nullptr;
	mMusicClip = anApp->mTileManifest->getMusic(theLevel->mMusic);
	if (mMusicClip != nullptr)
	{
		mMusicVolume = mMusicClip->getVolume();
		anApp->mMusicPlayer->play(mMusicClip);
	}
}

// 0x45bd90
void BonusGame::clearLevel()
{
	mUserToppingGroup->removeAllChildren();
	mTargetToppingGroup->removeAllChildren();
}

// 0x45ea90
void BonusGame::startPlay()
{
	mStates.changeState(new BonusNextPizzaState(this));
	mTimerRamp.start();
}

// 0x460080
void BonusGame::finishLevel()
{
	PizzaFrenzy::instance()->mMusicPlayer->stop();
	mMusicClip = nullptr;
	selectTopping(nullptr);
	mCursorTopping = nullptr;
}

// 0x45ffa0
void BonusGame::setPaused(bool thePaused)
{
	if (thePaused)
	{
		mLastTopping = mSelectedTopping;
		selectTopping(nullptr);
	}
	else
		selectTopping(mLastTopping);
}

// 0x45e140
void BonusGame::setStats(GameScreen* theScreen, GameStats* theStats)
{
	mScreen = theScreen;
	mTitles = static_cast<engine::FadeContainer*>(mScreen->getNode(NODE_TITLES));
	mUserPizza = getGroup(mScreen, NODE_USER_PIZZA);
	mUserPizzaPlace = engine::Point(USER_PIZZA_X, PIZZA_Y);
	mUserToppingGroup = getGroup(mScreen, NODE_USER_TOPPINGS);
	mTargetPizza = getGroup(mScreen, NODE_TARGET_PIZZA);
	mTargetPizzaPlace = engine::Point(TARGET_PIZZA_X, PIZZA_Y);
	mTargetToppingGroup = getGroup(mScreen, NODE_TARGET_TOPPINGS);
	mTimerText = getText(mScreen, NODE_TIMER);
	mPizzaCountText = getText(mScreen, NODE_PIZZA_COUNT);

	// Lorenzo's text is a typer in the place of the layout's text, which is emptied; his group is hidden (nothing
	// types the text)
	engine::TextItem* aLorenzoText = getText(mScreen, NODE_LORENZO_TEXT);
	mLorenzoText = new engine::TextTyper();
	mLorenzoText->setPosition(*aLorenzoText->getPosition());
	mLorenzoText->setFont(aLorenzoText->mFont);
	mLorenzoText->setWrapAndTiming(LORENZO_WRAP, LORENZO_RATE, 0);
	mLorenzoText->setColor(*aLorenzoText->getColor());
	mLorenzoText->setColorMode(aLorenzoText->mColorMode);
	mLorenzoText->setLineOffsets(0, 0);
	aLorenzoText->setText("");
	mLorenzo = getGroup(mScreen, NODE_LORENZO);
	mLorenzo->addChild(mLorenzoText);
	mLorenzo->setOpacity(0);
	mLorenzo->setVisible(false);

	mToppingButtons = getGroup(mScreen, NODE_TOPPING_BUTTONS);
	mCaption = getText(mScreen, NODE_CAPTION);
	mTimerGroup = getGroup(mScreen, NODE_TIMER_GROUP);
	mStats = theStats;
	mStats->resetLevelCounters();
	mStats->mPizzasOrdered = PIZZAS_ORDERED;
	mMusicClip = nullptr;
	mCursorTopping = new engine::ImageNode();
	setUpMotions();
	mStates.changeState(new ModeIntroState(this));
}

// 0x45be50
void BonusGame::setUpMotions()
{
	mUserRamp.set(MOTION_SECONDS, MOTION_TOP);
	mUserRamp.reset();
	mTargetRamp.set(MOTION_SECONDS, MOTION_TOP);
	mTargetRamp.reset();
	mTimerRamp.set(MOTION_SECONDS, MOTION_TOP);
	mTimerRamp.reset();
	mTimerRamp.stop();

	addPositionKey(mUserMotion, 0, engine::Point(mUserPizzaPlace.x - PIZZA_SLIDE, mUserPizzaPlace.y), false);
	addPositionKey(mUserMotion, 1.0f, mUserPizzaPlace, true);
	addPositionKey(mTargetMotion, 0, engine::Point(mTargetPizzaPlace.x + PIZZA_SLIDE, mTargetPizzaPlace.y), false);
	addPositionKey(mTargetMotion, 1.0f, mTargetPizzaPlace, true);

	engine::ScalarKey aScaleKey;
	aScaleKey.mTime = 0;
	aScaleKey.mValue = 0;
	aScaleKey.mInTime = 0;
	aScaleKey.mInSpeed = 1.0f;
	aScaleKey.mOutTime = 0;
	aScaleKey.mOutSpeed = 1.0f;
	mButtonsMotion.addKey(aScaleKey);
	aScaleKey.mTime = 1.0f;
	aScaleKey.mValue = 1.0f;
	aScaleKey.mInTime = KEY_END_SLOW_TIME;
	aScaleKey.mInSpeed = 0;
	aScaleKey.mOutTime = KEY_END_SLOW_TIME;
	aScaleKey.mOutSpeed = 1.0f;
	mButtonsMotion.addKey(aScaleKey);

	addPositionKey(mTimerMotion, 0, engine::Point(TIMER_GROUP_X, TIMER_GROUP_START_Y), false);
	addPositionKey(mTimerMotion, 1.0f, engine::Point(TIMER_GROUP_X, TIMER_GROUP_Y), true);

	mUserPizza->setPosition(mUserPizzaPlace.x - PIZZA_SLIDE, mUserPizzaPlace.y);
	mTargetPizza->setPosition(mTargetPizzaPlace.x + PIZZA_SLIDE, mTargetPizzaPlace.y);
	mTimerGroup->setPosition(TIMER_GROUP_X, TIMER_GROUP_START_Y);
	mToppingButtons->setScale(0);
}

// 0x4604e0
void BonusGame::beginPizza(Pizza* thePizza)
{
	makeTargetToppings(thePizza);
	makeButtons(thePizza);
	mCaption->setText(formatText(GT(0x501188), thePizza->mName));
	selectTopping(nullptr);
}

// 0x45f3a0
void BonusGame::makeTargetToppings(Pizza* thePizza)
{
	mTargetToppings.clear();
	mTargetToppingGroup->removeAllChildren();
	for (size_t i = 0; i < thePizza->mToppings.size(); i++)
	{
		const PizzaEntry& anEntry = thePizza->mToppings[i];
		engine::Ref<PizzaTopping> aTopping = new PizzaTopping();
		aTopping->setTopping(anEntry.mTopping);
		aTopping->setEnabled(false);
		mTargetToppings.push_back(aTopping);
		mTargetToppingGroup->addChild(aTopping);
		aTopping->setPosition(anEntry.mPlace.x, anEntry.mPlace.y);
		aTopping->setVisible(false);
	}
}

// 0x4600e0
void BonusGame::makeButtons(Pizza* thePizza)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mToppingButtonMap.clear();
	mToppingButtons->removeAllChildren();
	mUserToppings.clear();
	mUserToppingGroup->removeAllChildren();
	std::map<std::string, int> aCounts;
	for (size_t i = 0; i < thePizza->mToppings.size(); i++)
		aCounts[thePizza->mToppings[i].mTopping->mName]++;
	float aX = 0;
	for (std::map<std::string, int>::iterator anIt = aCounts.begin(); anIt != aCounts.end(); ++anIt)
	{
		Topping* aTopping = anApp->mTileManifest->getTopping(anIt->first);
		engine::Ref<ToppingButton> aButton = new ToppingButton(aTopping, anIt->second, mScreen);
		aButton->setScale(1.0f);
		aButton->setPosition(aX, 0);
		mToppingButtonMap[aTopping] = aButton;
		mToppingButtons->addChild(aButton);
		aX += BUTTON_SPACING;
	}
}

// 0x45fd50
void BonusGame::selectTopping(Topping* theTopping)
{
	engine::App* anApp = engine::App::instance();
	if (mSelectedTopping != nullptr)
	{
		anApp->mMouseMoveSignal.disconnect(this);
		anApp->mMouseDownSignal.disconnect(this);
		mScreen->removeChild(mCursorTopping);
	}
	mSelectedTopping = theTopping;
	if (theTopping != nullptr)
	{
		mCursorTopping->setImage(theTopping->mImage);
		anApp->mMouseMoveSignal.connect(this, &BonusGame::onMouseMove);
		anApp->mMouseDownSignal.connect(this, &BonusGame::onMouseDown);
		mScreen->addChild(mCursorTopping);
	}
	else if (mCursorTopping != nullptr)
		mCursorTopping->setImage(nullptr);
}

// 0x45bdb0
void BonusGame::onMouseMove(engine::Point& thePoint)
{
	engine::Point aGroup = getAbsPosition(mUserToppingGroup);
	float aDeltaX = (float)((int)thePoint.x - (int)aGroup.x);
	float aDeltaY = (float)((int)thePoint.y - (int)aGroup.y);
	mCursorTopping->setScale(distanceOf(aDeltaX, aDeltaY) < CURSOR_NEAR ? 1.0f : CURSOR_FAR_SCALE);
	mCursorTopping->setPosition(thePoint.x, thePoint.y);
}

// 0x45faa0
void BonusGame::onMouseDown(engine::Point& thePoint)
{
	engine::Point aGroup = getAbsPosition(mUserToppingGroup);
	float aDeltaX = (float)((int)thePoint.x - (int)aGroup.x);
	float aDeltaY = (float)((int)thePoint.y - (int)aGroup.y);
	double aLength = distanceOf(aDeltaX, aDeltaY);
	// the topping picked at the click: the last topping of a pizza ends it, which lets go of the pick
	engine::Ref<Topping> aPicked = mSelectedTopping;
	if (aLength < CURSOR_NEAR && aPicked != nullptr && mToppingButtonMap[aPicked]->getNumber() > 0 && mPlaying)
	{
		placeTopping(engine::Point(aDeltaX, aDeltaY), aPicked);
		if (mToppingButtonMap[aPicked]->getNumber() == 0)
			selectTopping(nullptr);
		playSound(SOUND_PLACE);
	}
	else if (aLength < CLICK_FAR && mSelectedTopping != nullptr && mPlaying)
		playSound(SOUND_INVALID_CLICK);
}

// 0x45f550
void BonusGame::placeTopping(const engine::Point& thePlace, Topping* theTopping)
{
	engine::Ref<PizzaTopping> aPlaced = new PizzaTopping();
	aPlaced->setTopping(theTopping);
	aPlaced->setEnabled(false);
	mUserToppings.push_back(aPlaced);
	mUserToppingGroup->addChild(aPlaced);
	aPlaced->setPosition(thePlace.x, thePlace.y);
	ToppingButton* aButton = mToppingButtonMap[theTopping];
	aButton->setNumber(aButton->getNumber() - 1);
	updateButton(theTopping);
	if (allToppingsPlaced())
	{
		mStats->mPizzasDelivered++;
		endPizza();
	}
}

// 0x45c6b0
bool BonusGame::allToppingsPlaced()
{
	for (std::map<Topping*, engine::Ref<ToppingButton>, ToppingOrder>::iterator anIt = mToppingButtonMap.begin();
	     anIt != mToppingButtonMap.end(); ++anIt)
		if (anIt->second->getNumber() != 0)
			return false;
	return true;
}

// 0x45cf60
void BonusGame::updateButton(Topping* theTopping)
{
	ToppingButton* aButton = mToppingButtonMap[theTopping];
	aButton->setCaption(std::to_string(aButton->getNumber()));
	if (aButton->getNumber() == 0)
		aButton->showDimmed();
	else
		aButton->showPlain();
}

// 0x460340
void BonusGame::onRightClick(engine::Point& thePoint)
{
	engine::Point aGroup = getAbsPosition(mUserToppingGroup);
	removeTopping(engine::Point((float)((int)thePoint.x - (int)aGroup.x), (float)((int)thePoint.y - (int)aGroup.y)));
}

// 0x45fe70
void BonusGame::removeTopping(const engine::Point& thePlace)
{
	for (size_t i = 0; i < mUserToppings.size(); i++)
	{
		PizzaTopping* aTopping = mUserToppings[i];
		engine::Point* aPosition = aTopping->getPosition();
		if (distanceOf(aPosition->x - thePlace.x, aPosition->y - thePlace.y) < TAKE_BACK_RANGE)
		{
			ToppingButton* aButton = mToppingButtonMap[aTopping->getTopping()];
			aButton->setNumber(aButton->getNumber() + 1);
			selectTopping(aTopping->getTopping());
			updateButton(aTopping->getTopping());
			mUserToppingGroup->removeChild(aTopping);
			mUserToppings.erase(mUserToppings.begin() + i);
			return;
		}
	}
}

// 0x45c570
void BonusGame::endPizza()
{
	mStates.changeState(new BonusScoreState(this));
}

// 0x45c8c0
bool BonusGame::scoreTopping(size_t theIndex)
{
	PizzaTopping* aTarget = mTargetToppings[theIndex];
	for (size_t i = 0; i < mTargetToppings.size(); i++)
		mTargetToppings[i]->setMarked(false);
	aTarget->setMarked(true);

	// the nearest topping of the player's of the same kind; the ones that were matched by the toppings before are
	// let go of the list (they stay on the pizza)
	float aBest = DISTANCE_NONE;
	PizzaTopping* aBestTopping = nullptr;
	engine::Point* aTargetPosition = aTarget->getPosition();
	for (size_t i = 0; i < mUserToppings.size();)
	{
		PizzaTopping* aUser = mUserToppings[i];
		if (aUser->isMarked())
		{
			aUser->setMarked(false);
			mUserToppings.erase(mUserToppings.begin() + i);
			continue;
		}
		engine::Point* aUserPosition = aUser->getPosition();
		float aDistance =
			(float)distanceOf(aTargetPosition->x - aUserPosition->x, aTargetPosition->y - aUserPosition->y);
		if (aDistance < aBest && aTarget->getTopping() == aUser->getTopping())
		{
			aBest = aDistance;
			aBestTopping = aUser;
		}
		i++;
	}

	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (aBest < DISTANCE_OKAY)
	{
		aBestTopping->setMarked(true);
		engine::Point aAt = getAbsPosition(aBestTopping);
		engine::Splat* aSplat = anApp->mSplatFactory->create(SPLAT_MATCH, aAt.x, aAt.y);
		if (aBest < DISTANCE_PERFECT)
		{
			aSplat->setText(GT(0x500ff4));
			mStats->mPerfectToppings++;
		}
		else if (aBest < DISTANCE_GOOD)
		{
			aSplat->setText(GT(0x500fd8));
			mStats->mGoodToppings++;
		}
		else
		{
			aSplat->setText(GT(0x500fd0));
			mStats->mOkayToppings++;
		}
		playSound(SOUND_MATCH);
	}
	else
	{
		mStats->mMissedToppings++;
		engine::Point aAt = getAbsPosition(aTarget);
		engine::Splat* aSplat = anApp->mSplatFactory->create(SPLAT_MISS, aAt.x - PIZZA_SLIDE, aAt.y);
		aSplat->setText(GT(0x500fb4));
		playSound(SOUND_MISS);
	}
	return theIndex + 1 < mTargetToppings.size();
}

// port: slot 0x10's answer, the node's place on the screen: its own place and its parents', added up when asked
engine::Point BonusGame::getAbsPosition(engine::Node* theNode)
{
	engine::Point aPoint = *theNode->getPosition();
	for (engine::Node* aParent = theNode->mParent; aParent != nullptr; aParent = aParent->mParent)
	{
		aPoint.x += aParent->getPosition()->x;
		aPoint.y += aParent->getPosition()->y;
	}
	return aPoint;
}

// port: the button of the topping, none for a topping the pizza does not have
ToppingButton* BonusGame::getButton(Topping* theTopping)
{
	std::map<Topping*, engine::Ref<ToppingButton>, ToppingOrder>::iterator anIt = mToppingButtonMap.find(theTopping);
	return anIt == mToppingButtonMap.end() ? nullptr : anIt->second.get();
}

// port: the state the round is in, for the test commands
BonusGame::Phase BonusGame::getPhase()
{
	engine::State* aState = mStates.getState();
	if (dynamic_cast<BonusPlayState*>(aState) != nullptr)
		return PHASE_PLAY;
	if (dynamic_cast<BonusNextPizzaState*>(aState) != nullptr)
		return PHASE_ARRIVE;
	if (dynamic_cast<BonusScoreState*>(aState) != nullptr)
		return PHASE_SCORE;
	if (dynamic_cast<BonusEndState*>(aState) != nullptr)
		return PHASE_END;
	return PHASE_INTRO;
}
