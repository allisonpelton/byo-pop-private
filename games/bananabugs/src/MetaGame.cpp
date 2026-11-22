#include "PathTrace.h"
#include "MetaGame.h"

#include "Anim.h"
#include "Cursor.h"
#include "Emitter.h"
#include "GameApp.h"
#include "InGameMenu.h"
#include "HighScore.h"
#include "Map.h"
#include "MenuButton.h"
#include "ObjString.h"
#include "Player.h"
#include "Res.h"
#include "Slots.h"
#include "SoundQueue.h"
#include "Transition.h"

#include <cstdio>

using namespace Sexy;

bool MetaGame::sFirstVisit = true;
bool MetaGame::sOverOffer = false;

// The key events of the game's own tracks and of its objects'
enum MetaEvent
{
	META_EVENT_START = 1,          // the start track's two keys: the casino button may show
	META_EVENT_STAR_BURST = 2,     // a life icon is on its way: stars burst from it
	META_EVENT_LIFE_ARRIVED = 3,   // a life icon has come: the lives count goes up
	META_EVENT_PANEL_OPEN = 4,     // the panel has come in: it may say everything is sold
	META_EVENT_CASINO_SHOWN = 5,   // the casino button has appeared: it may be clicked
};

// What the game is doing (mState)
enum MetaState
{
	META_WAIT_START = 0,           // for the start track's first event and the doors
	META_INTRO = 1,                // the first picture is shown until the mouse is released
	META_SLOTS_START = 2,
	META_SLOTS = 3,                // the casino's game runs
	META_LIVES_START = 4,
	META_LIVES = 5,                // the lives fly in
	META_SHOP = 6,                 // the player buys
	META_MAP = 7,                  // the map shows the way on
	META_LEAVE = 9,                // the map is done
	META_LEAVING = 10,             // the screen goes (TakeLevelChosen)
};

// The buttons' numbers (ObjBase::mButtonId); an offer's is its place from 1
enum MetaButton
{
	META_BUTTON_FIRST_OFFER = 1,
	META_BUTTON_LAST_OFFER = META_OFFERS,
	META_BUTTON_DONE = 99,
	META_BUTTON_CASINO = 100,
};

// The tracks of the panel and of the life icons, sold-out pictures, buttons and texts
enum MetaTrack
{
	PANEL_TRACK_OPEN = 0,
	PANEL_TRACK_HIDE = 1,          // the one it starts on: off to the right, not shown
	PANEL_TRACK_CLOSE = 2,
	BUTTON_TRACK_NORMAL = 0,
	BUTTON_TRACK_DIM = 1,          // an offer button's: dimmed, as sold or too dear
	DONE_TRACK_HOVER = 1,
	DONE_TRACK_LEAVE = 2,
	CASINO_TRACK_SHOW = 0,
	CASINO_TRACK_HOVER = 1,
	CASINO_TRACK_LEAVE = 2,
	TIP_TRACK_SHOW = 0,
	TIP_TRACK_HIDE = 1,
};

const int META_ICON_CEL = 229;      // the cel's size of the offers' pictures
const int META_LIVES_WAIT = 25;     // frames before the first life flies in, when there are lives
const int META_NO_LIVES_WAIT = 15;
const int META_LIFE_GAP = 40;       // frames between lives
const int META_FIRST_VISIT_WAIT = 15;
const int META_INTRO_UNTIL = 5;     // the map's progress after which the first picture is not shown
const float META_CORONA_SCALE = 4.0f;

// The picture of each building's offers and its cels
struct MetaIconDef
{
	const char* mName;
	int mCels;
};

static const MetaIconDef sIconDefs[META_BUILDINGS] = {
	{"images/meta/meta_icon_leisure", 2},
	{"images/meta/meta_icon_house", 3},
	{"images/meta/meta_icon_military", 3},
	{"images/meta/meta_icon_shotspeed", 3},
	{"images/meta/meta_icon_spearcraft", 2},
	{"images/meta/meta_icon_helper", 3},
	{"images/meta/meta_icon_flightschool", 3},
	{"images/meta/meta_icon_greenhouse", 3},
	{"images/meta/meta_icon_weather", 3},
	{"images/meta/meta_icon_idol", 3},
};

// What each entry says (0x44d500). The number of the entry is its place; the kinds are MetaBuildingKind.
struct MetaEntryDef
{
	int mKind;
	int mLevelIndex;
	int mLevelNumber;
	int mPrerequisite;
	const char* mName;
	const char* mLines[META_ENTRY_LINES];
};

static const MetaEntryDef sEntryDefs[META_ENTRIES] = {
	{BUILDING_MILITARY, 0, 1, META_ENTRY_NONE, "Missile Launcher", {"Unlocks a coin", "that gives you a", "missile to shoot", "at those bugs!"}},
	{BUILDING_LEISURE, 0, 1, META_ENTRY_NONE, "Banana Casino", {"Win powerful coins", "or squander all of", "your bananas in a", "game of chance!"}},
	{BUILDING_FLIGHT_SCHOOL, 0, 1, META_ENTRY_NONE, "Flight School", {"Get 1 more", "life for each", "visit to monkeytown", nullptr}},
	{BUILDING_HOUSE, 0, 1, META_ENTRY_NONE, "Battle Monkeys", {"Adds a monkey to", "the scoreboard who", "can zap the bugs", "with a blowgun!"}},
	{BUILDING_SHOT_SPEED, 0, 1, META_ENTRY_NONE, "Shot Speed x1", {"Increase the speed", "of your rope for", "each shot by 5%", ""}},
	{BUILDING_HOUSE, 1, 2, 3, "Battle Monkeys", {"Add another monkey", "the scoreboard who", "can zap the bugs", "with a blowgun!"}},
	{BUILDING_GREENHOUSE, 0, 1, META_ENTRY_NONE, "Banana Garden", {"Increase your", "bananas per bunch", "on each level from", "2 bananas to 3!"}},
	{BUILDING_MILITARY, 1, 2, 0, "Coco Bomb", {"Unlocks the Coco", "Bomb coin.  You", "can drop the bomb", "for bug carnage! "}},
	{BUILDING_LEISURE, 1, 2, 1, "The Disco", {"Unlocks the Disco", "Ball coin to", "distract the bugs", "with DANCE FEVER!"}},
	{BUILDING_GREENHOUSE, 1, 2, 6, "Banana Greenhouse", {"Increase your", "bananas per bunch", "on each level from", "3 bananas to 4!"}},
	{BUILDING_FLIGHT_SCHOOL, 1, 2, 2, "Flight School", {"Get 2 more", "lives for each", "visit to monkeytown", nullptr}},
	{BUILDING_WEATHER, 0, 1, META_ENTRY_NONE, "Rain Storm", {"Unlock the rain", "coin to summon a", "tropical downpour,", "slowing the bugs!"}},
	{BUILDING_HOUSE, 2, 3, 5, "Battle Monkeys", {"Add two more", "monkeys to make a", "total of four,", "with blowguns!"}},
	{BUILDING_IDOL, 0, 1, META_ENTRY_NONE, "Monkey God", {"Watch the Monkey", "God zap a pesky", "bug to start off a", "level !"}},
	{BUILDING_SHOT_SPEED, 1, 2, 4, "ShotSpeed 2x", {"Increase the speed", "of your rope for", "each shot by 10%", ""}},
	{BUILDING_HELPER, 0, 1, META_ENTRY_NONE, "Parrot Helper", {"Unlocks a coin for", "your very own pet", "parrot!  Release", "him for big help!"}},
	{BUILDING_GREENHOUSE, 2, 3, 9, "Banana DNA Lab", {"Increase your", "bananas per bunch", "on each level from", "4 bananas to 5!"}},
	{BUILDING_WEATHER, 1, 2, 11, "Earthquake", {"Unlock a coin to", "unleash a furious", "earthquake, shaking", "bugs off the bamboo!"}},
	{BUILDING_FLIGHT_SCHOOL, 2, 3, 10, "Flight School", {"Get 3 more", "lives for each", "visit to monkeytown", nullptr}},
	{BUILDING_SPEAR_CRAFT, 0, 1, META_ENTRY_NONE, "Flaming Arrows", {"Unlocks the coin", "for FLAMING ARROWS", "that will burn any", "bug they hit!"}},
	{BUILDING_SHOT_SPEED, 2, 3, 14, "ShotSpeed 3x", {"Increase the speed", "of your rope for", "each shot by 15%", ""}},
	{BUILDING_MILITARY, 2, 3, 7, "Cannon", {"Unlocks the cannon", "coin to squish", "multiple bugs in a", "single shot!"}},
	{BUILDING_HELPER, 1, 2, 15, "Zuma Helper", {"Unlocks the Zuma", "coin for your", "favorite frog to", "shoot those bugs!"}},
	{BUILDING_IDOL, 1, 2, 13, "Monkey God", {"Watch the Monkey", "God zap two pesky", "bugs to start off a", "level !"}},
	{BUILDING_SPEAR_CRAFT, 1, 2, 19, "Magic Arrows", {"Unlocks the Magic", "Arrows coin to", "shock any bug on", "the pipe it hits!"}},
	{BUILDING_WEATHER, 2, 3, 17, "Hurricane", {"Unlocks a coin to", "conjure hurricane", "winds, blowing bugs", "out of the jungle!"}},
	{BUILDING_IDOL, 2, 3, 23, "Monkey God", {"Watch the Monkey", "God zap three pesky", "bugs to start off a", "level !"}},
};

// 0x448e10: builds the screen and starts it: the buildings the player has, the offers, and the lives to give
MetaGame::MetaGame()
{
	mPanel = nullptr;
	for (int i = 0; i < META_BUILDINGS; i++)
		mBuildings[i] = nullptr;
	Player::sLastUnblocked = -1;
	mMenuButton = nullptr;
	SetState(STATE_Z, 0.01f);
	SetStateInt(STATE_ALPHA, 0);
	mAnim = Anim::Get("images/meta/meta_back", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mCoronaAnim = Anim::Get("images/corona1", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mLifeAnim = Anim::Get("images/meta/meta_monkey", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mArrowAnim = Anim::Get("images/meta/meta_arrow", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mSoldOutAnim = Anim::Get("images/meta/soldout", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mLeaving = false;
	mNoMap = false;
	mUnused2d9 = false;
	mFirstTrackDone = false;
	mBroke = false;
	mUnused4a8 = 0;
	mLivesToGive = 0;
	mBuiltCount = 0;
	mWaitFrames = 0;
	mUnused2e0 = 0x140;
	mUnused2c8 = 0;
	mSlots = nullptr;
	mGameMenu = nullptr;
	mCasinoButton = nullptr;
	mWantsMenu = false;
	mArrowHint = nullptr;
	mCasinoUnlocked = false;
	BuildLeaveTrack();
	BuildStartTrack();
	StartStartTrack();
	Cursor::Show();
	mState = META_WAIT_START;
	BuildPanel();
	BuildSoldOutPanel();
	BuildEntries();
	BuildOffers();
	BuildDoneButton();
	BuildCasinoButton();
	BuildCoronas();
	BuildLifeIcons();
	BuildBuildTip();
	BuildBuyTag();
	BuildMenuButton();
	BuildBananaPanel();
	LoadEntryStatus();
	mNothingLeft = BuildBuildingsAndOffers();
	UpdateBananasText(false);
	Activate(this, false);
	if (Player::sCurrent->mSavedProgress < META_INTRO_UNTIL) {
		BuildArrowHint();
		BuildIntro();
		mShowIntro = true;
	} else {
		mShowIntro = false;
	}
	// the original counts the screens made (0x60e69c), and nothing reads it
	Player::SaveAll();
}

// 0x4490a0: the next visit gives the lives again. The buildings (which may not be shown yet) and the glows are made
// children of this screen, so that it deletes them.
MetaGame::~MetaGame()
{
	sFirstVisit = true;
	for (int i = 0; i < META_BUILDINGS; i++) {
		if (mBuildings[i] != nullptr)
			mBuildings[i]->SetParent(this, nullptr);
	}
	for (int i = 0; i < 2; i++)
		mCoronas[i]->SetParent(this, nullptr);
	Player::SaveAll();
}

// 0x449160: the key events of the screen's own tracks and of its objects
void MetaGame::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case META_EVENT_START:
		if (!mFirstTrackDone) {
			mFirstTrackDone = true;
			if (Player::sCurrent->mFirstUpgradeFlag != 0 && !mCasinoButton->IsActive()) {
				mCasinoButton->mHoverState = 0;
				Activate(mCasinoButton, false);
				mCasinoButton->StartTrack(CASINO_TRACK_SHOW);
				return;
			}
		}
		break;
	case META_EVENT_STAR_BURST:
		theObj->mEmitters->BurstStarRing(0.0f, 0.0f);
		return;
	case META_EVENT_LIFE_ARRIVED:
		mLivesBefore++;
		SetLivesText(mLivesBefore);
		mLivesText->mEmitters->BurstStarRing(0.0f, 0.0f);
		return;
	case META_EVENT_PANEL_OPEN:
		if (mNothingLeft && !mSoldOutPanel->IsActive()) {
			mSoldOutPanel->StartTrack(0);
			Activate(mSoldOutPanel, false);
			gApp->mSoundQueue->Request(SOUND_MAPUNFOLD, 0, false);
			return;
		}
		break;
	case META_EVENT_CASINO_SHOWN:
		mCasinoUnlocked = true;
		break;
	}
}

// 0x4492c0: the game menu, then the steps of the screen
bool MetaGame::Update()
{
	if (!ObjBase::Update())
		return false;
	if (Transition::GetPhase() != TRANSITION_OPEN)
		return true;
	if (mGameMenu != nullptr) {
		int aResult = mGameMenu->GetResult();
		if (aResult == IN_GAME_MENU_RESUME || aResult == IN_GAME_MENU_QUIT) {
			if (aResult == IN_GAME_MENU_QUIT)
				mWantsMenu = true;
			mGameMenu->Close();
			mGameMenu = nullptr;
		}
	}
	if (mGameMenu == nullptr && mMenuButton->TakeClicked())
		mGameMenu = new InGameMenu();
	ObjBase* aHover = sHover;
	switch (mState) {
	case META_WAIT_START:
		if (mFirstTrackDone && Transition::GetPhase() == TRANSITION_OPEN) {
			if (!mShowIntro) {
				mState = META_LIVES_START;
			} else {
				mState = META_INTRO;
				Activate(mIntro, false);
				mIntro->StartTrack(0);
			}
			// the original sets a number nothing reads (0x60e6a0) here
		}
		break;
	case META_INTRO:
		if (sMouseHeld) {
			sMouseHeld = false;
			gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
			mIntro->StartTrack(1);
			mState = META_LIVES_START;
		}
		break;
	case META_SLOTS_START:
		mSlots = new Slots(this);
		mSlots->SetParent(this, nullptr);
		mSlots->Start();
		mState = META_SLOTS;
		break;
	case META_SLOTS:
		if (mSlots == nullptr) {
			mState = META_SHOP;
		} else if (mSlots->IsDone()) {
			mSlots->StartTrack(2);
			mState = META_SHOP;
			mSlots = nullptr;
			if (mPanel != nullptr) {
				mPanel->StartTrack(PANEL_TRACK_OPEN);
				mPanel->Activate(mPanel, false);
			}
			RefreshOffers();
		}
		break;
	case META_LIVES_START:
		if (!sFirstVisit) {
			mWaitFrames = META_FIRST_VISIT_WAIT;
			mLivesToGive = 0;
		} else {
			GiveLives();
		}
		mState = META_LIVES;
		break;
	case META_LIVES:
		if (mLivesToGive < 1) {
			if (mWaitFrames < 1) {
				mState = META_SHOP;
				if (mPanel != nullptr) {
					mPanel->StartTrack(PANEL_TRACK_OPEN);
					mPanel->Activate(mPanel, false);
					ShowHintArrow();
				}
			} else {
				mWaitFrames--;
			}
		} else if (mWaitFrames > 0) {
			mWaitFrames--;
			if (mWaitFrames < 1) {
				mLifeIcons[mLivesToGive]->StartTrack(0);
				Activate(mLifeIcons[mLivesToGive], false);
				gApp->mSoundQueue->Request(SOUND_HELI, 0, false);
				mLivesToGive--;
				mWaitFrames = META_LIFE_GAP;
			}
		}
		break;
	case META_SHOP: {
		if (aHover == mDoneButton) {
			if (mDoneButton->mHoverState == 0) {
				gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
				mDoneButton->StartTrack(DONE_TRACK_HOVER);
				mDoneButton->mHoverState = 1;
			}
		} else if (mDoneButton->mHoverState == 1) {
			mDoneButton->StartTrack(DONE_TRACK_LEAVE);
			mDoneButton->mHoverState = 0;
		}
		if (mCasinoUnlocked) {
			if (aHover == mCasinoButton) {
				if (mCasinoButton->mHoverState == 0) {
					mCasinoButton->StartTrack(CASINO_TRACK_HOVER);
					mCasinoButton->mHoverState = 1;
				}
			} else if (mCasinoButton->mHoverState == 1) {
				mCasinoButton->StartTrack(CASINO_TRACK_LEAVE);
				mCasinoButton->mHoverState = 0;
			}
		}
		bool anOnOffer = false;
		for (int i = 0; i < META_OFFERS; i++) {
			if (aHover == mOffers[i].mButton && mOffers[i].mState == OFFER_AVAILABLE) {
				ShowBuildTip(mOffers[i].mEntryIndex);
				ShowCoronas(mOffers[i].mButton, META_CORONA_SCALE);
				ShowBuyTag(mOffers[i].mButton);
				anOnOffer = true;
				sOverOffer = true;
				aHover = sHover;
			}
		}
		if (!anOnOffer && sOverOffer) {
			sOverOffer = false;
			HideCoronas();
			HideBuildTip();
			ShowTagOnFirstOffer();
		}
		break;
	}
	case META_MAP:
		if (mMap != nullptr && mMap->IsDone()) {
			Deactivate(mMap);
			sMouseHeld = false;
			mMap->Delete();
			mMap = nullptr;
			mState = META_LEAVE;
		}
		break;
	case META_LEAVE:
		mLeaving = true;
		break;
	}
	return true;
}

// 0x4497f0: an offer is bought, or the done or casino button pressed; true once the player has left
bool MetaGame::MouseUp(ObjBase* theObj)
{
	switch (theObj->mButtonId) {
	case META_BUTTON_FIRST_OFFER:
	case META_BUTTON_FIRST_OFFER + 1:
	case META_BUTTON_LAST_OFFER: {
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		MetaOffer& anOffer = mOffers[theObj->mButtonId - 1];
		int aPrice = mBuildings[anOffer.mBuilding]->GetPrice(anOffer.mLevel - 1);
		if (anOffer.mState == OFFER_AVAILABLE && Player::BuyUpgrade(anOffer.mBuilding, aPrice) == 0) {
			if (anOffer.mEntryIndex == 1) {
				Activate(mCasinoButton, false);
				mCasinoButton->StartTrack(CASINO_TRACK_SHOW);
				Player::MarkUpgradeBought();
			}
			mBuildings[anOffer.mBuilding]->SetLevel(mEntries[anOffer.mEntryIndex].mLevelNumber, true);
			gApp->mSoundQueue->Request(SOUND_CONSTRUCT, 0, false);
			UpdateBananasText(true);
			RefreshOffers();
			anOffer.mState = OFFER_BOUGHT;
			anOffer.mButton->StartTrack(BUTTON_TRACK_DIM);
			anOffer.mSoldOut->StartTrack(0);
			Activate(anOffer.mSoldOut, false);
			Deactivate(anOffer.mPrice);
		}
		break;
	}
	case META_BUTTON_DONE:
		if (!mNoMap && Player::sCurrent->mPrevMapLevel < Player::sCurrent->mMapLevel) {
			gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
			mState = META_MAP;
			mMap = new Map();
			mMap->Start(Player::sCurrent->mMapLevel - 1, Player::sCurrent->mMapLevel);
			mMap->SetParent(this, nullptr);
			return true;
		}
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		mLeaving = true;
		break;
	case META_BUTTON_CASINO:
		if (mSlots == nullptr && mCasinoUnlocked && Player::sCurrent->mFirstUpgradeFlag != 0) {
			gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
			mState = META_SLOTS_START;
			// the original sets a number nothing reads (0x60e6a0) here
			mPanel->StartTrack(PANEL_TRACK_CLOSE);
		}
		break;
	}
	return mLeaving;
}

// 0x449b10: an entry is bought when the player's level of its building has reached its level
void MetaGame::LoadEntryStatus()
{
	for (int i = 0; i < META_ENTRIES; i++)
		mEntries[i].mBought = mEntries[i].mLevelNumber <= Player::GetUpgradeLevel(mEntries[i].mKind);
}

// 0x449b40: the flight school's level gives that many lives, which fly in one by one
void MetaGame::GiveLives()
{
	mLivesToGive = Player::GetUpgradeLevel(UPGRADE_SEVENTH);
	mLivesBefore = Player::sCurrent->mLives;
	Player::AddLives(mLivesToGive);
	mWaitFrames = (mLivesToGive <= 0 ? 0 : META_LIVES_WAIT - META_NO_LIVES_WAIT) + META_NO_LIVES_WAIT;
}

// 0x449b90: true once, as the player has chosen to go on; the screen then goes
bool MetaGame::TakeLevelChosen()
{
	if (mLeaving) {
		StartLeaveTrack();
		mState = META_LEAVING;
		mLeaving = false;
		return true;
	}
	return false;
}

// 0x449bc0: the bananas the player has; red when no building can be bought, and the coins' sound
void MetaGame::UpdateBananasText(bool theSound)
{
	mBananasText->SetText(FormatScore(Player::sCurrent->mBananas));
	mBananasText->StartTrack(0);
	if (!mBroke) {
		mBananasText->SetState(STATE_RED, 0.2f);
		mBananasText->SetState(STATE_GREEN, 1.0f);
	} else {
		mBananasText->SetState(STATE_RED, 0.9f);
		mBananasText->SetState(STATE_GREEN, 0.8f);
	}
	mBananasText->SetState(STATE_BLUE, 0.1f);
	mBananasText->SetStateInt(STATE_ALPHA, 1);
	if (theSound)
		gApp->mSoundQueue->Request(SOUND_MONEY, 0, false);
}

// 0x449c90: the number of lives, and the sound of one coming
void MetaGame::SetLivesText(int theLives)
{
	char aText[16];
	sprintf(aText, "%d", theLives);
	mLivesText->SetText(aText);
	mLivesText->StartTrack(0);
	gApp->mSoundQueue->Request(SOUND_POPCORN, 0, false);
}

// 0x449d00: whether any building can be bought, and each offer's price and whether the player can pay it
void MetaGame::RefreshOffers()
{
	mBroke = false;
	int aAffordable = 0;
	for (int i = 0; i < META_BUILDINGS; i++) {
		if (mBuildings[i]->CanAfford())
			aAffordable++;
	}
	if (aAffordable == 0)
		mBroke = true;
	UpdateBananasText(false);
	for (int i = 0; i < META_OFFERS; i++) {
		MetaOffer& anOffer = mOffers[i];
		int aPrice = mBuildings[anOffer.mBuilding]->GetPrice(-1);
		char aText[16];
		if (aPrice < 1000)
			sprintf(aText, "%02d", aPrice);
		else
			sprintf(aText, "%d,%03d", aPrice / 1000, aPrice % 1000);
		anOffer.mPrice->SetText(aText);
		if (anOffer.mState == OFFER_BOUGHT)
			Deactivate(anOffer.mPrice);
		else
			Activate(anOffer.mPrice, false);
		if (anOffer.mState == OFFER_AVAILABLE) {
			int aTrack;
			if (aPrice > Player::sCurrent->mBananas) {
				anOffer.mState = OFFER_TOO_DEAR;
				aTrack = BUTTON_TRACK_DIM;
			} else {
				anOffer.mState = OFFER_AVAILABLE;
				aTrack = BUTTON_TRACK_NORMAL;
			}
			anOffer.mButton->StartTrack(aTrack);
		}
	}
}

// 0x449e40: the ten buildings at the levels the player has, and the (up to) three offers: the ones the player had
// before, or the first three whose earlier level has been bought. True when there is no offer left.
bool MetaGame::BuildBuildingsAndOffers()
{
	mBuiltCount = 0;
	bool aWasStarted = Player::sCurrent->mMetaGameStarted != 0;
	Player::StartMetaGame(-1, -1);
	for (int i = 0; i < META_BUILDINGS; i++) {
		mBuildings[i] = new MetaBuilding(i, this);
		int aLevel = Player::GetUpgradeLevel(i);
		if (aLevel > 0) {
			mBuildings[i]->SetLevel(aLevel, false);
			mBuiltCount++;
		}
	}
	for (int i = 0; i < META_OFFERS; i++)
		Deactivate(mOffers[i].mButton);
	int anOfferCount = 0;
	if (aWasStarted) {
		for (int aSlot = 0; aSlot < 3; aSlot++) {
			int anEntryIndex = Player::sCurrent->mMetaProgress[aSlot];
			if (anEntryIndex < 0)
				continue;
			MetaEntry& anEntry = mEntries[anEntryIndex];
			MetaOffer& anOffer = mOffers[anOfferCount];
			anOffer.mButton->mAnim = mIconAnims[anEntry.mKind];
			anOffer.mButton->SetStateInt(STATE_FRAME, anEntry.mLevelIndex);
			Activate(anOffer.mButton, false);
			anOffer.mBuilding = anEntry.mKind;
			anOffer.mEntryIndex = anEntryIndex;
			anOffer.mLevel = anEntry.mLevelNumber;
			anOffer.mEntry = &anEntry;
			int aPrice = mBuildings[anEntry.mKind]->GetPrice(-1);
			int aTrack;
			if (aPrice > Player::sCurrent->mBananas) {
				anOffer.mState = OFFER_TOO_DEAR;
				aTrack = BUTTON_TRACK_DIM;
			} else {
				anOffer.mState = OFFER_AVAILABLE;
				aTrack = BUTTON_TRACK_NORMAL;
			}
			anOffer.mButton->StartTrack(aTrack);
			if (anEntry.mBought) {
				anOffer.mState = OFFER_BOUGHT;
				anOffer.mButton->StartTrack(BUTTON_TRACK_DIM);
				anOffer.mSoldOut->StartTrack(0);
				Activate(anOffer.mSoldOut, false);
				Deactivate(anOffer.mPrice);
			}
			Player::StartMetaGame(aSlot, anEntryIndex);
			anOfferCount++;
			if (anOfferCount > 2)
				break;
		}
	} else {
		Player::StartMetaGame(0, -1);
		Player::StartMetaGame(1, -1);
		Player::StartMetaGame(2, -1);
		for (int anEntryIndex = 0; anEntryIndex < META_ENTRIES; anEntryIndex++) {
			MetaEntry& anEntry = mEntries[anEntryIndex];
			if (anEntry.mBought || !mEntries[anEntry.mPrerequisite].mBought)
				continue;
			MetaOffer& anOffer = mOffers[anOfferCount];
			anOffer.mButton->mAnim = mIconAnims[anEntry.mKind];
			anOffer.mButton->SetStateInt(STATE_FRAME, anEntry.mLevelIndex);
			Activate(anOffer.mButton, false);
			anOffer.mBuilding = anEntry.mKind;
			anOffer.mEntryIndex = anEntryIndex;
			anOffer.mLevel = anEntry.mLevelNumber;
			// the original does not set mEntry here
			int aPrice = mBuildings[anEntry.mKind]->GetPrice(-1);
			int aTrack;
			if (aPrice > Player::sCurrent->mBananas) {
				anOffer.mState = OFFER_TOO_DEAR;
				aTrack = BUTTON_TRACK_DIM;
			} else {
				anOffer.mState = OFFER_AVAILABLE;
				aTrack = BUTTON_TRACK_NORMAL;
			}
			anOffer.mButton->StartTrack(aTrack);
			Player::StartMetaGame(anOfferCount, anEntryIndex);
			anOfferCount++;
			if (anOfferCount > 2)
				break;
		}
	}
	RefreshOffers();
	return anOfferCount < 1;
}

// 0x44a170
void MetaGame::StartLeaveTrack()
{
	StartTrack(mLeaveTrack);
}

// 0x44a180
void MetaGame::StartStartTrack()
{
	StartTrack(mStartTrack);
}

// 0x44a190: the arrow over the first offer that can be bought
void MetaGame::ShowHintArrow()
{
	int i = 0;
	while (mOffers[i].mState != OFFER_AVAILABLE) {
		i++;
		if (i >= META_OFFERS)
			return;
	}
	if (mArrowHint != nullptr) {
		mArrowHint->SetParent(mOffers[i].mButton, nullptr);
		mArrowHint->StartTrack(0);
		Activate(mArrowHint, false);
	}
}

// 0x44a200: the arrow goes (its second track ends in its removal)
void MetaGame::HideHintArrow()
{
	if (mArrowHint != nullptr) {
		mArrowHint->StartTrack(1);
		mArrowHint = nullptr;
	}
}

// 0x44a230: the mouse has left the offers. The original draws a random number it does not use, puts the tag on the
// first offer that can be bought and takes the arrow away, and then ends by taking the tag away too.
void MetaGame::ShowTagOnFirstOffer()
{
	int i = 0;
	while (mOffers[i].mState != OFFER_AVAILABLE) {
		i++;
		if (i >= META_OFFERS) {
			Deactivate(mBuyTag);
			return;
		}
	}
	GameRandomNext();
	mBuyTag->SetParent(mOffers[i].mButton, nullptr);
	mBuyTag->StartTrack(0);
	Activate(mBuyTag, false);
	HideHintArrow();
	Deactivate(mBuyTag);
}

// 0x44a2b0: the tag over the offer the mouse has come to
void MetaGame::ShowBuyTag(ObjBase* theButton)
{
	if (mBuyTag->IsActive() && mBuyTag->mParent == theButton)
		return;
	gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
	mBuyTag->StartTrack(0);
	Activate(mBuyTag, false);
	mBuyTag->SetParent(theButton, nullptr);
	HideHintArrow();
}

// 0x44a330: the two glows behind the button, the second a little smaller, and one colour each. The original reads the
// green of the first from a stack slot it never set; it is white here.
void MetaGame::ShowCoronas(ObjBase* theButton, float theScale)
{
	static const float aRed[2] = {1.0f, 1.0f};
	static const float aGreen[2] = {1.0f, 1.0f};
	static const float aBlue[2] = {0.3f, 1.0f};
	for (int i = 0; i < 2; i++) {
		ObjBase* aCorona = mCoronas[i];
		if (!aCorona->IsActive() || aCorona->mParent != theButton) {
			aCorona->SetParent(theButton, nullptr);
			aCorona->SetState(STATE_Z, (float)(-0.5 - (double)i));
			aCorona->SetState(STATE_RED, aRed[i]);
			aCorona->SetState(STATE_GREEN, aGreen[i]);
			aCorona->SetState(STATE_BLUE, aBlue[i]);
			aCorona->SetState(STATE_SCALE_X, theScale);
			aCorona->SetState(STATE_SCALE_Y, theScale);
			Activate(aCorona, false);
		}
		theScale = (float)((double)theScale * 0.9);
	}
}

// 0x44a440
void MetaGame::HideCoronas()
{
	Deactivate(mCoronas[0]);
	Deactivate(mCoronas[1]);
}

// 0x44a470: the text of the entry on the tip, which shows if it was not
void MetaGame::ShowBuildTip(int theEntry)
{
	MetaEntry& anEntry = mEntries[theEntry];
	bool aWasActive = mBuildTip->IsActive();
	mBuildTip->SetLineText(0, "Build");
	mBuildTip->SetLineText(1, anEntry.mName);
	mBuildTip->SetLineText(3, anEntry.mLines[0]);
	mBuildTip->SetLineText(4, anEntry.mLines[1]);
	mBuildTip->SetLineText(5, anEntry.mLines[2]);
	mBuildTip->SetLineText(6, anEntry.mLines[3]);
	if (!aWasActive) {
		mBuildTip->StartTrack(TIP_TRACK_SHOW);
		Activate(mBuildTip, false);
	}
}

// 0x44a590: the tip's second track, and the tip goes at once (the original does both)
void MetaGame::HideBuildTip()
{
	if (mBuildTip->IsActive()) {
		mBuildTip->StartTrack(TIP_TRACK_HIDE);
		Deactivate(mBuildTip);
	}
}

// 0x44c140: the menu button of the corner
void MetaGame::BuildMenuButton()
{
	mMenuButton = new MenuButton();
	mMenuButton->SetParent(this, nullptr);
	Activate(mMenuButton, false);
}

// 0x44c1d0: the panel of the bananas and the lives, and the numbers on it
void MetaGame::BuildBananaPanel()
{
	mBananaPanel = new ObjBase();
	if (mPanel != nullptr)
		mPanel->ComputeDrawn();
	mBananaPanel->SetParent(this, nullptr);
	mBananaPanel->mAnim = Anim::Get("images/panel_bananas", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mBananaPanel->SetState(STATE_X, mPanel->mBase[STATE_X]);
	mBananaPanel->SetStateInt(STATE_Y, -510);
	mBananaPanel->SetStateInt(STATE_Z, 5);
	mBananaPanel->SetStateInt(STATE_ALPHA, 1);
	mBananaPanel->SetMouseable(32.0f, 32.0f);
	Activate(mBananaPanel, false);

	char aText[16];
	mBananasText = new ObjString();
	mBananasText->SetParent(mBananaPanel, nullptr);
	mBananasText->SetFont(gApp->mFont);
	sprintf(aText, "000000");
	mBananasText->Create(aText, false, 0);
	sprintf(aText, "%d", Player::sCurrent->mBananas);
	mBananasText->SetText(aText);
	mBananasText->SetStateInt(STATE_X, 24);
	mBananasText->SetStateInt(STATE_Y, -32);
	mBananasText->SetStateInt(STATE_Z, 1);
	mBananasText->SetState(STATE_RED, 0.2f);
	mBananasText->SetState(STATE_GREEN, 1.0f);
	mBananasText->SetState(STATE_BLUE, 0.1f);
	mBananasText->SetStateInt(STATE_ALPHA, 1);
	mBananasText->SetState(STATE_SCALE_X, 0.9f);
	mBananasText->SetState(STATE_SCALE_Y, 0.8f);
	Activate(mBananasText, true);
	BuildNumberPulse(mBananasText);

	mLivesText = new ObjString();
	mLivesText->SetParent(mBananaPanel, nullptr);
	mLivesText->SetFont(gApp->mFont);
	sprintf(aText, "000");
	mLivesText->Create(aText, false, 0);
	sprintf(aText, "%d", Player::sCurrent->mLives);
	mLivesText->SetText(aText);
	mLivesText->SetStateInt(STATE_X, 24);
	mLivesText->SetStateInt(STATE_Y, 48);
	mLivesText->SetState(STATE_Z, 1.5f);
	mLivesText->SetStateInt(STATE_RED, 1);
	mLivesText->SetState(STATE_GREEN, 0.2f);
	mLivesText->SetState(STATE_BLUE, 0.1f);
	mLivesText->SetStateInt(STATE_ALPHA, 1);
	mLivesText->SetState(STATE_SCALE_X, 0.9f);
	mLivesText->SetState(STATE_SCALE_Y, 0.9f);
	Activate(mLivesText, true);
	mLivesText->mEmitters = new EmitterSet(mLivesText);
	mLivesText->mEmitters->MakeStarRing(-0.1f, 64);
	BuildNumberPulse(mLivesText);
}

// 0x44c1d0 (twice): the track of one of the numbers, a pulse of its size
void MetaGame::BuildNumberPulse(ObjString* theText)
{
	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(20);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aPeak = new AnimKey();
	aPeak->SetDuration(20);
	aPeak->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aPeak->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimKey* aRest = new AnimKey();
	aRest->SetDuration(10);
	aRest->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrow, nullptr);
	aTrack->AddKey(aPeak, nullptr);
	aTrack->AddKey(aRest, aRest);
	theText->AddTrack(aTrack, nullptr);
}

// 0x44d500: the offers' pictures, and what each entry costs and says
void MetaGame::BuildEntries()
{
	for (int i = 0; i < META_BUILDINGS; i++)
		mIconAnims[i] = Anim::Get(sIconDefs[i].mName, ANIM_SCALE_SMOOTH, false, META_ICON_CEL, META_ICON_CEL,
			sIconDefs[i].mCels, false);
	for (int i = 0; i < META_ENTRIES; i++) {
		MetaEntry& anEntry = mEntries[i];
		anEntry.mKind = sEntryDefs[i].mKind;
		anEntry.mLevelIndex = sEntryDefs[i].mLevelIndex;
		anEntry.mLevelNumber = sEntryDefs[i].mLevelNumber;
		anEntry.mPrerequisite = sEntryDefs[i].mPrerequisite;
		anEntry.mBought = false;
		anEntry.mName = sEntryDefs[i].mName;
		for (int j = 0; j < META_ENTRY_LINES; j++)
			anEntry.mLines[j] = sEntryDefs[i].mLines[j];
	}
	// the one after the last: no kind, always bought, so that an entry with no prerequisite has one
	MetaEntry& aLast = mEntries[META_ENTRIES];
	aLast.mKind = -1;
	aLast.mLevelIndex = -1;
	aLast.mLevelNumber = -1;
	aLast.mPrerequisite = -1;
	aLast.mBought = true;
	aLast.mName = nullptr;
	for (int j = 0; j < META_ENTRY_LINES; j++)
		aLast.mLines[j] = nullptr;
}

// 0x44a5c0: the track of this object that ends the screen: it grows and fades, then the object goes
void MetaGame::BuildLeaveTrack()
{
	AnimKey* aKey1 = new AnimKey();
	aKey1->mEventId = 0;
	aKey1->SetDuration(100);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.9f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEventId = 0;
	aKey2->SetRemoveAtEnd();
	aKey2->SetDuration(20);
	aKey2->AddValue(STATE_SCALE_X, 4.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 4.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetRemoveAtEnd();
	aKey3->SetDuration(1);
	aKey3->AddValue(STATE_SCALE_X, 4.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 4.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	mLeaveTrack = AddTrack(aTrack1, nullptr);
}

// 0x44a810: the track that times the first events: its two keys carry event 1
void MetaGame::BuildStartTrack()
{
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(120);
	aKey1->AddValue(STATE_SCALE_X, 4.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 4.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ROTATION, 0.08726647f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEventId = META_EVENT_START;
	aKey2->SetDuration(50);
	aKey2->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.8f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = META_EVENT_START;
	aKey3->SetDuration(1);
	aKey3->SetRemoveAtEnd();
	aKey3->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->SetRemoveAtEnd();
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, nullptr);
	aTrack1->AddKey(aKey4, aKey4);
	mStartTrack = AddTrack(aTrack1, nullptr);
}

// 0x44ab40: the panel the offers are on, off to the right until the lives have come
void MetaGame::BuildPanel()
{
	MenuObj* aObj1 = new MenuObj();
	mPanel = aObj1;
	mPanel->BuildPanel(5, 9, true);
	mPanel->SetParent(this, nullptr);
	mPanel->SetStateInt(STATE_Y, 22);
	mPanel->SetEventTarget(this);
	mPanel->SetStateInt(STATE_X, 760);
	mPanel->SetStateInt(STATE_Z, 120);
	AnimKey* aKey1 = new AnimKey();
	aKey1->mEventId = 0;
	aKey1->SetDuration(40);
	aKey1->AddValue(STATE_X, 500.0f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEventId = META_EVENT_PANEL_OPEN;
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_X, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(50);
	aKey3->AddValue(STATE_X, 0.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	aTrack1->SetDelay(10, false);
	mPanel->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(10);
	aKey4->AddValue(STATE_X, 500.0f, MOD_HOLD);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, aKey4);
	mPanel->AddTrack(aTrack2, nullptr);
	AnimKey* aKey5 = new AnimKey();
	aKey5->mEventId = 0;
	aKey5->SetDuration(40);
	aKey5->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey5->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey6 = new AnimKey();
	aKey6->mEventId = 0;
	aKey6->SetDuration(50);
	aKey6->AddValue(STATE_X, 500.0f, MOD_HOLD);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey5, nullptr);
	aTrack3->AddKey(aKey6, aKey6);
	aTrack3->SetDelay(10, false);
	mPanel->AddTrack(aTrack3, nullptr);
	mPanel->StartTrack(1);
	mPanel->Deactivate(mPanel);
}

// 0x44aeb0: the "everything is sold" picture over the panel
void MetaGame::BuildSoldOutPanel()
{
	MenuObj* aObj1 = new MenuObj();
	mSoldOutPanel = aObj1;
	mSoldOutPanel->mAnim = mSoldOutAnim;
	mSoldOutPanel->SetParent(mPanel, nullptr);
	mSoldOutPanel->SetStateInt(STATE_X, 0);
	mSoldOutPanel->SetState(STATE_Y, (float)(-(double)mSoldOutPanel->mAnim->GetSourceCelHeight() * 0.5));
	mSoldOutPanel->SetStateInt(STATE_Z, 1);
	mSoldOutPanel->SetStateInt(STATE_SCALE_X, 1);
	mSoldOutPanel->SetStateInt(STATE_SCALE_Y, 0);
	mSoldOutPanel->SetState(STATE_OFFSET_Y, (float)((double)mSoldOutPanel->mAnim->GetSourceCelHeight() * 0.5));
	AnimKey* aKey1 = new AnimKey();
	aKey1->mEventId = 0;
	aKey1->SetDuration(25);
	aKey1->AddValue(STATE_X, (float)((double)mSoldOutPanel->mAnim->GetCelWidth() + 8.0), MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEventId = 0;
	aKey2->SetDuration(10);
	aKey2->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey2->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(50);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	mSoldOutPanel->AddTrack(aTrack1, nullptr);
	this->Deactivate(mSoldOutPanel);
}

// 0x44b180: the done button, below the panel
void MetaGame::BuildDoneButton()
{
	ObjBase* aObj1 = new ObjBase();
	mDoneButton = aObj1;
	aObj1->mAnim = Anim::Get("images/meta/panel_done", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mDoneButton->mButtonId = META_BUTTON_DONE;
	mDoneButton->SetParent(mPanel, nullptr);
	mDoneButton->SetStateInt(STATE_Z, 7);
	mDoneButton->SetState(STATE_Y, 508.0f);
	mDoneButton->SetStateInt(STATE_ALPHA, 0);
	mDoneButton->SetMouseable(32.0f, 32.0f);
	mDoneButton->SetMouseListener(this);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(25);
	aKey1->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ROTATION, -1.3962635f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_CUBE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEndCondition = KEY_SOUND;
	aKey2->mEndChannel = SOUND_DRUM_C;
	aKey2->mEndValue = 2.0f;
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(5);
	aKey3->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->SetDelay(100, false);
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	mDoneButton->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(5);
	aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(2);
	aKey5->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey5);
	mDoneButton->AddTrack(aTrack2, nullptr);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(5);
	aKey6->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey6->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey6->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimKey* aKey7 = new AnimKey();
	aKey7->SetDuration(2);
	aKey7->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey7->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey7->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey6, nullptr);
	aTrack3->AddKey(aKey7, aKey7);
	mDoneButton->AddTrack(aTrack3, nullptr);
	mDoneButton->Activate(mDoneButton, false);
}

// 0x44b890: the casino button, shown once the casino is bought
void MetaGame::BuildCasinoButton()
{
	ObjBase* aObj1 = new ObjBase();
	mCasinoButton = aObj1;
	aObj1->mAnim = Anim::Get("images/meta/meta_casino", ANIM_SCALE_SMOOTH, false, 179, 135, 2, false);
	mCasinoButton->SetParent(this, nullptr);
	mCasinoButton->SetStateInt(STATE_Z, 199);
	mCasinoButton->SetStateInt(STATE_Y, 508);
	mCasinoButton->SetStateInt(STATE_ALPHA, 0);
	mCasinoButton->SetMouseable(32.0f, 32.0f);
	mCasinoButton->SetEventTarget(this);
	mCasinoButton->SetMouseListener(this);
	mCasinoButton->mButtonId = META_BUTTON_CASINO;
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(75);
	aKey1->AddValue(STATE_Y, -500.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, -0.5f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, -0.5f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ROTATION, -3.1415927f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_CUBE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEndCondition = KEY_SOUND;
	aKey2->mEndChannel = SOUND_DRUM_C;
	aKey2->mEndValue = 2.0f;
	aKey2->mEventId = META_EVENT_CASINO_SHOWN;
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(100);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey3->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 5.0f, 2.0f, 0.0f, nullptr);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->SetDelay(100, false);
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey3);
	mCasinoButton->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(5);
	aKey4->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(50);
	aKey5->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey5->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey5->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey5->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 5.0f, 2.0f, 0.0f, nullptr);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, aKey5);
	mCasinoButton->AddTrack(aTrack2, nullptr);
	AnimKey* aKey6 = new AnimKey();
	aKey6->SetDuration(5);
	aKey6->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey6->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey6->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey6->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey6->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	AnimKey* aKey7 = new AnimKey();
	aKey7->SetDuration(50);
	aKey7->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey7->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aKey7->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey7->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey7->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 5.0f, 2.0f, 0.0f, nullptr);
	AnimTrack* aTrack3 = new AnimTrack();
	aTrack3->AddKey(aKey6, nullptr);
	aTrack3->AddKey(aKey7, aKey7);
	mCasinoButton->AddTrack(aTrack3, nullptr);
	mCasinoButton->Deactivate(mCasinoButton);
}

// 0x44c8f0: the first picture, which flies in and out
void MetaGame::BuildIntro()
{
	ObjBase* aObj1 = new ObjBase();
	mIntro = aObj1;
	aObj1->SetParent(this, nullptr);
	aObj1->mAnim = Anim::Get("images/meta/meta_intro", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mIntro->SetStateInt(STATE_X, 0);
	mIntro->SetStateInt(STATE_Y, -280);
	mIntro->SetStateInt(STATE_Z, 100);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(15);
	aKey1->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_Y, -700.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, aKey2);
	mIntro->AddTrack(aTrack1, nullptr);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(12);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_Y, -700.0f, MOD_HOLD);
	aKey4->SetRemoveAtEnd();
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey3, nullptr);
	aTrack2->AddKey(aKey4, nullptr);
	mIntro->AddTrack(aTrack2, nullptr);
	mIntro->Deactivate(mIntro);
}

// 0x44cbc0: the three offers: a picture of the building, the sold-out picture and the price on it
void MetaGame::BuildOffers()
{
	int anOfferY = -245;
	for (int i = 0; anOfferY < 490; i++, anOfferY += 245) {
		ObjBase* aButton = new ObjBase();
		MetaOffer& anOffer = mOffers[i];
		anOffer.mEntryIndex = 0;
		anOffer.mEntry = nullptr; // port: the original leaves it unset until the offers are chosen (nothing reads it)
		anOffer.mBuilding = 0;
		anOffer.mState = OFFER_AVAILABLE;
		anOffer.mButton = aButton;
		aButton->mButtonId = i + 1;
		aButton->SetParent(mPanel, nullptr);
		aButton->SetStateInt(STATE_X, -16);
		aButton->SetStateInt(STATE_Y, anOfferY);
		aButton->SetStateInt(STATE_Z, 3);
		aButton->SetStateInt(STATE_SCALE_X, 0);
		aButton->SetStateInt(STATE_SCALE_Y, 0);
		aButton->SetStateInt(STATE_ALPHA, 0);
		aButton->SetMouseable(32.0f, 32.0f);
		aButton->SetMouseListener(this);
		AnimKey* aKey1 = new AnimKey();
		aKey1->SetDuration(20);
		aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aKey1->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
		aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aKey1->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
		aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey2 = new AnimKey();
		aKey2->SetDuration(40);
		aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey3 = new AnimKey();
		aKey3->SetDuration(10);
		aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		AnimTrack* aTrack1 = new AnimTrack();
		aTrack1->AddKey(aKey1, nullptr);
		aTrack1->AddKey(aKey2, nullptr);
		aTrack1->AddKey(aKey3, aKey3);
		aButton->AddTrack(aTrack1, nullptr);
		AnimKey* aKey4 = new AnimKey();
		aKey4->SetDuration(30);
		aKey4->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey5 = new AnimKey();
		aKey5->SetDuration(10);
		aKey5->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aKey5->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aKey5->AddValue(STATE_RED, -0.5f, MOD_HOLD);
		aKey5->AddValue(STATE_GREEN, -0.5f, MOD_HOLD);
		aKey5->AddValue(STATE_BLUE, -0.5f, MOD_HOLD);
		aKey5->AddValue(STATE_ALPHA, 0.9f, MOD_HOLD);
		AnimTrack* aTrack2 = new AnimTrack();
		aTrack2->AddKey(aKey4, nullptr);
		aTrack2->AddKey(aKey5, aKey5);
		aButton->AddTrack(aTrack2, nullptr);
		ObjBase* aSoldOut = new ObjBase();
		anOffer.mSoldOut = aSoldOut;
		aSoldOut->mAnim = Anim::Get("images/meta/meta_icon_sold", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
		aSoldOut->SetParent(aButton, nullptr);
		aSoldOut->SetStateInt(STATE_X, 0);
		aSoldOut->SetStateInt(STATE_Y, 0);
		aSoldOut->SetStateInt(STATE_Z, 1);
		aSoldOut->SetStateInt(STATE_SCALE_X, 0);
		aSoldOut->SetStateInt(STATE_SCALE_Y, 0);
		aSoldOut->SetStateInt(STATE_ALPHA, 0);
		AnimKey* aKey6 = new AnimKey();
		aKey6->SetDuration(10);
		aKey6->AddValue(STATE_SCALE_X, 3.3f, MOD_TOWARD_NEXT);
		aKey6->AddValue(STATE_SCALE_Y, 3.3f, MOD_TOWARD_NEXT);
		aKey6->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey7 = new AnimKey();
		aKey7->SetDuration(2);
		aKey7->AddValue(STATE_SCALE_X, 0.9f, MOD_TOWARD_NEXT);
		aKey7->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);
		aKey7->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey8 = new AnimKey();
		aKey8->SetDuration(10);
		aKey8->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aKey8->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aKey8->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		AnimTrack* aTrack3 = new AnimTrack();
		aTrack3->AddKey(aKey6, nullptr);
		aTrack3->AddKey(aKey7, nullptr);
		aTrack3->AddKey(aKey8, aKey8);
		aSoldOut->AddTrack(aTrack3, nullptr);
		Deactivate(aSoldOut);
		ObjString* aPriceText = new ObjString();
		anOffer.mPrice = aPriceText;
		aPriceText->SetParent(aButton, nullptr);
		aPriceText->SetFont(gApp->mFont);
		aPriceText->SetState(STATE_SCALE_X, 0.6f);
		aPriceText->SetState(STATE_SCALE_Y, 0.8f);
		aPriceText->SetStateInt(STATE_X, 50);
		aPriceText->SetStateInt(STATE_Y, 50);
		aPriceText->SetState(STATE_RED, 1.0f);
		aPriceText->SetState(STATE_GREEN, 0.9f);
		aPriceText->SetState(STATE_BLUE, 0.1f);
		aPriceText->SetStateInt(STATE_ALPHA, 1);
		aPriceText->SetState(STATE_Z, 1.0f);
		aPriceText->Create("00000", true, 0);
		aPriceText->Activate(aPriceText, false);
	}
}

// 0x44df40: the paths the lives fly in on
void MetaGame::BuildLifeIcons()
{
	mPathTrace = new PathTrace();
	mPathTrace->Load("images/meta/meta_livespath");
	for (int i = 0; i < META_LIFE_ICONS; i++) {
		ObjBase* aIcon = new ObjBase();
		mLifeIcons[i] = aIcon;
		mLifeAnim->AddRef();
		aIcon->mAnim = mLifeAnim;
		aIcon->SetParent(this, nullptr);
		aIcon->SetStateInt(STATE_X, -690);
		aIcon->SetStateInt(STATE_Y, 200);
		aIcon->SetStateInt(STATE_SCALE_X, 0);
		aIcon->SetStateInt(STATE_SCALE_Y, 0);
		aIcon->SetStateInt(STATE_ALPHA, 0);
		aIcon->SetStateInt(STATE_Z, 200);
		aIcon->SetEventTarget(this);
		aIcon->mEmitters = new EmitterSet(aIcon);
		aIcon->mEmitters->MakeStarRing(-0.1f, 65);
		AnimKey* aKey1 = new AnimKey();
		aKey1->SetDuration(8);
		aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey2 = new AnimKey();
		aKey2->SetDuration(40);
		aKey2->mEventId = META_EVENT_STAR_BURST;
		aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aKey2->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
		aKey2->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
		AnimKey* aKey3 = new AnimKey();
		aKey3->SetDuration(5);
		aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aKey3->AddValue(STATE_SCALE_X, 1.5f, MOD_TOWARD_NEXT);
		aKey3->AddValue(STATE_SCALE_Y, 1.5f, MOD_TOWARD_NEXT);
		AnimKey* aKey4 = new AnimKey();
		aKey4->SetDuration(120);
		aKey4->mEventId = META_EVENT_STAR_BURST;
		aKey4->AddModifier(STATE_X, false, -690.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, mPathTrace->mX);
		aKey4->AddModifier(STATE_Y, false, 200.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, mPathTrace->mY);
		aKey4->AddValue(STATE_PATH, 0.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_SCALE_X, 1.3f, MOD_TOWARD_NEXT);
		aKey4->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
		aKey4->mEasing = EASE_IN_SQUARE;
		AnimKey* aKey5 = new AnimKey();
		aKey5->SetDuration(1);
		aKey5->mEventId = META_EVENT_LIFE_ARRIVED;
		aKey5->AddValue(STATE_PATH, (float)(mPathTrace->mCount - 1), MOD_HOLD);
		aKey5->AddValue(STATE_ALPHA, 0.5f, MOD_HOLD);
		aKey5->AddValue(STATE_SCALE_X, 0.1f, MOD_HOLD);
		aKey5->AddValue(STATE_SCALE_Y, 0.3f, MOD_HOLD);
		AnimTrack* aTrack1 = new AnimTrack();
		aTrack1->AddKey(aKey1, nullptr);
		aTrack1->AddKey(aKey2, nullptr);
		aTrack1->AddKey(aKey3, nullptr);
		aTrack1->AddKey(aKey4, nullptr);
		aTrack1->AddKey(aKey5, nullptr);
		aIcon->AddTrack(aTrack1, nullptr);
	}
}

// 0x44e4c0: the two glows behind a hovered offer
void MetaGame::BuildCoronas()
{
	for (int i = 0; i < 2; i++) {
		ObjBase* aCorona = new ObjBase();
		mCoronas[i] = aCorona;
		mCoronaAnim->AddRef();
		aCorona->mAnim = mCoronaAnim;
		aCorona->SetStateInt(STATE_X, 0);
		aCorona->SetStateInt(STATE_Y, 0);
		aCorona->SetStateInt(STATE_Z, 0);
		aCorona->SetStateInt(STATE_RED, 1);
		aCorona->SetStateInt(STATE_GREEN, 1);
		aCorona->SetStateInt(STATE_BLUE, 1);
		aCorona->SetState(STATE_ALPHA, 0.6f);
		AnimKey* aKey1 = new AnimKey();
		aKey1->SetDuration(360);
		aKey1->mDrawMode = 1;
		aKey1->AddModifier(STATE_ROTATION, false, 0.0f, (i == 0 ? MOD_WRAP_DEGREES : MOD_SPIN), 1.0f, 1.0f, 0.0f, nullptr);
		AnimTrack* aTrack1 = new AnimTrack();
		aTrack1->AddKey(aKey1, aKey1);
		aCorona->AddTrack(aTrack1, nullptr);
		Deactivate(aCorona);
	}
}

// 0x44e640: the text of the offer the mouse is on
void MetaGame::BuildBuildTip()
{
	MenuObj* aObj1 = new MenuObj();
	mBuildTip = aObj1;
	aObj1->SetParent(this, nullptr);
	mBuildTip->BuildPanel(7, 5, false);
	mBuildTip->SetState(STATE_Z, 200.0f);
	mBuildTip->SetState(STATE_ALPHA, 0.0f);
	mBuildTip->SetState(STATE_SCALE_X, 0.0f);
	mBuildTip->SetState(STATE_SCALE_Y, 0.0f);
	mBuildTip->BuildLines(gApp->mFont, false);
	mBuildTip->SetLineColor(0, 1.0f, 0.8f, 0.0f, 1.0f);
	mBuildTip->SetLineScale(0, 1.0f, 1.0f);
	mBuildTip->MoveLines(0, 16.0f, false);
	mBuildTip->SetLineColor(1, 0.9f, 0.9f, 0.0f, 1.0f);
	mBuildTip->SetLineScale(1, 0.8f, 0.8f);
	mBuildTip->MoveLines(1, 30.0f, true);
	mBuildTip->SetLineScale(2, 0.7f, 0.7f);
	mBuildTip->MoveLines(2, 8.0f, true);
	mBuildTip->SetLineScale(3, 0.7f, 0.7f);
	mBuildTip->MoveLines(3, 8.0f, true);
	mBuildTip->SetLineScale(4, 0.7f, 0.7f);
	mBuildTip->MoveLines(4, 8.0f, true);
	mBuildTip->SetLineScale(5, 0.7f, 0.7f);
	mBuildTip->MoveLines(5, 8.0f, true);
	mBuildTip->SetLineScale(6, 0.7f, 0.7f);
	mBuildTip->MoveLines(6, 8.0f, true);
	AnimKey* aKey1 = new AnimKey();
	aKey1->mEventId = 0;
	aKey1->SetDuration(15);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->mEventId = 0;
	aKey2->SetDuration(50);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, aKey2);
	mBuildTip->AddTrack(aTrack1, nullptr);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mEventId = 0;
	aKey3->SetDuration(10);
	aKey3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey3->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey4 = new AnimKey();
	aKey4->mEventId = 0;
	aKey4->SetDuration(1);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.9f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_Y, 0.3f, MOD_HOLD);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey3, nullptr);
	aTrack2->AddKey(aKey4, nullptr);
	mBuildTip->AddTrack(aTrack2, nullptr);
	mBuildTip->Deactivate(mBuildTip);
}

// 0x44ebf0: the tag over the offer the mouse is on
void MetaGame::BuildBuyTag()
{
	ObjBase* aObj1 = new ObjBase();
	mBuyTag = aObj1;
	aObj1->mAnim = Anim::Get("images/meta/meta_pricetags", ANIM_SCALE_SMOOTH, false, 220, 100, 6, false);
	mBuyTag->SetParent(mPanel, nullptr);
	mBuyTag->SetStateInt(STATE_X, -256);
	mBuyTag->SetStateInt(STATE_Z, 5);
	mBuyTag->SetStateInt(STATE_ALPHA, 0);
	mBuyTag->SetStateInt(STATE_FRAME, 1);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(50);
	aKey1->AddValue(STATE_X, -128.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(150);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_X, -10.0f, MOD_TOWARD_NEXT);
	aKey2->mEasing = EASE_COSINE;
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(150);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->mEasing = EASE_COSINE;
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey2);
	mBuyTag->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(10);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey4->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(1);
	aKey5->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, nullptr);
	mBuyTag->AddTrack(aTrack2, nullptr);
	ObjString* aObj2 = new ObjString();
	mBuyTagText = aObj2;
	aObj2->SetParent(mBuyTag, nullptr);
	aObj2->SetFont(gApp->mFont);
	aObj2->Create("BUY", true, 0);
	aObj2->SetStateInt(STATE_X, -28);
	aObj2->SetStateInt(STATE_Y, -8);
	aObj2->SetState(STATE_RED, 0.2f);
	aObj2->SetState(STATE_GREEN, 1.0f);
	aObj2->SetState(STATE_BLUE, 0.1f);
	aObj2->SetState(STATE_Z, 1.0f);
	aObj2->SetState(STATE_SCALE_X, 0.6f);
	aObj2->SetState(STATE_SCALE_Y, 0.8f);
	aObj2->Activate(aObj2, true);
}

// 0x44f080: the arrow that points at the first offer
void MetaGame::BuildArrowHint()
{
	ObjBase* aObj1 = new ObjBase();
	mArrowHint = aObj1;
	mArrowAnim->AddRef();
	aObj1->mAnim = mArrowAnim;
	mArrowHint->SetParent(mPanel, nullptr);
	mArrowHint->SetStateInt(STATE_X, -236);
	mArrowHint->SetStateInt(STATE_Z, 5);
	mArrowHint->SetStateInt(STATE_ALPHA, 0);
	mArrowHint->SetStateInt(STATE_FRAME, 0);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(50);
	aKey1->AddValue(STATE_X, -128.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey1->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(150);
	aKey2->AddValue(STATE_ALPHA, 0.9f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_X, -20.0f, MOD_TOWARD_NEXT);
	aKey2->mEasing = EASE_COSINE;
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(150);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->mEasing = EASE_COSINE;
	AnimTrack* aTrack1 = new AnimTrack();
	aTrack1->AddKey(aKey1, nullptr);
	aTrack1->AddKey(aKey2, nullptr);
	aTrack1->AddKey(aKey3, aKey2);
	mArrowHint->AddTrack(aTrack1, nullptr);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(10);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey4->mEasing = EASE_IN_SQUARE;
	AnimKey* aKey5 = new AnimKey();
	aKey5->SetDuration(1);
	aKey5->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aKey5->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	aKey5->SetRemoveAtEnd();
	AnimTrack* aTrack2 = new AnimTrack();
	aTrack2->AddKey(aKey4, nullptr);
	aTrack2->AddKey(aKey5, nullptr);
	mArrowHint->AddTrack(aTrack2, nullptr);
}
