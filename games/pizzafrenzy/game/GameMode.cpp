#include "GameMode.h"
#include "Van.h"
#include "App.h"
#include "CustomerPopup.h"
#include "ModeStates.h"
#include "PizzaFrenzy.h"
#include "Random.h"
#include "ScreenNodes.h"
#include "TextItem.h"

using namespace pizzafrenzy;

namespace
{
// While the time is slowed the music is silent (0x4505f0)
const float SLOWED_MUSIC_VOLUME = 0.0f;
// the layer of a vehicle that is drawn above the others (the blimp's)
const int AIR_LAYER = 4;



// the texts of the level's titles, by their numbers in the string table
const int STRING_DAY = 0xe0;
const int STRING_CITY_NAME = 0xe1;
const int STRING_LEVEL_NAME = 0xe2;

// the nodes of the game screen's layout that the titles fill
const char* const NODE_EXTRA = "extra";
const char* const NODE_CITY_NAME = "text1";
const char* const NODE_LEVEL_NAME = "text2";
const char* const NODE_DAY = "text3";

const char* const SOUND_HANGUP = "popup_hangup";
const char* const SOUND_PICK = "order_click1";
const char* const SOUND_SEND = "order_click2";
const char* const SOUND_INVALID_CLICK = "order_invalidClick";
const char* const SOUND_COUPON = "tip_couponConvert";
const char* const SOUND_UPGRADE = "bonus_upgrade";
const char* const SOUND_TOPPING_COMBO = "bonus_toppingCombo";
const char* const SOUND_SPEEDY = "bonus_speedyBlast";
const char* const SOUND_FRENZY = "frenzy%d";
const char* const ACTION_CUSTOMER_HANGUP = "customerHangup";
const char* const ACTION_FALSE_ARREST = "falseArrest";
const char* const ACTION_SPECIAL_HANGUP = "specialHangup";
const char* const MUSIC_LOW_HAPPINESS = "music_lowSatWarning";

const float DELIVERY_INTERVAL = 0.4f;
const float WAVE_TIMER_START = 1.0f;
const float WAVE_TIMER_CAP = 1.5f;
const float SPEED_BONUS_PER_CUSTOMER = 1.5f;
const int MAX_CONCENTRATION_WAVE = 8;
const int HANGUP_HAPPINESS = -10;
const int LOW_HAPPINESS = 20;
const int MAX_HAPPINESS = 100;

// what a delivery is worth: the combo's multiplier grows by a twentieth for each pizza of the same topping in a
// row after the first; every fifth pizza of the combo is a frenzy, which pays for each of its fives; ten deliveries
// in a row add happiness
const float COMBO_STEP = 0.05f;
const int FRENZY_COMBO = 5;
const int FRENZY_MAX_SOUND = 5;
const int FRENZY_BONUS = 300;
const int STREAK_LENGTH = 10;
const int STREAK_HAPPINESS = 10;
const int SPEEDY_HAPPINESS = 10;
// a tip is worth more the sooner the delivery follows the one before it
const float TIP_FAST_SECONDS = 1.5f;
const float TIP_MEDIUM_SECONDS = 3.0f;
const float TIP_FAST_MULTIPLIER = 3.0f;
const float TIP_MEDIUM_MULTIPLIER = 2.0f;
const float TIP_SLOW_MULTIPLIER = 1.0f;
// a coupon costs a hundred more each time it is used
const int COUPON_PRICE_STEP = 100;
// where a tip comes up: the middle of the cell the vehicle stopped on
const float TIP_CELL_SHARE = -0.5f;

engine::Point toPoint(const engine::FloatPair& thePair)
{
	return engine::Point(thePair.mFirst, thePair.mSecond);
}

void playSound(const std::string& theName, float theVolume = 1.0f, float thePitch = 1.0f)
{
	PizzaFrenzy::instance()->mSounds->play(theName, theVolume, thePitch);
}
}

// 0x459510 (speed), 0x45b3e0 (concentration): the counts start empty, the factor of the slowed time one
GameMode::GameMode() :
	mMusicVolume(0),
	mLevel(nullptr),
	mWaveIndex(0),
	mWavesDone(false),
	mSlowFactor(1.0f),
	mSlowTime(0),
	mSlowSound(nullptr),
	mWaveTimer(WAVE_TIMER_START),
	mSpawnTimer(0),
	mSecondsSinceDelivery(0),
	mSpeedBonusTime(0),
	mWaveCustomersLeft(0),
	mOrdersBegun(0),
	mDeliveryStreak(0),
	mRightClickHandled(false),
	mComboTopping(nullptr),
	mDeliveryTimer(0)
{
	mSpawnDelay.mFirst = 0;
	mSpawnDelay.mSecond = 0;
	mWaitTime.mFirst = 0;
	mWaitTime.mSecond = 0;
}

// 0x45a2e0 (speed), 0x4596d0
GameMode::~GameMode()
{
	mStates.changeState(nullptr);
}

// 0x459330's connection of the right button (0x455cb0 calls slot 0x48)
void GameMode::connectRightButton()
{
	engine::App::instance()->mRightDownSignal.connect(this, &GameMode::onRightDown);
}

// 0x4569c0
void GameMode::update(engine::UpdateInfo& theInfo)
{
	updatePopups(theInfo);
	mHud->updateHud(theInfo);
	mStats->mElapsedTime = theInfo.mSeconds + mStats->mElapsedTime;
	if (0.0f < mSlowTime)
	{
		mSlowTime = mSlowTime - theInfo.mSeconds;
		if (mSlowSound != nullptr && !mSlowSound->isPlaying())
		{
			mSlowSound->play();
			PizzaFrenzy::instance()->mMusicPlayer->setVolume(SLOWED_MUSIC_VOLUME);
		}
		if (0.0f < mSlowTime)
			theInfo.mSeconds = mSlowFactor * theInfo.mSeconds;
		else
		{
			mSlowTime = 0;
			mSlowFactor = 1.0f;
			if (mSlowSound != nullptr)
				mSlowSound->stop();
			PizzaFrenzy::instance()->mMusicPlayer->setVolume(mMusicVolume);
		}
	}
	mScreen->updateScreen(theInfo);
	mStates.update(theInfo);
}

// 0x459cb0
void GameMode::setLevel(Level* theLevel)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mLevel = theLevel;
	reset();
	theLevel->setUpKitchens(mMap);
	mMap->collectKitchenToppings();
	theLevel->setUpBoats(mMap);
	mScreen->drawCity();
	mMusicClip = anApp->mTileManifest->getMusic(theLevel->mMusic);
	if (mMusicClip != nullptr)
	{
		mMusicVolume = mMusicClip->getVolume();
		anApp->mMusicPlayer->play(mMusicClip);
	}
	makeEvents();
	mComboTopping = nullptr;
	mStats->mComboLength = 0;
	mMap->showCombo(nullptr, 0, 0);
	// the speed bonus banner (0x44fb70, the game screen's +0x204) starts over here: left out, see GameScreen.h
	beginWaves();
	mStates.changeState(new ModeIntroState(this));
}

// 0x459150
void GameMode::clearLevel()
{
	clearEvents();
	mMap->clearBoatSpawners();
	mLevel = nullptr;
}

// 0x456c20
void GameMode::startPlay()
{
	mHud->showCoupons();
	mStates.changeState(new ModePlayState(this));
}

// 0x456300
void GameMode::setPaused(bool thePaused)
{
	clearPicks();
	mHud->pauseTipJarSound(thePaused);
}

// 0x455fe0
void GameMode::updateMap(engine::UpdateInfo& theInfo)
{
	mMap->update(theInfo);
}

// 0x4569c0's first call: the popup of the special event that shows has its frame
void GameMode::updatePopups(engine::UpdateInfo& theInfo)
{
	if (!mEventPopups.empty())
		mEventPopups.front()->updatePopup(theInfo);
}

// 0x417cb0
void GameMode::makeEvents()
{
	for (size_t i = 0; i < mLevel->mEvents.size(); i++)
		mEventTimers.push_back(new EventTimer(&mLevel->mEvents[i]));
}

// 0x459150's release of the vector at +0x58
void GameMode::clearEvents()
{
	mEventTimers.clear();
}

// 0x457290: the level's counters and lists start over, and the waves go from the level's first
void GameMode::reset()
{
	mStats->startLevel(mLevel);
	mSpawnTimer = 0;
	mWaveTimer = WAVE_TIMER_START;
	mDeliveryTimer = 0;
	mDeliveries.clear();
	mWaveOrders.clear();
	mWavesDone = false;
	mCoupon = nullptr;
	mComboTopping = nullptr;
	for (size_t i = 0; i < mEventTimers.size(); i++)
		mEventTimers[i]->reset();
	mWaveIndex = 0;
	mActiveOrders.clear();
	mWaveCustomersLeft = 0;
	mDeliveryStreak = 0;
	mOrdersBegun = 0;
	mSecondsSinceDelivery = 0;
	mHud->resetHud();
	mComboTopping = nullptr;
	mStats->mComboLength = 0;
	mMap->showCombo(nullptr, 0, 0);
	mRightClickHandled = false;
	mSlowTime = 0;
	mSlowFactor = 1.0f;
}

// 0x4580f0: the node "extra" is cleared and the names of the city, the level and the day are written
void GameMode::beginWaves()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	City* aCity = anApp->getCurrentCity();
	engine::ScreenLayout* aLayout = mScreen->mGameLayout;
	static_cast<engine::Container*>(aLayout->getNode(NODE_EXTRA))->removeAllChildren();
	if (engine::TextItem* aCityName = static_cast<engine::TextItem*>(aLayout->getNode(NODE_CITY_NAME)))
		aCityName->setText(formatMessage(engine::App::instance()->loadString(STRING_CITY_NAME), aCity->getName()));
	if (engine::TextItem* aLevelName = static_cast<engine::TextItem*>(aLayout->getNode(NODE_LEVEL_NAME)))
		aLevelName->setText(formatMessage(engine::App::instance()->loadString(STRING_LEVEL_NAME), mLevel->mName));
	if (engine::TextItem* aDay = static_cast<engine::TextItem*>(aLayout->getNode(NODE_DAY)))
		aDay->setText(formatMessage(engine::App::instance()->loadString(STRING_DAY), std::to_string(anApp->mDay)));
}

// 0x457490
void GameMode::updateWaves(engine::UpdateInfo& theInfo)
{
	if (!mDeliveries.empty())
	{
		mDeliveryTimer -= theInfo.mSeconds;
		if (mDeliveryTimer <= 0.0f)
		{
			QueuedDelivery aDelivery = mDeliveries.front();
			CustomerTile* aTile = static_cast<CustomerTile*>(aDelivery.mTile.get());
			if (aTile->getOrder()->mEvent == nullptr)
				deliver(aDelivery.mKitchen, aTile, true);
			else
				startSpecialEvent(aDelivery.mKitchen, aTile);
			if (!mDeliveries.empty())
				mDeliveries.pop_front();
			if (!mDeliveries.empty())
				mDeliveryTimer = DELIVERY_INTERVAL;
		}
	}
	if (mWavesDone)
		return;
	mSecondsSinceDelivery += theInfo.mSeconds;
	mWaveTimer -= theInfo.mSeconds;
	mSpeedBonusTime -= theInfo.mSeconds;
	if (mWaveTimer < 0.0f && mWaveOrders.empty() && !isPoliceBusy())
		startNextWave();
	mSpawnTimer -= theInfo.mSeconds;
	while (!mWaveOrders.empty() && mSpawnTimer <= 0.0f)
	{
		engine::Ref<Order> anOrder = mWaveOrders.back();
		if (!placeOrder(anOrder))
			break;
		mWaveOrders.pop_back();
		mSpawnTimer = engine::randomFloat(toPoint(mSpawnDelay));
		mWaveTimer = mLevel->mWaveGap + anOrder->mWaitTime;
	}
	if (mLevel->mNumOrders <= mOrdersBegun && mWaveOrders.empty())
		mWavesDone = true;
}

// 0x45a9a0: the wave's customers are drawn, as many as the level has orders left (and in the concentration mode at
// most eight), the events' orders count among them, and the orders are shuffled
void GameMode::startNextWave()
{
	if (mOrdersBegun >= mLevel->mNumOrders)
	{
		mWavesDone = true;
		return;
	}
	mWaveTimer = mLevel->mWaveGap;
	mSecondsSinceDelivery = 0;
	mStats->mWavesStarted++;
	LevelWave& aWave = mLevel->mWaves[mWaveIndex];
	mWaitTime = aWave.mWaitTime;
	mSpawnDelay = aWave.mSpawnDelay;
	int aCount = engine::randomRange(aWave.mSize);
	if (PizzaFrenzy::instance()->mGameMode == PizzaFrenzy::MODE_CONCENTRATION && aCount > MAX_CONCENTRATION_WAVE)
		aCount = MAX_CONCENTRATION_WAVE;
	if (mLevel->mNumOrders <= mOrdersBegun + aCount)
		aCount = mLevel->mNumOrders - mOrdersBegun;
	mOrdersBegun += aCount;
	if (mOrdersBegun + 1 == mLevel->mNumOrders)
	{
		aCount++;
		mOrdersBegun++;
	}
	mSpeedBonusTime = (float)aCount * SPEED_BONUS_PER_CUSTOMER;
	int aLeft = aCount;
	for (size_t i = 0; i < mEventTimers.size(); i++)
	{
		engine::Ref<Order> anOrder = mEventTimers[i]->tick();
		if (anOrder != nullptr && aLeft > 0)
		{
			mWaveOrders.push_back(anOrder);
			aLeft--;
			mWaveCustomersLeft++;
		}
	}
	while (aLeft > 0)
	{
		Order* anOrder = new Order();
		anOrder->clear();
		mWaveOrders.push_back(anOrder);
		aLeft--;
		mWaveCustomersLeft++;
	}
	shuffleOrders();
}

// 0x459e20: each order in turn is drawn from what is left and numbered from the last number down; the draw is made
// even for the last one
void GameMode::shuffleOrders()
{
	std::list<engine::Ref<Order>> aShuffled;
	int aNumber = (int)mWaveOrders.size();
	while (!mWaveOrders.empty())
	{
		aNumber--;
		int aDrawn = engine::randomRange(0, (int)mWaveOrders.size() - 1);
		std::list<engine::Ref<Order>>::iterator anIter = mWaveOrders.begin();
		while (aDrawn > 0)
		{
			aDrawn--;
			++anIter;
		}
		Order* anOrder = *anIter;
		anOrder->mIndex = aNumber;
		aShuffled.push_back(anOrder);
		mWaveOrders.erase(anIter);
	}
	mWaveOrders.swap(aShuffled);
}

// 0x457100
bool GameMode::assignOrder(Order* theOrder)
{
	CustomerTile* aTile = theOrder->mTile;
	if (aTile == nullptr || !aTile->isEmpty())
	{
		aTile = mMap->findFreeCustomerTile();
		if (aTile == nullptr)
			return false;
		theOrder->mTile = aTile;
	}
	if (theOrder->mCharacter == nullptr)
		theOrder->mCharacter = theOrder->mTile->getCharacter();
	if (theOrder->mPizzaCount == 0)
		theOrder->mPizzaCount = theOrder->mCharacter->mNumPizzas;
	if (theOrder->mWaitTime == 0.0f)
	{
		theOrder->mWaitTime = engine::randomFloat(toPoint(mWaitTime));
		if (PizzaFrenzy::instance()->mGameMode != PizzaFrenzy::MODE_SPEED)
			theOrder->mWaitTime = theOrder->mWaitTime * 3.0f;
		if (theOrder->mEvent != nullptr && theOrder->mEvent->mType == "MovieStar")
			theOrder->mWaitTime = theOrder->mWaitTime * 0.5f;
	}
	if (theOrder->mTopping == nullptr)
		theOrder->mTopping = mMap->getRandomKitchenTopping();
	return true;
}

// 0x459b90: an order that has a house is kept, the pizzas it asks for are counted, and the house is given the order;
// the order of an event that wants no pizza makes the police come
bool GameMode::placeOrder(Order* theOrder)
{
	if (!assignOrder(theOrder))
		return false;
	mActiveOrders.push_back(theOrder);
	if (theOrder->asksForPizza())
		mStats->mPizzasOrdered += theOrder->mPizzaCount;
	else if (CustomerTile* aPoliceTile = mMap->findCustomerTileBySpecial(Character::SPECIAL_POLICE))
		aPoliceTile->callPolice(Character::SPECIAL_POLICE);
	theOrder->mTile->setOrder(theOrder, false);
	return true;
}

// 0x459fc0: the wave's timer is held to a second and a half when the houses are idle, and to the level's gap (not
// under a second and a half) when they are not; the speedy delivery banner is translated next
void GameMode::waveDone()
{
	mSecondsSinceDelivery = 0;
	if (mMap->isIdle())
	{
		if (mWaveTimer > WAVE_TIMER_CAP)
			mWaveTimer = WAVE_TIMER_CAP;
	}
	else if (mWaveTimer > mLevel->mWaveGap)
	{
		mWaveTimer = mLevel->mWaveGap < WAVE_TIMER_CAP ? WAVE_TIMER_CAP : mLevel->mWaveGap;
	}
	if (0.0f < mSpeedBonusTime)
	{
		// the splat "speedyDelivery" flies to the happiness meter, which the bonus is added to when it arrives; the
		// splats are left out, and the happiness is added now
		playSound(SOUND_SPEEDY);
		addHappiness(SPEEDY_HAPPINESS);
	}
}

// 0x456260
void GameMode::addHappiness(int theChange)
{
	int aHappiness = mStats->mHappiness + theChange;
	if (aHappiness > MAX_HAPPINESS)
		aHappiness = MAX_HAPPINESS;
	else if (aHappiness < 0)
		aHappiness = 0;
	mStats->mHappiness = aHappiness;
	if (mStats->mHappiness <= LOW_HAPPINESS)
		setLowHappinessWarning(true);
	else
		setLowHappinessWarning(false);
	mHud->updateHappiness();
}

// 0x456bd0
void GameMode::addTips(int theTips)
{
	mStats->addTips(theTips);
	if (mMap->isIdle() && mWaveTimer > WAVE_TIMER_CAP)
		mWaveTimer = WAVE_TIMER_CAP;
}

// 0x455b80
void GameMode::slowTime(float theSeconds, float theFactor)
{
	mSlowFactor = theFactor;
	mSlowTime = theSeconds;
}

// 0x457b80: the hud shows the warning and the music is the warning's; off, the level's music is back
void GameMode::setLowHappinessWarning(bool theOn)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mHud->setWarning(theOn);
	if (theOn)
		anApp->mMusicPlayer->play(anApp->mTileManifest->getMusic(MUSIC_LOW_HAPPINESS));
	else
		anApp->mMusicPlayer->play(mMusicClip);
}

// 0x4563d0
bool GameMode::isPoliceBusy()
{
	CustomerTile* aTile = mMap->findCustomerTileBySpecial(Character::SPECIAL_POLICE);
	if (aTile != nullptr)
	{
		PizzaPopup* aPopup = aTile->getOccupant();
		if (aPopup != nullptr && aPopup->getKind() != 2)
			return true;
	}
	return false;
}

// 0x456410
void GameMode::dismissPolice()
{
	CustomerTile* aTile = mMap->findCustomerTileBySpecial(Character::SPECIAL_POLICE);
	if (aTile != nullptr)
	{
		PizzaPopup* aPopup = aTile->getOccupant();
		if (aPopup != nullptr && aPopup->getKind() == 2 && !hasPendingSpecialOrder())
			aTile->hangUp();
	}
}

// 0x456e50
bool GameMode::hasPendingSpecialOrder()
{
	bool aFound = false;
	for (size_t i = 0; i < mMap->mCustomers.size() && !aFound; i++)
	{
		CustomerTile* aTile = static_cast<CustomerTile*>(mMap->mCustomers[i].get());
		if (aTile->isClickable())
		{
			PizzaPopup* aPopup = aTile->getOccupant();
			if (aPopup->getKind() == 0)
			{
				Order* anOrder = aPopup->getOrder();
				if (anOrder->mEvent != nullptr && !anOrder->mEvent->mDeliver)
					aFound = true;
			}
		}
	}
	for (size_t i = 0; i < mPicks.size() && !aFound; i++)
	{
		Order* anOrder = mPicks[i]->getOrder();
		if (anOrder->mEvent != nullptr && !anOrder->mEvent->mDeliver)
			aFound = true;
	}
	if (aFound)
		return true;
	for (std::list<engine::Ref<Order>>::iterator anIter = mActiveOrders.begin(); anIter != mActiveOrders.end();
	     ++anIter)
	{
		if ((*anIter)->mEvent != nullptr && !(*anIter)->mEvent->mDeliver)
			return true;
	}
	return false;
}

// 0x457900
void GameMode::hangUpAll(bool theQuiet)
{
	mStats->mComboLength = 0;
	mActiveOrders.clear();
	mStats->mOrderCount -= (int)mWaveOrders.size();
	mWaveOrders.clear();
	mSpeedBonusTime = 0;
	for (size_t i = 0; i < mMap->mCustomers.size(); i++)
	{
		CustomerTile* aTile = static_cast<CustomerTile*>(mMap->mCustomers[i].get());
		if (aTile->getActiveOrder() != nullptr)
		{
			aTile->hangUp();
			mStats->mOrderCount--;
		}
	}
	if (!theQuiet)
	{
		PizzaFrenzy::instance()->mSounds->play(SOUND_HANGUP, 1.0f, 1.0f);
		mStats->mOrderCount++;
		mWaveCustomersLeft++;
		action(ACTION_CUSTOMER_HANGUP);
	}
	mWaveCustomersLeft = 0;
	dismissPolice();
	waveDone();
}

// 0x457760
void GameMode::action(const std::string& theAction)
{
	if (theAction == ACTION_CUSTOMER_HANGUP)
	{
		if (!mActiveOrders.empty())
			mActiveOrders.pop_front();
		addHappiness(HANGUP_HAPPINESS);
		mWaveCustomersLeft--;
		mDeliveryStreak = 0;
		mSpeedBonusTime = 0;
		mStats->mOrderCount--;
		if (mWaveCustomersLeft == 0)
			waveDone();
	}
	else if (theAction == ACTION_FALSE_ARREST)
	{
		addHappiness(HANGUP_HAPPINESS);
		mDeliveryStreak = 0;
	}
	else if (theAction == ACTION_SPECIAL_HANGUP)
	{
		if (!mWaveOrders.empty())
			mWaveOrders.pop_front();
		mWaveCustomersLeft--;
		mStats->mOrderCount--;
		if (mWaveCustomersLeft == 0)
			waveDone();
		dismissPolice();
	}
}

// 0x458c40
void GameMode::clearPicks()
{
	for (size_t i = 0; i < mPicks.size(); i++)
		mPicks[i]->unpick();
	releasePicks();
}

// 0x458060
void GameMode::releasePicks()
{
	mPicks.clear();
	if (mScreen != nullptr)
	{
		if (mCoupon != nullptr)
		{
			mCoupon->putBack();
			mCoupon = nullptr;
			mScreen->setCouponCursor(nullptr);
		}
		mScreen->hideOrderCursor();
	}
}

// 0x456130
void GameMode::rightClick()
{
	if (!mRightClickHandled)
	{
		if (mCoupon != nullptr)
		{
			mCoupon->putBack();
			mCoupon = nullptr;
			mScreen->setCouponCursor(nullptr);
			mRightClickHandled = false;
			return;
		}
		clearPicks();
	}
	mRightClickHandled = false;
}

// 0x456ac0: the level is over for want of happiness, or when the waves are done, the houses are idle and no vehicle
// is out: in the second case the pizzas delivered against those ordered decide the way it ends (the end's states,
// 0x456660 and the prize blimp 0x456730; the application's endLevel is what follows them)
void GameMode::checkLevelEnd()
{
	if (mStats->mHappiness < 1)
	{
		mStates.changeState(new ModeEndState(this, true));
		return;
	}
	if (!mWavesDone)
		return;
	if (!mMap->isIdle())
		return;
	// a tip that waits for its click holds the level (the map's list at +0x68)
	if (!mMap->mTips.empty())
		return;
	if (mStats->mPizzasDelivered < mStats->mPizzasOrdered)
		mStates.changeState(new ModeEndState(this, false));
	else
		mStates.changeState(new PrizeBlimpState(this));
}

// port: the "win" and "lose" test commands. A win clears what stands in the way of checkLevelEnd's blimp path (the
// wave's orders and the orders placed, the picks, the pizzas on their way, the tiles' popups and tips, as finishLevel
// clears them) and counts every pizza ordered as delivered; a loss takes the happiness away, which the check ends the
// game on. The check itself runs in the play state's next updates.
void GameMode::endLevelForTest(bool theWin)
{
	if (!theWin)
	{
		addHappiness(-MAX_HAPPINESS);
		return;
	}
	mWaveOrders.clear();
	mActiveOrders.clear();
	mDeliveries.clear();
	mOrdersBegun = mLevel->mNumOrders;
	mWavesDone = true;
	mWaveCustomersLeft = 0;
	mStats->mOrderCount = 0;
	mStats->mPizzasDelivered = mStats->mPizzasOrdered;
	clearPicks();
	clearTiles();
}

// 0x456320
void GameMode::finishLevel()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	anApp->mMusicPlayer->stop();
	mMusicClip = nullptr;
	if (mSlowSound != nullptr && mSlowSound->isPlaying())
		mSlowSound->stop();
	clearPicks();
	clearTiles();
	clearEventPopups();
	mStats->mScore = mStats->mScore + mStats->mTipMoney;
}

// 0x456c90
void GameMode::clearTiles()
{
	for (size_t i = 0; i < mMap->mTips.size(); i++)
		releaseTip(mMap->mTips[i]);
	for (size_t i = 0; i < mMap->mCustomers.size(); i++)
		static_cast<BuildingTile*>(mMap->mCustomers[i].get())->removeOccupant();
	for (size_t i = 0; i < mMap->mKitchens.size(); i++)
		static_cast<BuildingTile*>(mMap->mKitchens[i].get())->removeOccupant();
}

// 0x45adb0: a house of a customer picks its order, or the kitchen's pizza is sent for the orders picked, or the police
// take the customers of the events picked; a click that is not taken flashes the cursor's indicators and sounds
void GameMode::click(BuildingTile* theTile)
{
	if (!theTile->isClickable())
		return;
	PizzaPopup* aPopup = theTile->getOccupant();
	if (mCoupon != nullptr)
	{
		if (aPopup->getKind() != POPUP_CUSTOMER)
		{
			playSound(SOUND_INVALID_CLICK);
			return;
		}
		useCoupon(static_cast<CustomerPopup*>(aPopup));
	}
	if (aPopup->getKind() == POPUP_CUSTOMER)
	{
		if (mPicks.empty())
		{
			pickOrder(static_cast<CustomerTile*>(theTile));
			playSound(SOUND_PICK);
			return;
		}
		Order* aPickedOrder = mPicks.front()->getOrder();
		Order* anOrder = static_cast<CustomerTile*>(theTile)->getOrder();
		if (aPickedOrder->mTopping != anOrder->mTopping)
		{
			mScreen->flashCursor();
			playSound(SOUND_INVALID_CLICK);
			return;
		}
		if ((int)mPicks.size() < mStats->getPickLimit(anOrder->mTopping))
		{
			pickOrder(static_cast<CustomerTile*>(theTile));
			playSound(SOUND_PICK);
			return;
		}
		mScreen->flashCursor();
		playSound(SOUND_INVALID_CLICK);
	}
	else if (aPopup->getKind() == POPUP_KITCHEN)
	{
		if (mPicks.empty())
		{
			playSound(SOUND_INVALID_CLICK);
			return;
		}
		if (mPicks.front()->getOrder()->mTopping == static_cast<KitchenTile*>(theTile)->getTopping())
		{
			playSound(SOUND_SEND);
			for (size_t i = 0; i < mPicks.size(); i++)
			{
				QueuedDelivery aDelivery;
				aDelivery.mTile = mPicks[i];
				aDelivery.mKitchen = theTile;
				mDeliveries.push_back(aDelivery);
			}
			releasePicks();
			return;
		}
		// the splat "wrongPizza" comes up on the kitchen (left out), and the customers of the orders picked give up
		for (size_t i = 0; i < mPicks.size(); i++)
		{
			mPicks[i]->setImpatient(false);
			mPicks[i]->unpick();
		}
		releasePicks();
	}
	else if (aPopup->getKind() == POPUP_POLICE)
	{
		int aPoliceOrders = 0;
		for (size_t i = 0; i < mPicks.size(); i++)
		{
			SpecialEvent* anEvent = mPicks[i]->getOrder()->mEvent;
			if (anEvent != nullptr && !anEvent->mDeliver)
				aPoliceOrders++;
		}
		if (mPicks.empty() || aPoliceOrders == 0)
		{
			playSound(SOUND_INVALID_CLICK);
			return;
		}
		playSound(SOUND_SEND);
		std::vector<engine::Ref<CustomerTile>>::iterator anIter = mPicks.begin();
		while (anIter != mPicks.end())
		{
			engine::Ref<CustomerTile> aPick = *anIter;
			SpecialEvent* anEvent = aPick->getOrder()->mEvent;
			if (anEvent != nullptr && !anEvent->mDeliver)
			{
				anIter = mPicks.erase(anIter);
				arrest(theTile, aPick);
				aPopup->serve();
			}
			else
				++anIter;
		}
		refreshPickCursor();
	}
}

// 0x45ad20
void GameMode::pickOrder(CustomerTile* theTile)
{
	mPicks.push_back(theTile);
	theTile->pick();
	refreshPickCursor();
}

// 0x4571e0
void GameMode::refreshPickCursor()
{
	mScreen->hideOrderCursor();
	if (!mPicks.empty())
	{
		CustomerPopup* aPopup = static_cast<CustomerPopup*>(mPicks.back()->getOccupant());
		aPopup->buildPickCards(mScreen->mOrderCursor);
	}
}

// 0x455b20: left empty: the special events' popups (the factory 0x44a490 and the classes 0x44a050 to 0x44b100) are
// translated next; the event's pizza is not served until they are
void GameMode::startSpecialEvent(BuildingTile* theKitchen, CustomerTile* theTile)
{
}

// 0x455ff0: the customer's order is finished: the tip comes up on the road cell the vehicle stopped on (the citizen's
// award, for an event that wants no pizza)
void GameMode::orderFinished(BuildingTile* theTile, Order* theOrder, float theSeconds)
{
	engine::IntPair aCell = mMap->cellToPoint(theTile->getRoadCell());
	int aShift = (int)((float)mMap->getCellSize() * TIP_CELL_SHARE);
	engine::Point aPoint((float)(aCell.mFirst + aShift), (float)(aCell.mSecond + aShift));
	TipPopup* aTip;
	if (theOrder->asksForPizza())
		aTip = new TipPopup();
	else
		aTip = new CitizenAward();
	addTip(theOrder, aPoint, aTip);
}

// 0x455d60
void GameMode::addTip(Order* theOrder, const engine::Point& thePoint, TipPopup* theTip)
{
	if (theTip == nullptr)
		theTip = new TipPopup();
	float aSeconds = theOrder == nullptr ? 0.0f : theOrder->mSecondsSinceDelivery;
	float aMultiplier = TIP_SLOW_MULTIPLIER;
	if (aSeconds >= TIP_FAST_SECONDS)
	{
		if (aSeconds < TIP_MEDIUM_SECONDS)
			aMultiplier = TIP_MEDIUM_MULTIPLIER;
	}
	else
		aMultiplier = TIP_FAST_MULTIPLIER;
	theTip->setValue(aMultiplier, theOrder == nullptr ? nullptr : theOrder->mTopping);
	showTip(theTip, thePoint);
}

// 0x4561a0
void GameMode::showTip(TipPopup* theTip, const engine::Point& thePoint)
{
	theTip->setMap(mMap, thePoint);
	mScreen->mTipsGroup->addChild(theTip);
}

// 0x4561d0
void GameMode::releaseTip(TipPopup* theTip)
{
	theTip->addOwnFlags(engine::NODE_REMOVE);
	if (mMap->isIdle() && mWaveTimer > WAVE_TIMER_CAP)
		mWaveTimer = WAVE_TIMER_CAP;
}

// 0x456240
void GameMode::addVehicleNode(Van* theVan)
{
	if (theVan->getLayer() == AIR_LAYER)
		mScreen->mAirGroup->addChild(theVan);
	else
		mScreen->mGroundGroup->addChild(theVan);
}

// 0x456220
void GameMode::removeVehicleNode(Van* theVan)
{
	theVan->addOwnFlags(engine::NODE_REMOVE);
	mMap->removeVehicle(theVan);
}

// 0x45ac60
void GameMode::addEventPopup(EventPopup* thePopup)
{
	if (!mEventPopups.empty())
		mEventPopups.back()->setVisible(false);
	mEventPopups.push_back(thePopup);
	mScreen->addChild(thePopup);
	thePopup->show();
}

// 0x457a90
void GameMode::removeEventPopup(EventPopup* thePopup)
{
	for (std::vector<engine::Ref<EventPopup>>::iterator anIter = mEventPopups.begin(); anIter != mEventPopups.end();
	     ++anIter)
	{
		if (anIter->get() == thePopup)
		{
			mEventPopups.erase(anIter);
			break;
		}
	}
	thePopup->addOwnFlags(engine::NODE_REMOVE);
}

// 0x457b00
void GameMode::clearEventPopups()
{
	for (size_t i = 0; i < mEventPopups.size(); i++)
		mEventPopups[i]->addOwnFlags(engine::NODE_REMOVE);
	mEventPopups.clear();
}

// 0x457c50: with no order picked the coupon is held on the cursor (the one held before goes back); with orders
// picked and the tips to pay the price for each, the orders ask for the coupon's topping
void GameMode::pickCoupon(CouponButton* theCoupon)
{
	int aTotalPrice = theCoupon->getPrice() * (int)mPicks.size();
	if (mPicks.empty())
	{
		if (mCoupon != nullptr)
		{
			mCoupon->putBack();
			mCoupon = nullptr;
			mScreen->setCouponCursor(nullptr);
		}
		mCoupon = theCoupon;
		mScreen->setCouponCursor(theCoupon);
	}
	else if (aTotalPrice <= mStats->mTipMoney)
	{
		playSound(SOUND_COUPON);
		mStats->addTips(-aTotalPrice);
		CustomerPopup* aPopup = nullptr;
		for (size_t i = 0; i < mPicks.size(); i++)
		{
			aPopup = static_cast<CustomerPopup*>(mPicks[i]->getOccupant());
			Order* anOrder = aPopup->getOrder();
			anOrder->mTopping = theCoupon->getTopping();
			aPopup->replaceOrder(anOrder, false);
		}
		mScreen->hideOrderCursor();
		theCoupon->setPrice(theCoupon->getPrice() + COUPON_PRICE_STEP);
		theCoupon->putBack();
		aPopup->buildPickCards(mScreen->mOrderCursor);
	}
}

// 0x457e80: the coupon held is used on the customer's order, and paid for
void GameMode::useCoupon(CustomerPopup* thePopup)
{
	Order* anOrder = thePopup->getOrder();
	anOrder->mTopping = mCoupon->getTopping();
	thePopup->replaceOrder(anOrder, true);
	playSound(SOUND_COUPON);
	mStats->addTips(-mCoupon->getPrice());
	mCoupon->setPrice(mCoupon->getPrice() + COUPON_PRICE_STEP);
	mCoupon->putBack();
	mCoupon = nullptr;
	mScreen->setCouponCursor(nullptr);
}

// 0x456d10
void GameMode::arrest(BuildingTile* thePoliceTile, CustomerTile* theTile)
{
	static_cast<PolicePopup*>(thePoliceTile->getOccupant())->sendCar(theTile);
	mStats->mCustomersServed++;
	mWaveCustomersLeft--;
	mStats->mOrderCount--;
	if (mWaveCustomersLeft == 0)
		waveDone();
	CustomerTile* aPoliceHouse = static_cast<CustomerTile*>(thePoliceTile);
	if (hasPendingSpecialOrder())
	{
		aPoliceHouse->callPolice(Character::SPECIAL_POLICE);
		return;
	}
	Order* anOrder = new Order();
	anOrder->mTile = aPoliceHouse;
	anOrder->mCharacter = aPoliceHouse->getCharacter();
	anOrder->mTopping = theTile->getOrder()->mTopping;
	anOrder->mPizzaCount = anOrder->mCharacter->mNumPizzas;
	mStats->mPizzasOrdered += anOrder->mPizzaCount;
	mStats->mOrderCount++;
	mStats->mCustomersServed--;
	mWaveCustomersLeft++;
	anOrder->mWaitTime = theTile->getOrder()->mWaitTime;
	aPoliceHouse->setOrder(anOrder, false);
}

// 0x45a300: the pizza the kitchen made is given to the customer of the tile: the combo's length and best are counted,
// and the money of the pizzas and of a frenzy is added; the splats that show the money (the combo's text, the frenzy's
// and the pizzas' "toppingBonus") are left out. The kitchen's van goes to the house, which serves and then finishes
// the order (the tip); the event that wants no pizza is sent away by the police instead
void GameMode::deliver(BuildingTile* theKitchen, CustomerTile* theTile, bool theSendVan)
{
	Order* anOrder = theTile->getOrder();
	if (anOrder->mTopping == mComboTopping && anOrder->asksForPizza())
		mStats->mComboLength++;
	else
	{
		mComboTopping = anOrder->mTopping;
		mStats->mComboLength = 1;
	}
	int& aBestCombo = mStats->mBestCombos[mComboTopping];
	if (aBestCombo < mStats->mComboLength)
	{
		aBestCombo = mStats->mComboLength;
		int anUpgradeIndex = -1;
		for (size_t i = 0; mComboTopping != nullptr && i < mComboTopping->mUpgrades.size(); i++)
		{
			if (mStats->mComboLength == mComboTopping->mUpgrades[i]->mCombo)
				anUpgradeIndex = (int)i;
		}
		if (mStats->mToppingLevels[mComboTopping] < anUpgradeIndex)
			playSound(SOUND_UPGRADE);
	}
	if (!mActiveOrders.empty())
		mActiveOrders.pop_front();
	anOrder->mSecondsSinceDelivery = mSecondsSinceDelivery;
	mSecondsSinceDelivery = 0;
	if (!anOrder->asksForPizza())
	{
		theTile->removeOccupant();
		dismissPolice();
	}
	else
	{
		mStats->mCustomersServed++;
		int aLength = mStats->mComboLength;
		if (aLength > 0)
		{
			float aMultiplier = (float)(aLength - 1) * COMBO_STEP + 1.0f;
			int aFives = aLength / FRENZY_COMBO;
			mMap->showCombo(mComboTopping, aLength, aFives);
			if (aLength % FRENZY_COMBO == 0 && aFives > 0)
			{
				playSound(formatText(SOUND_FRENZY, std::min(aFives - 1, FRENZY_MAX_SOUND)));
				mStats->addTips(FRENZY_BONUS * aFives);
			}
			playSound(SOUND_TOPPING_COMBO, 1.0f, aMultiplier);
		}
		mDeliveryStreak++;
		if (mDeliveryStreak == STREAK_LENGTH)
		{
			mDeliveryStreak = 0;
			addHappiness(STREAK_HAPPINESS);
		}
		Topping* aTopping = anOrder->mTopping;
		int aPizzaCount = anOrder->mPizzaCount;
		if (aPizzaCount > 0)
			mStats->addTips(aTopping->getUpgradeValue(mStats->getUpgrade(aTopping)) * aPizzaCount);
		mStats->mPizzasDelivered += aPizzaCount;
		if (theSendVan)
			static_cast<KitchenTile*>(theKitchen)->sendVan(theTile);
		else
			mSpeedBonusTime = 0;
	}
	mWaveCustomersLeft--;
	mStats->mOrderCount--;
	if (mWaveCustomersLeft == 0)
		waveDone();
}
