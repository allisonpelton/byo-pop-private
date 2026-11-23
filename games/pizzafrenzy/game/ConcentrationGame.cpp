#include "ConcentrationGame.h"
#include "CustomerPopup.h"
#include "PizzaFrenzy.h"
#include "Random.h"

using namespace pizzafrenzy;

namespace
{
const char* const WAIT_SOUND = "popup_wait";
const char* const FLIPOVER_SOUND = "popup_flipover";

// the music is this much of its volume while the orders show
const float MUSIC_SHARE_WHILE_SHOWING = 0.5f;
// the seconds the orders show: this share of a wait drawn from the wave's range, for each customer
const float SHOW_SHARE_PER_CUSTOMER = 0.06666667f;
const float SPEED_BONUS_PER_CUSTOMER = 1.5f;
// the show's seconds are over (so the orders are hidden at the next update)
const float SHOW_OVER = -1.0f;

engine::Point toPoint(const engine::FloatPair& thePair)
{
	return engine::Point(thePair.mFirst, thePair.mSecond);
}
}

// 0x45bbd0
void ConcentrationGame::setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats)
{
	SpeedGame::setup(theScreen, theMap, theHud, theStats);
	mWaitSound = PizzaFrenzy::instance()->mSounds->getVoice(WAIT_SOUND);
	if (mWaitSound != nullptr)
		mWaitSound->setLoop(true);
	connectRightButton();
}

// 0x45b4f0's and 0x45b6e0's: the sound starts if it does not play
void ConcentrationGame::startWaitSound()
{
	if (mWaitSound != nullptr && !mWaitSound->isPlaying())
	{
		mWaitSound->play();
		PizzaFrenzy::instance()->mMusicPlayer->setVolume(mMusicVolume * MUSIC_SHARE_WHILE_SHOWING);
	}
}

// 0x45b4f0
void ConcentrationGame::ordersChanged()
{
	startWaitSound();
	int aClickable = 0;
	for (size_t i = 0; i < mMap->mCustomers.size(); i++)
	{
		if (static_cast<CustomerTile*>(mMap->mCustomers[i].get())->isClickable())
			aClickable++;
	}
	float aSeconds = engine::randomFloat(toPoint(mWaitTime));
	// The port's fix of a lock (IDENTICAL_PLAY.md, "Deviations left"): with no customer clickable (a wave's orders still
	// showing, every customer dimmed) the original sets the seconds to 0, and since updateWaves turns the orders over
	// only while they run, the dimmed customers stay dimmed and the level never ends. The event changed no order then,
	// so the seconds the orders show for are kept. The random draw is made either way, as the original's.
	if (aClickable == 0)
		return;
	mSpawnTimer = (float)((double)aSeconds * SHOW_SHARE_PER_CUSTOMER * aClickable);
}

// 0x45b590
void ConcentrationGame::hideOrders()
{
	for (size_t i = 0; i < mMap->mCustomers.size(); i++)
	{
		BuildingTile* aTile = static_cast<BuildingTile*>(mMap->mCustomers[i].get());
		PizzaPopup* aPopup = aTile->getOccupant();
		if (aPopup != nullptr && aPopup->getKind() == POPUP_CUSTOMER && aPopup->isPicked())
			static_cast<CustomerPopup*>(aPopup)->hideOrder();
	}
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	anApp->mSounds->play(FLIPOVER_SOUND, 1.0f, 1.0f);
	if (mWaitSound != nullptr && mWaitSound->isPlaying())
		mWaitSound->stop();
	anApp->mMusicPlayer->setVolume(mMusicVolume);
	mSecondsSinceDelivery = 0;
	mSpeedBonusTime = (float)mWaveCustomersLeft * SPEED_BONUS_PER_CUSTOMER;
}

// 0x45b6e0: the customers are placed at once, with no wait between them, and the seconds in mSpawnTimer (the speed
// mode's gap between customers) are the ones the orders show for
void ConcentrationGame::updateWaves(engine::UpdateInfo& theInfo)
{
	updateDeliveries(theInfo);
	if (mWavesDone)
		return;
	mSecondsSinceDelivery += theInfo.mSeconds;
	mWaveTimer -= theInfo.mSeconds;
	mSpeedBonusTime -= theInfo.mSeconds;
	if (mWaveTimer < 0.0f && mWaveOrders.empty() && mMap->isIdle())
	{
		startNextWave();
		double aCount = (double)mWaveOrders.size();
		mSpawnTimer = (float)(aCount * engine::randomFloat(toPoint(mWaitTime)) * SHOW_SHARE_PER_CUSTOMER);
	}
	while (!mWaveOrders.empty())
	{
		engine::Ref<Order> anOrder = mWaveOrders.back();
		if (!placeOrder(anOrder))
			break;
		mWaveOrders.pop_back();
		mWaveTimer = mLevel->mWaveGap + anOrder->mWaitTime;
		startWaitSound();
	}
	if (mSpawnTimer == 0.0f)
		return;
	mSpawnTimer -= theInfo.mSeconds;
	if (mSpawnTimer > 0.0f || !mWaveOrders.empty())
		return;
	mSpawnTimer = 0;
	hideOrders();
}

// 0x45b370
void ConcentrationGame::rightClick()
{
	GameMode::rightClick();
	if (mSpawnTimer > 0.0f)
		mSpawnTimer = SHOW_OVER;
}

// 0x45ba10
bool ConcentrationGame::placeOrder(Order* theOrder)
{
	return placeOrderAs(theOrder, true);
}
