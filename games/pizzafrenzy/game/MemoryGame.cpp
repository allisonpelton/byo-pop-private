#include "MemoryGame.h"
#include "CustomerPopup.h"
#include "GameText.h"
#include "ModeStates.h"
#include "PizzaFrenzy.h"
#include "Random.h"
#include "ScreenNodes.h"
#include "Van.h"

using namespace pizzafrenzy;

namespace
{
const char* const SPLAT_WRONG_CUSTOMER = "wrongCustomer";
const char* const SPLAT_WRONG_PIZZA = "wrongPizza";
const char* const SPLAT_TOPPING_BONUS = "toppingBonus";
const char* const SOUND_TOPPING_COMBO = "bonus_toppingCombo";
const char* const ACTION_CUSTOMER_HANGUP = "customerHangup";

// the customers come at least this many seconds apart
const float MIN_SPAWN_SECONDS = 0.35f;
// a pizza pays this much: ten, or fifteen for a topping that uses its picture as the preview
const int PIZZA_VALUE = 10;
const int PIZZA_VALUE_WITH_PREVIEW = 15;
const float COMBO_STEP = 0.05f;
const int STREAK_LENGTH = 10;
const int STREAK_HAPPINESS = 10;
}

// 0x4606a0
void MemoryGame::startPlay()
{
	mStates.changeState(new ModePlayState(this));
}

// 0x460710: 0x457490's, with the map's idleness for the police's and a least gap between two customers
void MemoryGame::updateWaves(engine::UpdateInfo& theInfo)
{
	updateDeliveries(theInfo);
	if (mWavesDone)
		return;
	mSecondsSinceDelivery += theInfo.mSeconds;
	mWaveTimer -= theInfo.mSeconds;
	mSpeedBonusTime -= theInfo.mSeconds;
	if (mWaveTimer < 0.0f && mWaveOrders.empty() && mMap->isIdle())
		startNextWave();
	mSpawnTimer -= theInfo.mSeconds;
	while (!mWaveOrders.empty() && mSpawnTimer <= 0.0f)
	{
		engine::Ref<Order> anOrder = mWaveOrders.back();
		if (!placeOrder(anOrder))
			break;
		mWaveOrders.pop_back();
		float aGap = engine::randomFloat(engine::Point(mSpawnDelay.mFirst, mSpawnDelay.mSecond));
		mSpawnTimer = aGap < MIN_SPAWN_SECONDS ? MIN_SPAWN_SECONDS : aGap;
		mWaveTimer = mLevel->mWaveGap + anOrder->mWaitTime;
	}
	if (mLevel->mNumOrders <= mOrdersBegun && mWaveOrders.empty())
		mWavesDone = true;
}

bool MemoryGame::isOldest(Order* theOrder)
{
	return !mActiveOrders.empty() && theOrder == mActiveOrders.front().get();
}

void MemoryGame::showSplat(BuildingTile* theBuilding, const char* theName)
{
	engine::Point aPosition = *theBuilding->getPosition();
	PizzaFrenzy::instance()->mSplatFactory->create(theName, aPosition.x, aPosition.y);
}

void MemoryGame::countInOrder(Order* theOrder)
{
	mStats->mComboLength++;
	theOrder->mSecondsSinceDelivery = mSecondsSinceDelivery;
	mSecondsSinceDelivery = 0;
	PizzaFrenzy::instance()->mSounds->play(SOUND_TOPPING_COMBO, 1.0f,
	                                       (float)(mStats->mComboLength - 1) * COMBO_STEP + 1.0f);
}

// 0x461030
void MemoryGame::click(BuildingTile* theTile)
{
	if (!theTile->isClickable())
		return;
	PizzaPopup* aPopup = theTile->getOccupant();
	if (mCoupon == nullptr && aPopup->getKind() != POPUP_CUSTOMER && aPopup->getKind() != POPUP_POLICE &&
	    !mPicks.empty() && mPicks.front()->getOrder()->mTopping != static_cast<KitchenTile*>(theTile)->getTopping())
	{
		// the pizza is not what the first order picked asks for: the splat "wrongPizza" comes up on the kitchen, the
		// customers of the orders picked give up, and then every customer goes
		showSplat(theTile, SPLAT_WRONG_PIZZA);
		for (size_t i = 0; i < mPicks.size(); i++)
		{
			mPicks[i]->setImpatient(false);
			mPicks[i]->unpick();
		}
		releasePicks();
		hangUpAll(true);
		return;
	}
	GameMode::click(theTile);
}

// 0x460c40
void MemoryGame::deliver(BuildingTile* theKitchen, CustomerTile* theTile, bool theSendVan)
{
	Order* anOrder = theTile->getOrder();
	if (anOrder->mCharacter->mSpecial == Character::SPECIAL_POLICE)
		countInOrder(anOrder);
	else
	{
		if (!isOldest(anOrder))
		{
			showSplat(theKitchen, SPLAT_WRONG_CUSTOMER);
			theTile->setImpatient(false);
			hangUpAll(true);
			return;
		}
		mActiveOrders.pop_front();
		countInOrder(anOrder);
	}
	if (anOrder->asksForPizza())
	{
		mStats->mCustomersServed++;
		engine::Point aKitchen = *theKitchen->getPosition();
		int aPizzaValue = anOrder->mTopping->mUsesImageAsPreview != 0 ? PIZZA_VALUE_WITH_PREVIEW : PIZZA_VALUE;
		int aPizzaCount = anOrder->mPizzaCount;
		std::string aBonusText = formatText(GT(0x500e2c), aPizzaValue, aPizzaCount);
		mStats->addTips(aPizzaValue * aPizzaCount);
		mDeliveryStreak++;
		if (mDeliveryStreak == STREAK_LENGTH)
		{
			mDeliveryStreak = 0;
			addHappiness(STREAK_HAPPINESS);
		}
		mStats->mPizzasDelivered += aPizzaCount;
		if (!theSendVan)
			mSpeedBonusTime = 0;
		else
		{
			// the money of the pizzas comes up over the kitchen as the splat "toppingBonus"
			engine::Splat* aBonus = PizzaFrenzy::instance()->mSplatFactory->create(SPLAT_TOPPING_BONUS, aKitchen.x,
			                                                                      aKitchen.y);
			aBonus->setText(aBonusText);
			static_cast<KitchenTile*>(theKitchen)->sendVan(theTile);
		}
	}
	else
	{
		theTile->removeOccupant();
		dismissPolice();
	}
	mWaveCustomersLeft--;
	mStats->mOrderCount--;
	if (mWaveCustomersLeft == 0)
		waveDone();
}

// 0x4611a0
void MemoryGame::startSpecialEvent(BuildingTile* theKitchen, CustomerTile* theTile)
{
	if (!mActiveOrders.empty() && !isOldest(theTile->getOrder()))
	{
		showSplat(theKitchen, SPLAT_WRONG_CUSTOMER);
		theTile->setImpatient(false);
		hangUpAll(true);
		return;
	}
	GameMode::startSpecialEvent(theKitchen, theTile);
}

// 0x4609f0
void MemoryGame::arrest(BuildingTile* thePoliceTile, CustomerTile* theTile)
{
	Order* anOrder = theTile->getOrder();
	if (!isOldest(anOrder))
	{
		showSplat(thePoliceTile, SPLAT_WRONG_CUSTOMER);
		action(ACTION_CUSTOMER_HANGUP);
		theTile->setImpatient(false);
		hangUpAll(true);
		return;
	}
	mActiveOrders.pop_front();
	countInOrder(anOrder);
	GameMode::arrest(thePoliceTile, theTile);
}

// 0x460690
void MemoryGame::waveDone()
{
	mStats->mComboLength = 0;
	GameMode::waveDone();
}
