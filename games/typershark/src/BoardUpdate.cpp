// BoardUpdate.cpp: Board::Update (the per-tick logic of the game screen) and the small helpers that only it (and the event
// loops) call (job B1). Update moves the diver down the ocean, ages the popups and timers, spawns waves, updates every
// creature list through its event loop (BoardEvents.cpp), and decides when the diver is eaten (PlayerEaten).
//
// Left out of Update (registration, removed): the call at 0x45c18d to the app's trial play-time accounting (0x41d9d0), and the
// block at 0x45ca88 to 0x45cb0f that stopped an unregistered abyss game at a depth limit (Board +0x150). The continue link
// (id 300, mContinueLink) only returns to the map.
//
// The lists hold the Enemy part of each creature (Board.h); a Shark, Piranha or the black and red shark variants are
// reached with dynamic_cast, the way the original reads the virtual-base offset. The original's inlined
// vector<T*>::erase loops ("copy the tail down by one") are std::vector::erase, and the deleting-destructor call through
// the GameObject part is a plain delete.
//
// What the dialog class (job D1) must offer, as PlayerEaten uses it: GameOverDialog(WinWSApp*, Player*, bool theEndless, int,
// int, int, int theScore) (ctor 0x43adb0), the int mField11c (+0x11c), the std::string mRankTitle (+0x128) and the int
// mField144 (+0x144, set only in an endless game). HudBar (job B2) must offer Update() (0x441760). StatusBar (job W1) must
// offer HideButtons() (0x441f10: hides its buttons and sends the widget at +0x68 to the back) and ShowButton(bool)
// (0x441f80: shows or hides that one widget, in front or at the back).

#include "Board.h"
#include "Enemy.h"
#include "Shark.h"
#include "Piranha.h"
#include "BlackShark.h"
#include "RedShark.h"
#include "Clam.h"
#include "BonusCreature.h"
#include "ProjectileBoss.h"
#include "Projectile.h"
#include "Bubble.h"
#include "BonusText.h"
#include "BackgroundFish.h"
#include "Fixes.h"
#include "HudBar.h"
#include "StatusBar.h"
#include "GameOverDialog.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"
#include "Image.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x43fd30
bool Board::IsBelowSurface()
{
	if (mLessonManager->LastWaveIsBoss() && mPlayer->mFlag118 == false && mPlayer->mState == 0)
	{
		if ((double)mSurfaceHeight < mDivePos)
		{
			if (!mLessonManager->IsEndless())
				return true;
		}
	}
	return false;
}

// 0x43fd90: the percent of the letters typed right (mField0f60 right, mField0f64 wrong) picks the popup: a bronze, silver or
// gold medal; from 95 percent up the popup also pays a bonus that grows with the streak, the diver's level and the game's
// difficulty (or, in an abyss game, the depth)
void Board::ShowRating()
{
	int aPercent = (int)((double)mField0f60 / ((double)mField0f64 + (double)mField0f60) * 100.0);
	RatingPopup* aRating = mRating;
	if (aPercent < 0x5f)
	{
		aRating->mStreak = 0;
		mRating->mBonus = 0;
	}
	else
		aRating->mStreak++;
	if (aPercent == 0)
		return;
	if (mRating->mPoints == 0)
		return;

	mRating->mPercent = aPercent;
	mRating->mActive = true;
	mRating->mAlpha = 255;
	mRating->mX = 0.0;		// the original stores two zero dwords at +0x10 and +0x14
	mRating->mY = (double)(mFloorY - 0x32);
	if (aPercent < 0x50)
	{
		mRating->mImage = mApp->mImages[IMAGE_MEDAL_BRONZE];
		mApp->PlaySample(SOUND_WAVE_BONUS);
		return;
	}
	if (aPercent < 0x5f)
	{
		mRating->mImage = mApp->mImages[IMAGE_MEDAL_SILVER];
		mApp->PlaySample(SOUND_WAVE_BONUS2);
		return;
	}
	mRating->mImage = mApp->mImages[IMAGE_MEDAL_GOLD];
	mApp->PlaySample(SOUND_WAVE_BONUS3);

	RatingPopup* aPopup = mRating;
	int aStreak = aPopup->mStreak;
	if (aStreak > 10)
		aStreak = 10;
	int aLevelFactor = mPlayer->mField114;
	if (aLevelFactor > 10)
		aLevelFactor = 10;
	else
		aLevelFactor = aLevelFactor + 1;
	int aDifficultyFactor = mApp->mDifficulty;
	if (aDifficultyFactor < 0)
	{
		// an abyss game (difficulty -1): the depth in feet takes its place
		if (mScore < 0x3e8)
			aDifficultyFactor = 0;
		else if (mScore < 0x6d6)
			aDifficultyFactor = 1;
		else if (mScore < 0x9c4)
			aDifficultyFactor = 2;
		else
			aDifficultyFactor = 3 + (mScore >= 0xcb2 ? 1 : 0);
	}
	int aStreakBonus = aLevelFactor * aStreak * 75;
	int aDifficultyBonus = aDifficultyFactor * 200;
	int aBonus = (int)((double)aPercent * 0.01 * (double)aPopup->mPoints + (double)aStreakBonus + (double)aDifficultyBonus);
	if (aBonus > 20000)
		aBonus = 20000;
	aPopup->mBonus = aBonus;
	mPlayer->AddScore(aBonus);
}

// 0x4402f0
void Board::ResetPiranhaSpeed()
{
	mPiranhaSpeed = 1.0;
	mField0e94 = 0;
}

// 0x4408d0: four pairs of offsets (a creature's spot on the treasure picture?) for each treasure of the game (10 to 15)
void Board::GetSpotOffset(int* theX, int* theY, int theIndex)
{
	switch (mApp->GetTreasureIndex())
	{
	case 10:
		break;
	case 0xb:
		if (theIndex == 0)
		{
			*theX = 4;
			*theY = -13;
			return;
		}
		if (theIndex == 1)
		{
			*theX = -14;
			*theY = 4;
			return;
		}
		if (theIndex == 2)
		{
			*theX = 21;
			*theY = 2;
			return;
		}
		*theX = 3;
		*theY = 20;
		return;
	default:
		if (theIndex == 0)
		{
			*theX = -9;
			*theY = -10;
			return;
		}
		if (theIndex == 1)
		{
			*theX = 16;
			*theY = -11;
			return;
		}
		*theX = 3;
		*theY = 3;
		return;
	case 0xd:
		if (theIndex == 0)
		{
			*theX = -11;
			*theY = -11;
			return;
		}
		if (theIndex == 1)
		{
			*theX = 19;
			*theY = -11;
			return;
		}
		if (theIndex == 2)
		{
			*theX = -11;
			*theY = 19;
			return;
		}
		*theX = 19;
		*theY = 19;
		return;
	case 0xe:
		if (theIndex == 0)
		{
			*theX = 4;
			*theY = -11;
			return;
		}
		if (theIndex == 1)
		{
			*theX = 18;
			*theY = 3;
			return;
		}
		if (theIndex == 2)
		{
			*theX = 4;
			*theY = 19;
			return;
		}
		*theX = -12;
		*theY = 4;
		return;
	case 0xf:
		if (theIndex == 0)
		{
			*theX = 3;
			*theY = -12;
			return;
		}
		if (theIndex == 1)
		{
			*theX = 16;
			*theY = 4;
			return;
		}
		if (theIndex == 2)
		{
			*theX = 1;
			*theY = 20;
			return;
		}
		*theX = -10;
		*theY = 4;
		return;
	}
	// the treasure 10
	if (theIndex == 0)
	{
		*theX = -11;
		*theY = -11;
		return;
	}
	if (theIndex == 1)
	{
		*theX = -10;
		*theY = 19;
		return;
	}
	if (theIndex == 2)
	{
		*theX = 17;
		*theY = -11;
		return;
	}
	*theX = 15;
	*theY = 16;
	return;
}

// 0x440b90: a creature close to the diver's side of the screen (x up to 160) switches the music to the tense track 0x25,
// one almost on the diver (x up to 15) to track 0x2b, unless one of those is already playing
void Board::UpdateDangerMusic(Enemy* theEnemy, bool theFlag)
{
	if (theEnemy->mX <= 160.0 && theFlag)
	{
		int aMusic = mApp->mMusicOrder;
		if (aMusic != 0x25 && aMusic != 0x2b)
		{
			mApp->ChangeMusic(0x25);
			return;
		}
	}
	if (theEnemy->mX <= 15.0 && theFlag && mApp->mMusicOrder != 0x2b)
		mApp->ChangeMusic(0x2b);
}

// 0x441bb0: how many of the piranha, shark and bonus creature lists hold something
int Board::CountActiveLists()
{
	int aCount = 0;
	if (!mPiranhas.empty())
		aCount = 1;
	if (!mSharks.empty())
		aCount = aCount + 1;
	if (!mBonusCreatures.empty())
		aCount = aCount + 1;
	return aCount;
}

// 0x442760: the third pair of clams was found: the diver is shown the treasure (state 6), the continue link comes up, the
// clams open (state 3) and the profile records the level's treasure
void Board::ShowTreasure()
{
	mPlayer->SetState(6);
	mApp->mWidgetManager->BringToFront(mContinueLink);
	mContinueLink->SetDisabled(false);
	mContinueLink->SetVisible(true);
	mApp->mUser->mField25c[mLessonManager->GetLevel()] = 1;
	unsigned long aRand = gGameRand->NextNoAssert();
	mApp->mUser->CountSecretFound(aRand & 3);
	mTreasureImage = mApp->mImages[IMAGE_SECRET_CROWN_LRG + (aRand & 3)];
	mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, false);
	mApp->PlaySample(SOUND_WINGEM);
	mStats.mReserved[7] = mGameTimer->Elapsed();		// the game's time, at Board +0xe40 (word 8 of GetStats' block)
	unsigned int i = 0;
	while (mClams.size() != 0 && i < mClams.size())
	{
		mClams[i]->SetState(3);
		i++;
	}
}

// 0x446110: a creature of a bonus wave (wave type 2) may only be placed where its box (and, for the black and red sharks,
// the words that follow it) does not run into the box of another bonus creature, or into the path the other one will
// swim; in a wave of type 0 or 1 the creature is fine unless it is beyond the Board's right edge. Returns true when it
// can stay where it is.
bool Board::AvoidWordOverlap(Enemy* theEnemy)
{
	Rect aRect;			// the box of the creature looked at (the original's [ebp-0x2c], zero at first)
	Rect aRect2;		// the box of theEnemy (the original's [ebp-0x4c])
	bool aResult = true;
	int aWaveType = mLessonManager->GetCurrentWaveType();
	if (aWaveType == 0 || aWaveType == 1)
	{
		if ((double)mWidth < theEnemy->mX)
			return false;
		return true;
	}
	if (aWaveType != 2)
		return true;

	// port: the original counts the loop by the size of mBonusTexts (+0xdb8) and indexes mBonusCreatures (+0xdc8) with it;
	// the index is held inside mBonusCreatures here (Oddities)
	int aCount = (int)mBonusTexts.size();
	mApp->mFontMyriadCondensed14Bold->StringWidth("A");		// the result is not used
	int aCharWidth;
	int i = 0;
	while (i < aCount)
	{
		if (i >= (int)mBonusCreatures.size())
			break;
		Enemy* anOther = mBonusCreatures[i];
		if (anOther != theEnemy && anOther->mVisible)
		{
			aRect.mX = (int)anOther->mX;
			aRect.mY = (int)anOther->mY;
			aRect.mHeight = anOther->GetFrameHeight();
			aCharWidth = mApp->mFontMyriadCondensed14Bold->StringWidth("A");
			int aRecordType = anOther->mField0e4;
			if (aRecordType == 1 || aRecordType == 10)
			{
				BlackShark* aBlackShark = dynamic_cast<BlackShark*>(anOther);
				bool aHasWord2 = aBlackShark->mWord2 != "";
				if (aHasWord2)
				{
					std::string aWord = anOther->mWord;
					aRect.mWidth = anOther->GetFrameWidth() + ((int)aWord.size() + 1) * aCharWidth;
				}
				else
					aRect.mWidth = anOther->GetFrameWidth();
			}
			else if (aRecordType == 2 || aRecordType == 9)
			{
				RedShark* aRedShark = dynamic_cast<RedShark*>(anOther);
				aRect.mWidth = anOther->GetFrameWidth();
				if (aRedShark->mWord2 != "")
				{
					std::string aWord = anOther->mWord;
					aRect.mWidth = aRect.mWidth + ((int)aWord.size() + 1) * aCharWidth;
				}
				if (aRedShark->mWord3 != "")
				{
					std::string aWord2 = aRedShark->mWord2;
					aRect.mWidth = aRect.mWidth + ((int)aWord2.size() + 1) * aCharWidth;
				}
			}
			else
				aRect.mWidth = anOther->GetFrameWidth();

			aRect2.mX = (int)theEnemy->mX;
			aRect2.mY = theEnemy->GetFrameHeight() + mHeight;
			aRect2.mWidth = theEnemy->GetFrameWidth();
			aRect2.mHeight = theEnemy->GetFrameHeight();
			if (aRect.Intersects(aRect2))
			{
				aResult = false;
				return aResult;
			}
			aRect2.mX = aRect.mX;
			if (aRect.Intersects(aRect2))
			{
				// the two would meet if theEnemy swam up to the other one's column: when does it get there?
				double aRelVelY = theEnemy->mVelY - anOther->mVelY;
				double anOtherBottom = (double)anOther->GetFrameHeight() + anOther->mY;
				double aGap = anOtherBottom - (double)(theEnemy->GetFrameHeight() + mHeight);
				if (aRelVelY != 0.0)
				{
					double aTime = aGap / aRelVelY;
					double aSpot = (double)(theEnemy->GetFrameHeight() + mHeight) - theEnemy->mVelY * aTime;
					if (35.0 < aSpot)
					{
						aResult = false;
						return aResult;
					}
				}
			}
		}
		i++;
	}
	return aResult;
}

// 0x446720: a clam's word is one capital letter that no other clam has: drawn again, with the clams looked through from the
// first again, while a clam has the same one
std::string Board::PickClamWord()
{
	std::string aWord = StrFormat("%c", (int)(gGameRand->NextNoAssert() % 26) + 'A');
	unsigned int i = 0;
	while (i < mClams.size())
	{
		std::string aClamWord = mClams[i]->mWord;
		if (aClamWord == aWord)
		{
			aWord = StrFormat("%c", (int)(gGameRand->NextNoAssert() % 26) + 'A');
			i = 0;
		}
		else
			i++;
	}
	return aWord;
}

// 0x44fe30: the diver lost a life (his count is mField110): without lives the result is recorded and the game-over dialog
// is made; otherwise he starts again at the top of the screen (state 1)
void Board::PlayerEaten()
{
	mPlayer->mField110 = mPlayer->mField110 - 1;
	if (mPlayer->mField110 < 0)
	{
		mApp->mUser->RecordResultWithDouble(true, -1, -1, -1.0, -1, -10, -1);
		mApp->mUser->ClearFields1cc();
		mStats.mReserved[7] = mGameTimer->Elapsed();		// Board +0xe40, word 8 of GetStats' block
		SaveProgress(true, true);
		if (mOceanDepth == -1)
		{
			mGameOverDialog = new GameOverDialog(mApp, mPlayer, true, mApp->mField900 + mTypeTime,
				mApp->mField90c + mField0f68, mApp->mField908 + mField0f6c, mScore);
			mGameOverDialog->mField144 = mScore;
		}
		else
		{
			mGameOverDialog = new GameOverDialog(mApp, mPlayer, false, mApp->mField900 + mTypeTime,
				mApp->mField90c + mField0f68, mApp->mField908 + mField0f6c, mScore);
		}
		mGameOverDialog->mField11c = mApp->mField8fc + mStats.mReserved[7];
		mGameOverDialog->mRankTitle = GetRankTitle();
		mApp->ChangeMusic(0x1e);
		SetPaused(true);
		mPlayer->SetState(8);
		mApp->AddDialog(DIALOG_GAME_OVER, mGameOverDialog);
		return;
	}
	mApp->mFlag671 = true;
	mTarget = NULL;
	mTyped = "";
	mPlayer->SetState(1);
	ResetPiranhaSpeed();
	mRatingTimer->Reset();
	mFlag0f80 = true;
	if ((double)mHeight < mField0e10 && mBoss == NULL)
	{
		mTimer0e50->SetDisabled(false);
		mTimer0e50->Reset();
		mField0e54 = 0xff;
	}
}

// 0x45c140: one tick of the game. The paths that return early leave out the MarkDirty at the end (0x45e74c).
void Board::Update()
{
	mUpdateCount++;
	if (mDisabled)
		return;
	if (mPaused)
		return;
	Widget::Update();

	if (mFirstUpdate)
	{
		mFirstUpdate = false;
		mApp->PlaySample(SOUND_DIVERENTERS);
		if (mLessonManager->GetLevel() > 0)
			mApp->mUser->mField25c[mLessonManager->GetLevel()] = 1;
		mApp->SaveGameProgress(false, true);
		mField0d2c = mApp->mUser->GetField1e4();
	}
	mHudBar->Update();

	// the black fade-in at the start of a level goes down by 2 every other tick
	if (mFadeAlpha > 0)
	{
		if (mUpdateCnt % 2 != 0)
		{
			if (mFadeAlpha == 0x75)
			{
				mApp->SetCursor(0);
				mApp->ShowLevelIntro();
				mField0e54 = mApp->mUser->mFlag1dc ? 255 : 0;
				mTimer0e50->SetDisabled(false);
			}
			mFadeAlpha = mFadeAlpha - 2;
			if (mFadeAlpha < 0)
				mFadeAlpha = 0;
		}
	}

	// the waves of the water move every other tick
	if (mUpdateCnt % 2 == 0)
	{
		if (mWaveGrowing)
		{
			mWaveAmp = mWaveAmp + 0.004;
			if (!(mWaveAmp < 5.0))
			{
				mWaveGrowing = false;
				mWaveAmp = 5.0;
			}
		}
		else
		{
			mWaveAmp = mWaveAmp - 0.004;
			if (mWaveAmp <= 4.0)
			{
				mWaveGrowing = true;
				mWaveAmp = 4.0;
			}
		}
		mWavePhase1 = mWavePhase1 + 1;
		if (mUpdateCnt % 4 == 0)
			mWavePhase2 = mWavePhase2 + 1;
	}

	if (mPlayer->mState == 7)
	{
		// the sonar sweep over the sea while the diver is in state 7: a new picture every sixth tick
		if (mField0ef8 != 0xff)
			return;
		if (mUpdateCnt % 6 != 0)
			return;
		if (mField0ee4 > 0x10)
			return;
		if (mField0ee4 == 0)
			mApp->PlaySample(SOUND_SONARPING1);
		mField0ee4 = mField0ee4 + 1;
		int aStep = mField0ee4;
		if (aStep <= 4)
			mImage0ee8 = mApp->mImages[IMAGE_SONAR_SWEEP_0 + aStep];
		else if (aStep <= 8)
		{
			mImage0ee8 = NULL;
			mImage0eec = mApp->mImages[IMAGE_SONAR_SWEEP_1 + aStep];
		}
		else if (aStep <= 0xc)
		{
			mImage0eec = NULL;
			mImage0ef0 = mApp->mSonarSweepImages[aStep - 8];
		}
		else if (aStep <= 0x10)
		{
			mImage0ef0 = NULL;
			mImage0ef4 = mApp->mSonarSweep2Images[aStep - 12];
		}
		else
			mImage0ef4 = NULL;
		MarkDirty();
		return;
	}

	// the diver's EMP charge: 0.01 for every mEmpInterval milliseconds, never in a bonus level
	if (mPlayer->mEmpCharge < 1.0 && mEmpTimer->Elapsed() >= mEmpInterval && !mLessonManager->IsBonusLevel())
	{
		mPlayer->mEmpCharge = mPlayer->mEmpCharge + 0.01;
		if (!(mPlayer->mEmpCharge < 1.0))
			mPlayer->mEmpCharge = 1.0;
		mEmpTimer->Reset();
	}
	else
	{
		// the first time the charge is full, the hint box about the Shark Zapper comes up
		if (!(mPlayer->mEmpCharge < 1.0) && mFlag0f80 && !IsBelowSurface() && mFadeAlpha < 5 && mApp->HintsEnabled()
			&& !mLessonManager->IsBonusLevel() && mField0e54 == 0 && (mPlayer->mState == 0 || mPlayer->mState == 1))
		{
			mApp->ShowHintDialog(GT(0x005316f0), GT(0x0052b520));
			mApp->SetUserHints(false);
		}
	}

	// the white flash goes down by 8 a tick
	if (mFlashAlpha > 0)
	{
		mFlashAlpha = mFlashAlpha - 8;
		if (mFlashAlpha < 0)
			mFlashAlpha = 0;
	}

	// the charge drains by 0.01 a tick after the zap
	if (mFlag0eb4)
	{
		double aCharge = mPlayer->mEmpCharge - 0.01;
		if (aCharge <= 0.0)
		{
			mFlag0eb4 = false;
			aCharge = 0.0;
		}
		mPlayer->mEmpCharge = aCharge;
	}

	// every 400 ticks, when fewer than 8 fish swim behind, 0 to 4 new ones swim in (the later ones follow the first)
	if (mUpdateCnt % 400 == 0)
	{
		// (the original's followers are gone in the tick they are made, so the fixes mod's schools count only the leaders, which
		// keeps the number of fish that this test reads, and so the generator's draws, as the original's)
		unsigned int aFishCount = mBackgroundFish.size();
		if (FixEnabled("schools"))
		{
			aFishCount = 0;
			for (unsigned int i = 0; i < mBackgroundFish.size(); i++)
				if (!mBackgroundFish[i]->mFollower)
					aFishCount++;
		}
		if (mBackgroundFish.empty() || aFishCount < 8)
		{
			int aNumFish = (int)(gGameRand->NextNoAssert() % 5);
			BackgroundFish* aFirstFish = NULL;
			int aFishIndex = 0;
			while (aFishIndex < aNumFish)
			{
				BackgroundFish* aFish;
				if (aFishIndex == 0)
				{
					aFish = new BackgroundFish(mApp);
					aFirstFish = aFish;
				}
				else
					aFish = new BackgroundFish(mApp, aFirstFish);
				mBackgroundFish.push_back(aFish);
				aFishIndex++;
			}
		}
	}

	// the fish swim; one that has passed the right edge is deleted
	unsigned int aFishIdx = 0;
	while (aFishIdx < mBackgroundFish.size())
	{
		mBackgroundFish[aFishIdx]->Update();
		BackgroundFish* aFish = mBackgroundFish[aFishIdx];
		if ((double)mWidth < aFish->mX)
		{
			delete aFish;
			mBackgroundFish.erase(mBackgroundFish.begin() + aFishIdx);
			aFishIdx--;
		}
		aFishIdx++;
	}

	// a boss is on the screen and not beaten yet: the diver is afraid (state 9)
	if (mPlayer->mState == 0 && mBoss != NULL && mBoss->mState != 3)
	{
		mPlayer->SetState(9);
		mPlayer->SetFlag118(false);
	}

	if (mPlayer->mFlag118)
	{
		if (!mApp->mUser->mFlag1dc)
			mPlayer->SetFlag118(false);
		else
		{
			// the climb down is over: in a lesson with a boss at the end, the dive stops once the ocean floor is in view
			if (!mLessonManager->mAbyssMode && (mPlayer->mState == 0 || mPlayer->mState == 1))
			{
				double aBottom = (double)(mHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight());
				if (!(aBottom < mField0e10) && mLessonManager->LastWaveIsBoss() && !mFlag0ebc && !mLessonManager->IsEndless())
					mPlayer->SetFlag118(false);
			}

			// the small number in the corner fades out two seconds after the fade-in
			if (mTimer0e50->Elapsed() >= 2000)
			{
				mField0e54 = mField0e54 - 3;
				if (mField0e54 <= 0)
				{
					mField0e54 = 0;
					mTimer0e50->SetDisabled(true);
				}
			}

			// the dive: the depth grows by the diver's speed, the two background strips scroll up (a strip that has
			// left the screen jumps back below the other one)
			mDivePos = mDivePos + mDiverSpeed;
			for (int i = 0; i < 2; i++)
			{
				double aNewY = mBgLayers[i].mY - mDiverSpeed;
				mBgLayers[i].mY = aNewY;
				if (!(-(double)mField0f84 < aNewY))
					mBgLayers[i].mY = (double)(mField0f84 * 2) + aNewY;
			}

			// the labels of the depth ruler scroll up too; one that has left the top is deleted, one that the diver has
			// reached sets the depth (mScore) and, on the way, the checkpoints of an abyss game
			unsigned int aMarkerIdx = 0;
			while (aMarkerIdx < mDepthMarkers.size())
			{
				DepthMarker* aMarker = mDepthMarkers[aMarkerIdx];
				aMarker->mY = aMarker->mY - mDiverSpeed;
				if (aMarker->mY < -20.0)
				{
					delete aMarker;
					mDepthMarkers.erase(mDepthMarkers.begin() + aMarkerIdx);
					aMarkerIdx--;
				}
				else
				{
					aMarker = mDepthMarkers[aMarkerIdx];
					if (!((double)mPlayer->GetFrameHeight() + mPlayer->mY < aMarker->mY))
					{
						if (mDepthMarkers[aMarkerIdx]->mDepth > mScore)
						{
							mScore = mDepthMarkers[aMarkerIdx]->mDepth;
							if (mScore % 1000 == 0)
							{
								if (mLessonManager->mAbyssMode && mBoss == NULL && !mFlag0d28)
								{
									int aCount = mField0d2c + 1;
									mFlag0d28 = true;
									mField0d2c = aCount;
									mApp->mUser->SetField1e4(aCount);
								}
							}
							if (mScore % 400 == 0)
								mApp->mUser->SetAdValue((mScore / 400) * 400, mPlayer->mEmpCharge);
						}
					}
				}
				aMarkerIdx++;
			}

			// the ruler is kept 15 labels long: new ones are made after the last, a label "- N" for every 20 and a plain "-"
			// for every 4 (the port does not read the last of an empty list, which the original would)
			if (!mDepthMarkers.empty() && mDepthMarkers.size() < 0xf)
			{
				DepthMarker* aLast = mDepthMarkers[mDepthMarkers.size() - 1];
				int aDepth = aLast->mDepth + 4;
				int aEndDepth = aLast->mDepth + 0x40;
				if (aDepth < aEndDepth)
				{
					int aOffset = (aDepth - aLast->mDepth) * 10;
					do
					{
						if (aDepth % 20 == 0)
						{
							DepthMarker* aNewMarker = new DepthMarker;
							aNewMarker->mText = StrFormat(GTC(0x00530a6c), aDepth);
							aNewMarker->mX = 0.0;
							aNewMarker->mY = (double)aOffset + aLast->mY;
							aNewMarker->mDepth = aDepth;
							mDepthMarkers.push_back(aNewMarker);
						}
						else if (aDepth % 4 == 0)
						{
							DepthMarker* aNewMarker = new DepthMarker;
							aNewMarker->mText = "-";		// 0x530a68, a single dash (not in the game text table)
							aNewMarker->mX = 0.0;
							aNewMarker->mY = (double)aOffset + aLast->mY;
							aNewMarker->mDepth = aDepth;
							mDepthMarkers.push_back(aNewMarker);
						}
						aDepth++;
						aOffset += 10;
					} while (aDepth < aEndDepth);
				}
			}

			if (!(mField0e10 == -1.0))
			{
				int aFloorLimit = mHeight - mApp->mImages[IMAGE_OCEANFLOOR]->GetHeight()
					- mApp->mImages[IMAGE_STATUSBARX]->GetHeight() + 10;
				if (!((double)aFloorLimit < mField0e10))
				{
					// the ocean floor is in reach
					if (mPlayer->mState == 0 && !(mPlayer->mY < 235.0))
					{
						mPlayer->SetState(4);
						if (mLessonManager->IsBonusLevel())
						{
							// the clams of the bonus level: 22 of them in four staggered rows (5, 6, 5, 6), four of which
							// (drawn here, all different) are of the type that wins
							unsigned long aRand = gGameRand->NextNoAssert();
							int aClamA = (int)(aRand % 0x16);
							int aClamD = aClamA;
							int aClamB;
							do
							{
								aClamB = (int)(gGameRand->NextNoAssert() % 0x16);
							} while (aClamB == aClamA);
							int aClamC;
							do
							{
								aClamC = (int)(gGameRand->NextNoAssert() % 0x16);
							} while (aClamC == aClamA || aClamC == aClamB);
							while (aClamD == aClamA || aClamD == aClamB || aClamD == aClamC)
								aClamD = (int)(gGameRand->NextNoAssert() % 0x16);

							int aRow = 0;
							do
							{
								int aClamIndex;
								int aClamX;
								double aClamY;
								int aEndX;
								if (aRow % 2 != 0)
								{
									aClamIndex = (aRow != 1) ? 16 : 5;
									aClamY = (double)((aRow != 1) ? 330 : 220);
									aClamX = 175;
									aEndX = 0x1db;
								}
								else
								{
									aClamIndex = (aRow != 0) ? 11 : 0;
									aClamY = (double)((aRow != 0) ? 280 : 170);
									aClamX = 200;
									aEndX = 0x1c2;
								}
								do
								{
									int aClamType = 0;
									if (aClamIndex == aClamA || aClamIndex == aClamB || aClamIndex == aClamC || aClamIndex == aClamD)
										aClamType = 1;
									Clam* aClam = new Clam(mApp, aClamType);
									aClam->SetX((double)aClamX);
									aClam->SetY(aClamY);
									aClam->SetWord(PickClamWord());
									mClams.push_back(aClam);
									aClamX += 0x32;
									aClamIndex++;
								} while (aClamX < aEndX);
								aRow++;
							} while (aRow < 4);
						}
					}
					else if (mPlayer->mState == 0xb && !(mPlayer->mY < 235.0))
					{
						// the diver is back at the bottom after the treasure: the level is won
						mPlayer->SetFlag118(false);
						mPlayer->SetState(0xc);
						mEmpTimer->SetDisabled(true);
						mTimer0f00->SetDisabled(false);
						mTimer0f00->Reset();
						mApp->PlaySample(SOUND_WINGEM);
						mStats.mReserved[7] = mGameTimer->Elapsed();		// Board +0xe40
						mPlayer->AddScore(GetLevelBonus());
					}
					else
						mPlayer->SetFlag118(false);
				}
				else
				{
					// the floor is still below: the sea pictures scroll up with the dive (the far ones slower, the near ones faster)
					mField0e10 = mField0e10 - mDiverSpeed;
					double aNear = (double)(mHeight - mApp->mImages[IMAGE_STATUSBARX]->GetHeight());
					if (aNear > mField0d20)
					{
						mField0d10 = mField0d10 - (mDiverSpeed + mDiverSpeed);
						mField0d18 = mField0d18 - mDiverSpeed * 1.8;
						mField0d20 = mField0d20 - mDiverSpeed * 1.6;
					}
					else
					{
						mField0d10 = mField0d10 - mDiverSpeed;
						mField0d18 = mField0d18 - mDiverSpeed;
						mField0d20 = mField0d20 - mDiverSpeed;
					}
				}
			}
			else
			{
				// no ocean floor (endless): every 200 feet of depth the lesson goes on to the next level (0x45c140: score / 200)
				if (mScore / 200 > mField1c0 / 200 && mFlag0f80)
				{
					mField1c0 = mScore;
					mLessonManager->NextLevel();
					bool aGoOn = true;
					if (mLessonManager->mAbyssMode)
					{
						unsigned int aLevelCount = (unsigned int)mLessonManager->mLevels.size();
						if ((unsigned int)mLessonManager->mLevel >= aLevelCount - 1)
							aGoOn = false;
					}
					if (aGoOn)
						mLessonManager->NextWave(true);
				}
			}
		}
	}
	else
	{
		if (mPlayer->mState == 1 || mPlayer->mState == 9)
		{
			if (mTimer0e50->Elapsed() >= 2000)
			{
				mField0e54 = mField0e54 - 3;
				if (mField0e54 <= 0)
				{
					mField0e54 = 0;
					mTimer0e50->SetDisabled(true);
				}
			}
		}
	}

	if (mPlayer->mState == 4)
	{
		// the diver waits at the bottom of the screen for the creatures of the wave to leave
		if (!mFlag0f80)
		{
			if (!(mBoss != NULL && mBoss->mState == 3))
			{
				int aWaveType = mLessonManager->GetCurrentWaveType();
				// port: for a wave type other than 0, 1 and 2 the original reads an unset stack slot as the count; 0 here
				int aCount = 0;
				if (aWaveType == 0)
					aCount = (int)mSharks.size();
				else if (aWaveType == 1)
					aCount = (int)mPiranhas.size();
				else if (aWaveType == 2)
					aCount = (int)mBonusCreatures.size();
				int i = 0;
				if (aCount > 0)
				{
					Enemy* aCreature = NULL;
					do
					{
						if (mLessonManager->GetCurrentWaveType() == 0)
							aCreature = mSharks[i];
						else if (mLessonManager->GetCurrentWaveType() == 1)
							aCreature = mPiranhas[i];
						else if (mLessonManager->GetCurrentWaveType() == 2)
							aCreature = mBonusCreatures[i];
						if (mLessonManager->GetCurrentWaveType() != 2)
						{
							// a shark or piranha that reached the right edge is deleted
							if ((double)mWidth <= aCreature->mX)
							{
								delete aCreature;
								if (mLessonManager->GetCurrentWaveType() == 0)
									mSharks.erase(mSharks.begin() + i);
								else if (mLessonManager->GetCurrentWaveType() == 1)
									mPiranhas.erase(mPiranhas.begin() + i);
								aCount = aCount - 1;
								i--;
							}
						}
						else
						{
							// a bonus creature that has left through the top is taken off the list. leak fix (the original leaks it, 0x45c140):
							// the board keeps it with the retired creatures and frees it with the board, since mTarget may still point to it
							if ((double)(-aCreature->GetFrameHeight()) >= aCreature->mY)
							{
								mRetired.push_back(aCreature);
								mBonusCreatures.erase(mBonusCreatures.begin() + i);
								aCount = aCount - 1;
								i--;
							}
						}
						i++;
					} while (i < aCount);
				}
				if (aCount == 0)
				{
					mFlag0f80 = true;
					mRatingTimer->SetDisabled(true);
					ShowRating();
				}
			}
		}
		if (mFlag0f80)
		{
			if (mRating->mAlpha == 0)
			{
				mApp->ChangeMusic(0x22);
				mField0f38 = mField0f38 + 0.96;
				if (!(mField0f38 < 240.0))
				{
					mPlayer->SetState(5);
					mApp->PlaySample(SOUND_TREASURE);
					mTarget = NULL;
					mApp->mStatusBar->HideButtons();
					mApp->mStatusBar->ShowButton(false);
					MarkDirty();
					if (mLessonManager->IsBonusLevel())
					{
						unsigned int aClamIdx = 0;
						while (aClamIdx < mClams.size())
						{
							mClams[aClamIdx]->SetState(4);
							aClamIdx++;
						}
					}
				}
			}
		}
	}
	else if (mPlayer->mState == 5)
	{
		// the wave is over and the screen is empty: the treasure level (or the level's end) comes
		if (!mLessonManager->IsBonusLevel())
		{
			if (mString0f10 == "")
			{
				mString0f10 = mApp->GetLongWord(mLessonManager->GetLevelSettings()->mMinEndDiff,
					mLessonManager->GetLevelSettings()->mMaxEndDiff);
				mField0f2c = 0;
			}
		}
		if (mLessonManager->IsBonusLevel())
			mField0f38 = mField0f38 - 0.12;
		else
			mField0f38 = mField0f38 - 0.1;
		if (mField0f38 <= 0.0)
		{
			mField0f38 = 0.0;
			mPlayer->SetState(6);
			mEmpTimer->SetDisabled(true);
			int aSpot = (int)(gGameRand->NextNoAssert() & 3);
			GetSpotOffset(&mField0ec0[0], &mField0ec0[1], aSpot);
			aSpot++;
			if (aSpot >= 4)
				aSpot = 0;
			GetSpotOffset(&mField0ec0[3], &mField0ec0[4], aSpot);
			aSpot++;
			if (aSpot >= 4)
				aSpot = 0;
			GetSpotOffset(&mField0ec0[6], &mField0ec0[7], aSpot);
			if (!mLessonManager->IsBonusLevel())
			{
				if (mField0f0c > mApp->mUser->mField64)
					mApp->mUser->mField64 = mField0f0c;
				mApp->mUser->CountGemFound(mApp->GetTreasureIndex() - 10);
				if (mField0f30 > mApp->mUser->mBestTreasureWords)
					mApp->mUser->mBestTreasureWords = mField0f30;
				mPlayer->AddScore(GetLevelBonus());
				mTimer0f00->SetDisabled(false);
				mTimer0f00->Reset();
				mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, false, false);
				mApp->PlaySample(SOUND_WINGEM);
				mStats.mReserved[7] = mGameTimer->Elapsed();		// Board +0xe40
			}
			else
				ShowTreasure();
		}

		// the glow (mField0f44 between 170 and 255) pulses in the last third of the countdown, faster in the last sixth
		double aProgress = mField0f38 * 0.004166666666666667;
		if (aProgress <= 0.33333)
		{
			int aStep = (aProgress <= 0.1666666) ? 8 : 4;
			if (mFlag0f40)
			{
				mField0f44 = mField0f44 + aStep;
				if (mField0f44 > 0xff)
				{
					mFlag0f40 = false;
					mField0f44 = 0xff;
				}
			}
			else
			{
				mField0f44 = mField0f44 - aStep;
				if (!(mField0f44 >= 0xaa))
				{
					mFlag0f40 = true;
					mField0f44 = 0xaa;
				}
			}
		}
	}
	else if (mPlayer->mState == 6 || mPlayer->mState == 0xc)
	{
		// the treasure is shown (6) or the level is won (0xc)
		if (mUpdateCnt % 11 == 0)
		{
			mField0f48 = mField0f48 + 1;
			if (mField0f48 >= 10)
				mField0f48 = 0;
		}
		if (mLessonManager->IsBonusLevel())
			mApp->ChangeMusic(0x11);
		if (mTimer0f00->Elapsed() >= 0xbb8)
		{
			mTimer0f00->SetDisabled(true);
			mFlag0f08 = true;
		}
		if (mUpdateCnt % 10 == 0)
		{
			mField0ec0[2] = mField0ec0[2] + 1;
			if (mField0ec0[2] >= 14)
				mField0ec0[2] = 0;
			mField0ec0[5] = mField0ec0[5] + 1;
			if (mField0ec0[5] >= 14)
				mField0ec0[5] = 0;
			mField0ec0[8] = mField0ec0[8] + 1;
			if (mField0ec0[8] >= 14)
				mField0ec0[8] = 0;
		}
		if (mFlag0f08)
		{
			mField0f04 = mField0f04 - 1;
			if (mField0f04 <= 0)
			{
				mField0f04 = 0;
				mPlayer->SetState(7);
				if (mApp->mStatusBar != NULL)
				{
					mApp->mStatusBar->HideButtons();
					mApp->mStatusBar->ShowButton(false);
					MarkDirty();
				}
				mApp->mStatusBar->HideButtons();
				mApp->mStatusBar->ShowButton(false);
				mApp->ChangeMusic(0x11);
				mApp->mWidgetManager->BringToFront(mContinueLink);
				mContinueLink->SetDisabled(false);
				mContinueLink->SetVisible(true);
				int aPercent = (int)((double)mField0f68 / ((double)mField0f6c + (double)mField0f68) * 100.0);
				if (aPercent >= 0x5f)
				{
					mField0e8c = mField0e8c + 1;
					mPlayer->AddScore(GetFinishBonus());
				}
				SaveProgress(true, false);
			}
		}
	}
	else if (mPlayer->mState == 2)
	{
		// the beaten diver has floated up out of the screen: a life is gone
		if ((double)(-mPlayer->GetFrameHeight()) > mPlayer->mY)
			PlayerEaten();
	}

	mPlayer->Update();
	if (mBoss != NULL)
	{
		mBoss->Update();
		// leak fix (see ProjectileBoss::Update): the shots the boss has let go are kept with the retired creatures
		ProjectileBoss* aShotBoss = static_cast<ProjectileBoss*>(mBoss);
		for (size_t i = 0; i < aShotBoss->mFinishedShots.size(); i++)
			mRetired.push_back(aShotBoss->mFinishedShots[i]);
		aShotBoss->mFinishedShots.clear();
		HandleBossEvents(mBoss);
	}

	// every 8 feet of the dive the diver is paid a point
	if (!(mDivePos - mDivePosMarked < 8.0))
	{
		mPlayer->AddScore(1);
		mDivePosMarked = mDivePos;
	}

	// the opening hint ("Type the words on the sharks...") fades once the player knows the game
	if (!mApp->mUser->mFlag1dc && mFlag0ea8)
	{
		mField0ea4 = mField0ea4 - 2;
		if (mField0ea4 <= 0)
		{
			mField0ea4 = 0;
			mFlag0ea8 = false;
		}
	}

	// a new wave is sent when the screen is clear (or, in the first level, once the diver has dived half way)
	if (mFlag0f80)
	{
		if (!mFlag0ebc && mPlayer->mState == 0
			&& ((double)mHeight <= mField0e10 || mOceanDepth == -1)
			&& mBoss == NULL
			&& ((double)(mSurfaceHeight >> 1) < mDivePos || !mApp->mUser->mFlag1dc))
			SpawnWave();
		if (mFlag0f80 && !mFlag0ebc && mPlayer->mState == 0 && !mPlayer->mFlag118 && mBoss == NULL
			&& mLessonManager->LastWaveIsBoss() && CountActiveLists() == 0)
			SpawnWave();
	}

	// the creatures: the sharks first, then the piranhas, the clams and the bonus creatures
	mFlag0efc = false;
	double aEmpLimit = (double)(int)mPlayer->mEmpX + 16.0;		// the EMP wave and the creatures it has reached
	unsigned int aSharkIdx = 0;
	while (aSharkIdx < mSharks.size())
	{
		Enemy* aEnemy = mSharks[aSharkIdx];
		Shark* aShark = dynamic_cast<Shark*>(aEnemy);
		if (!aEnemy->mVisible)
		{
			if (AvoidWordOverlap(aEnemy))
				aEnemy->SetVisible(true);
		}
		else
		{
			if (mPlayer->mEmpActive && aEnemy->mX <= aEmpLimit && aEnemy->mState == 0)
			{
				mPlayer->AddScore(aEnemy->mField54);
				aEnemy->mFlag50 = true;
				aEnemy->ReleaseTarget();
				aEnemy->SetState(5);
				aEnemy->mEvents.push_back(1);
				mApp->ChangeMusic(0);
			}
			UpdateDangerMusic(aEnemy, aShark->IsSwimming());
		}
		if (aShark->mEntered && aEnemy->mState == 1 && mPlayer->mAlpha <= 0x80)
		{
			aShark->mEntered = false;
			aEnemy->mVelX = 2.3;
		}
		if ((mPlayer->mState == 3 || mPlayer->mState == 2) && aEnemy->mState != 1)
			aEnemy->SetState(4);
		aEnemy->Update();
		if (!HandleSharkEvents(aEnemy))
		{
			// a shark that has died is moved to the front of the list once (the index stays, so the next one is looked at
			// from the same place)
			if (aEnemy->mFlag08)
				aSharkIdx++;
			else if (aEnemy->mState == 3)
			{
				aEnemy->mFlag08 = true;
				mSharks.erase(mSharks.begin() + aSharkIdx);
				mSharks.insert(mSharks.begin(), aEnemy);
			}
			else
				aSharkIdx++;
		}
	}

	unsigned int aPiranhaIdx = 0;
	while (aPiranhaIdx < mPiranhas.size())
	{
		Enemy* aEnemy = mPiranhas[aPiranhaIdx];
		Piranha* aPiranha = dynamic_cast<Piranha*>(aEnemy);
		if (!aEnemy->mVisible)
		{
			if (AvoidWordOverlap(aEnemy) || mPlayer->mState == 3)
				aEnemy->SetVisible(true);
		}
		else
		{
			if (mPlayer->mEmpActive && aEnemy->mX <= aEmpLimit && aEnemy->mState == 0)
			{
				mPlayer->AddScore(aEnemy->mField54);
				aEnemy->mFlag50 = true;
				aEnemy->ReleaseTarget();
				aEnemy->SetState(5);
				aEnemy->mEvents.push_back(1);
				mApp->ChangeMusic(0);
			}
			UpdateDangerMusic(aEnemy, aPiranha->IsSwimming());
		}
		if (aPiranha->mEntered && aEnemy->mState == 1 && mPlayer->mAlpha <= 0x80)
		{
			aPiranha->mEntered = false;
			aEnemy->mVelX = 2.3;
		}
		if ((mPlayer->mState == 3 || mPlayer->mState == 2) && aEnemy->mState != 1)
			aEnemy->SetState(4);
		aEnemy->Update();
		if (!HandlePiranhaEvents(aEnemy))
		{
			// as with the sharks, a dead piranha (state 3) is moved to the front once
			int aPiranhaState = aEnemy->mState;
			if (aPiranhaState == 2)
				aPiranhaIdx++;
			else if (aEnemy->mFlag08)
				aPiranhaIdx++;
			else if (aPiranhaState == 3)
			{
				aEnemy->mFlag08 = true;
				mPiranhas.erase(mPiranhas.begin() + aPiranhaIdx);
				mPiranhas.insert(mPiranhas.begin(), aEnemy);
			}
			else
				aPiranhaIdx++;
		}
	}

	unsigned int aClamIdx2 = 0;
	while (aClamIdx2 < mClams.size())
	{
		mClams[aClamIdx2]->Update();
		HandleClamEvents(mClams[aClamIdx2]);
		aClamIdx2++;
	}

	int aBonusIdx = 0;
	while (aBonusIdx < (int)mBonusCreatures.size())
	{
		Enemy* aBonus = mBonusCreatures[aBonusIdx];
		if (!aBonus->mVisible)
		{
			if (AvoidWordOverlap(aBonus))
			{
				aBonus->SetVisible(true);
				aBonus->SetY((double)(aBonus->GetFrameHeight() + mHeight));
			}
		}
		else
		{
			if (mPlayer->mEmpActive && aBonus->mX <= aEmpLimit && (double)mHeight > aBonus->mY && aBonus->mState == 0)
			{
				mPlayer->AddScore(200);
				aBonus->mFlag50 = true;
				aBonus->SetState(3);
				aBonus->mEvents.push_back(1);
				mApp->ChangeMusic(0);
			}
		}
		aBonus->Update();
		if (HandleBonusCreatureEvents(aBonus))
			aBonusIdx--;
		aBonusIdx++;
	}

	// every 100 ticks a bubble rises from a random place on the bottom edge of the window
	if (mUpdateCnt % 100 == 0)
	{
		Bubble* aNewBubble = new Bubble(mApp);
		aNewBubble->SetX((double)(gGameRand->NextNoAssert() % (unsigned int)mApp->mWidth));
		aNewBubble->SetY((double)mApp->mHeight);
		mBubbles.push_back(aNewBubble);
	}

	// the bubbles rise; one that has left the top is deleted
	int aBubbleIdx = 0;
	while (aBubbleIdx < (int)mBubbles.size())
	{
		Bubble* aBubble = mBubbles[aBubbleIdx];
		aBubble->Update();
		if (aBubble->mY <= -20.0)
		{
			delete aBubble;
			mBubbles.erase(mBubbles.begin() + aBubbleIdx);
			aBubbleIdx--;
		}
		aBubbleIdx++;
	}

	// the texts of the points (state 1: finished) are deleted
	int aTextIdx = 0;
	while (aTextIdx < (int)mBonusTexts.size())
	{
		mBonusTexts[aTextIdx]->Update();
		BonusText* aText = mBonusTexts[aTextIdx];
		if (aText->mState == 1)
		{
			delete aText;
			mBonusTexts.erase(mBonusTexts.begin() + aTextIdx);
			aTextIdx--;
		}
		aTextIdx++;
	}

	// the rating popup rises and fades (its timer is restarted when it is half gone)
	if (mRating->mActive)
	{
		mRating->mY = mRating->mY - 1.0;
		mRating->mAlpha = mRating->mAlpha - 1;
		if (mRating->mAlpha <= 0x80)
		{
			mRatingTimer->SetDisabled(false);
			mRatingTimer->Reset();
		}
		if (mRating->mAlpha <= 0)
		{
			mRating->mAlpha = 0;
			mRating->mActive = false;
			mRating->mPoints = 0;
			mRating->mImage = NULL;
		}
	}

	// the banner text (mBannerText) fades out two seconds after it was set
	if (mBannerText != "")
	{
		if (mBannerLife > 0)
		{
			if (mBannerTimer->Elapsed() >= 0x7d0)
			{
				mBannerLife = mBannerLife - 2;
				if (mBannerLife <= 0)
					mBannerLife = 0;
			}
		}
	}

	MarkDirty();
}
