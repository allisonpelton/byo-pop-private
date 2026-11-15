#include "SavedGames.h"

#include "Application.h"
#include "Avatar.h"
#include "ChallengeScreen.h"
#include "Gameplay.h"
#include "Object.h"
#include "Sticker.h"
#include "Timer.h"
#include "UserManager.h"
#include "ValueList.h"

using namespace Sexy;

// 0x469300: the starting point's numbers: nothing collected, 5 health out of 5, no hammer. (The original sets only these and the
// mode and level; the rest of the numbers are what the memory held.)
RestartPoint::RestartPoint() :
	mMode(0),
	mLevel(0),
	mBonusRounds(0)
{
	for (int i = 0; i < Stats::NUM_SAVED_STATS; i++)
		mStats[i] = 0.0;
	mStats[1] = 5.0;
	mStats[2] = 5.0;
}

// 0x4696a0: the text is what ToString makes: five numbers, the mode, the level and the bonus rounds, twenty numbers
RestartPoint::RestartPoint(const std::string& theState) :
	mMode(0),
	mLevel(0),
	mBonusRounds(0)
{
	ValueList aList(theState);
	for (int i = 0; i < 5; i++)
		mStats[i] = aList.NextDouble();
	mMode = aList.NextInt();
	mLevel = aList.NextInt();
	mBonusRounds = aList.NextInt();
	for (int i = 5; i < Stats::NUM_SAVED_STATS; i++)
		mStats[i] = aList.NextDouble();
}

// 0x469350
void RestartPoint::Capture()
{
	GetStats()->GetSaved(mStats);
	mMode = gApp->mGameMode;
	mLevel = gApp->mGameScreen->mLevel;
	mBonusRounds = gApp->mGameScreen->mBonusRoundsDone;
}

// 0x4693b0
void RestartPoint::Restore() const
{
	GetStats()->SetSaved(mStats);
	GameScreen* aGameScreen = gApp->mGameScreen;
	aGameScreen->SetLevel(mLevel + 1, 0);
	aGameScreen->mBonusRoundsDone = mBonusRounds;
	aGameScreen->Start(mMode);
}

// 0x469420
std::string RestartPoint::ToString() const
{
	ValueList aList;
	for (int i = 0; i < 5; i++)
		aList.Add(mStats[i]);
	aList.AddInt(mMode);
	aList.AddInt(mLevel);
	aList.AddInt(mBonusRounds);
	for (int i = 5; i < Stats::NUM_SAVED_STATS; i++)
		aList.Add(mStats[i]);
	return aList.ToString();
}

// 0x46d440
SavedGame::SavedGame() :
	mVersion(0),
	mState(0),
	mLevel(0),
	mTime(0.0),
	mBonusRounds(0),
	mCycle(0),
	mTimerTotal(0.0),
	mTimerLeft(0.0),
	mTimerRate(0.0),
	mTimerPhase(0),
	mInLevelIntro(false),
	mGameplayPhase(0),
	mPhaseTimer(0.0),
	mNextPopTimer(0.0),
	mCloverTimer(0.0),
	mStrawberryTimer(0.0),
	mDoubleHitTimer(0.0),
	mFiveTimesTimer(0.0),
	mPopNow(false),
	mFirstCubbyStarted(false),
	mSaveItemHasBeenCreated(false),
	mMustCreateSaveItem(false),
	mFlagCC(false),
	mClicksOnLevel(0),
	mKindIndex(0),
	mInShop(false),
	mInChallenge(false),
	mInChallengeExplain(false)
{
	for (int i = 0; i < GameScreen::NUM_SHUFFLED_BOARDS; i++)
		mBoardOrder[i] = 0;
}

// 0x46d620: the game on screen goes into the saved game of its mode
void User::StoreGameSnapshot()
{
	GameScreen* aGameScreen = gApp->mGameScreen;
	SavedGame& aSaved = GetSavedGame(aGameScreen->mMode);

	aSaved.mVersion = SAVED_GAME_VERSION;
	aSaved.mLevel = aGameScreen->mLevel;
	aSaved.mTime = aGameScreen->mLevelTime;
	aSaved.mBonusRounds = aGameScreen->mBonusRoundsDone;
	for (int i = 0; i < GameScreen::NUM_SHUFFLED_BOARDS; i++)
		aSaved.mBoardOrder[i] = aGameScreen->mShuffledBoards[i];
	aSaved.mCycle = aGameScreen->mCycle;
	aSaved.mStats = GetStats()->mValues;

	aSaved.mCubbies.clear();
	int anIndex = 0;
	for (Cubby* aCubby = ObjectManager::Get()->FindByType<Cubby>(&anIndex); aCubby != nullptr; aCubby = ObjectManager::Get()->FindByType<Cubby>(&anIndex))
	{
		CubbyState aState;
		aCubby->SaveState(aState);
		aSaved.mCubbies.push_back(aState);
		anIndex++;
	}

	// port: the clock and the avatar are there in every game on screen; a missing one leaves the old values
	Timer* aTimer = GetTimer();
	if (aTimer != nullptr)
	{
		aSaved.mTimerTotal = aTimer->mTotal;
		aSaved.mTimerLeft = aTimer->mLeft;
		aSaved.mTimerRate = aTimer->mRate;
		aSaved.mTimerPhase = aTimer->mPhase;
	}
	aSaved.mState = 3;
	aSaved.mInLevelIntro = gApp->mLevelIntro != nullptr;

	Gameplay* aGameplay = GetGameplay();
	aSaved.mGameplayPhase = aGameplay->mPhase;
	aSaved.mPhaseTimer = aGameplay->mPhaseTimer;
	aSaved.mNextPopTimer = aGameplay->mNextPopTimer;
	aSaved.mCloverTimer = aGameplay->mCloverTimer;
	aSaved.mStrawberryTimer = aGameplay->mStrawberryTimer;
	aSaved.mDoubleHitTimer = aGameplay->mDoubleHitTimer;
	aSaved.mFiveTimesTimer = aGameplay->mFiveTimesTimer;
	aSaved.mPopNow = aGameplay->mPopNow;
	aSaved.mFirstCubbyStarted = aGameplay->mFirstCubbyStarted;
	aSaved.mSaveItemHasBeenCreated = aGameplay->mChallengeDone;
	aSaved.mMustCreateSaveItem = aGameplay->mChallengeShown;
	aSaved.mFlagCC = aGameplay->mFlagCC;

	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		aSaved.mClicksOnLevel = anAvatar->mClickCount;

	aSaved.mBouncing.clear();
	anIndex = 0;
	for (GnomeBouncing* aBouncing = ObjectManager::Get()->FindByType<GnomeBouncing>(&anIndex); aBouncing != nullptr; aBouncing = ObjectManager::Get()->FindByType<GnomeBouncing>(&anIndex))
	{
		BouncingState aState;
		aBouncing->SaveState(aState);
		aSaved.mBouncing.push_back(aState);
		anIndex++;
	}

	aSaved.mFlying.clear();
	anIndex = 0;
	for (GnomeFlying* aFlying = ObjectManager::Get()->FindByType<GnomeFlying>(&anIndex); aFlying != nullptr; aFlying = ObjectManager::Get()->FindByType<GnomeFlying>(&anIndex))
	{
		FlyingState aState;
		aFlying->SaveState(aState);
		aSaved.mFlying.push_back(aState);
		anIndex++;
	}

	aSaved.mStickers.clear();
	anIndex = 0;
	for (Sticker* aSticker = ObjectManager::Get()->FindByType<Sticker>(&anIndex); aSticker != nullptr; aSticker = ObjectManager::Get()->FindByType<Sticker>(&anIndex))
	{
		StickerState aState;						// 0x4435f0
		aState.mName = aSticker->mName;
		aState.mHit = aSticker->mHit;
		aSaved.mStickers.push_back(aState);
		anIndex++;
	}

	aSaved.mKindList = aGameScreen->mKindList;
	aSaved.mKindIndex = aGameScreen->mKindIndex;
	aSaved.mInShop = gApp->IsShopOpen(true);
	aSaved.mInChallenge = gApp->IsChallengeOpen(true);
	aSaved.mInChallengeExplain = false;
	if (aSaved.mInChallenge && gApp->mChallengeScreen != nullptr)
	{
		aSaved.mChallengeState = gApp->mChallengeScreen->SaveExtra();
		aSaved.mInChallengeExplain = gApp->IsBonusRoundOpen();
	}

	// The original goes on: with stat 0x28 set and no licence (a trial's own check, 0x41e510) it also drops the game. The port has
	// only the licensed game.
	if (GetStats()->GetInt(STAT_KINGS_DESTROYED) != 0)
		aSaved.mState = 0;
}

// 0x46de60: the saved game of the mode being played goes into the game screen (which Application::CreateGameScreen has just made)
void User::LoadSavedGame()
{
	int aMode = gApp->mGameMode;
	SavedGame& aSaved = GetSavedGame(aMode);
	if (aSaved.mState == 0)
		return;

	GameScreen* aGameScreen = gApp->mGameScreen;
	aGameScreen->SetLevel(aSaved.mLevel, aSaved.mCycle);
	aGameScreen->mLevelTime = aSaved.mTime;
	aGameScreen->mBonusRoundsDone = aSaved.mBonusRounds;
	for (int i = 0; i < GameScreen::NUM_SHUFFLED_BOARDS; i++)
		aGameScreen->mShuffledBoards[i] = aSaved.mBoardOrder[i];
	aGameScreen->Start(aMode);
	GetStats()->mValues = aSaved.mStats;				// 0x42ef00

	if (aSaved.mInShop)
		gApp->OpenShopScreen();
	if (aSaved.mInChallenge)
	{
		gApp->OpenChallengeScreen();
		std::string aChallengeState = aSaved.mChallengeState;
		gApp->mChallengeScreen->LoadExtra(aChallengeState);
		if (aSaved.mInChallengeExplain)
			gApp->DoBonusRoundDialog();
	}

	// the cubbies of the board that Start made are found by their names
	for (size_t i = 0; i < aSaved.mCubbies.size(); i++)
	{
		Cubby* aCubby = dynamic_cast<Cubby*>(ObjectManager::Get()->FindByName(aSaved.mCubbies[i].mName));
		if (aCubby != nullptr)
			aCubby->LoadState(aSaved.mCubbies[i]);
	}

	Timer* aTimer = GetTimer();
	if (aTimer != nullptr)
	{
		aTimer->mTotal = aSaved.mTimerTotal;
		aTimer->mLeft = aSaved.mTimerLeft;
		aTimer->mRate = aSaved.mTimerRate;
		aTimer->mPhase = aSaved.mTimerPhase;
	}
	aSaved.mState = 0;

	Gameplay* aGameplay = GetGameplay();
	aGameplay->mPhase = aSaved.mGameplayPhase;
	aGameplay->mPhaseTimer = aSaved.mPhaseTimer;
	aGameplay->mNextPopTimer = aSaved.mNextPopTimer;
	aGameplay->mCloverTimer = aSaved.mCloverTimer;
	aGameplay->mStrawberryTimer = aSaved.mStrawberryTimer;
	aGameplay->mDoubleHitTimer = aSaved.mDoubleHitTimer;
	aGameplay->mFiveTimesTimer = aSaved.mFiveTimesTimer;
	aGameplay->mPopNow = aSaved.mPopNow;
	aGameplay->mFirstCubbyStarted = aSaved.mFirstCubbyStarted;
	aGameplay->mChallengeDone = aSaved.mSaveItemHasBeenCreated;
	aGameplay->mChallengeShown = aSaved.mMustCreateSaveItem;
	aGameplay->mFlagCC = aSaved.mFlagCC;

	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		anAvatar->mClickCount = aSaved.mClicksOnLevel;

	// the jesters and the aviators are made again; one whose name is taken is made with a blank name, and the saved name is blanked too
	for (size_t i = 0; i < aSaved.mBouncing.size(); i++)
	{
		BouncingState& aState = aSaved.mBouncing[i];
		std::string aName = aState.mName;
		if (ObjectManager::Get()->FindByName(aName) != nullptr)
		{
			aName = "";
			aState.mName = "";
		}
		MakeGnomeBouncing(aName)->LoadState(aState);
	}
	for (size_t i = 0; i < aSaved.mFlying.size(); i++)
	{
		FlyingState& aState = aSaved.mFlying[i];
		std::string aName = aState.mName;
		if (ObjectManager::Get()->FindByName(aName) != nullptr)
		{
			aName = "";
			aState.mName = "";
		}
		MakeGnomeFlying(aName)->LoadState(aState);
	}

	// the stickers of the board are found by their names; a struck one is gone again (0x443120)
	for (size_t i = 0; i < aSaved.mStickers.size(); i++)
	{
		Sticker* aSticker = dynamic_cast<Sticker*>(ObjectManager::Get()->FindByName(aSaved.mStickers[i].mName));
		if (aSticker != nullptr)
			aSticker->SetHit(aSaved.mStickers[i].mHit);
	}

	aGameScreen->mKindList = aSaved.mKindList;
	aGameScreen->mKindIndex = aSaved.mKindIndex;

	// what is on screen: the level's introduction, or the shop or challenge as they were, or the "Ready?" box
	if (!aSaved.mInLevelIntro)
	{
		if (!aSaved.mInShop)
		{
			if (aSaved.mInChallenge)
			{
				if (gApp->GetDialog(DIALOG_CHALLENGE_FINISHED) != nullptr)
					return;
				if (gApp->IsBonusRoundOpen())
					return;
			}
			gApp->DoReadyGoDialog(true, "Ready?", "");
		}
	}
	else
		gApp->CreateLevelIntro();
}
