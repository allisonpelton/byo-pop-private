// BoardGame.cpp: the Board's game-level members (job B3): what WinWSApp calls to hand the game's numbers in and out
// (GetStats, StartGame, SetFastCPU), to save the profile (SaveProgress), to pause (SetPaused), to start a level or an
// abyss game (SetLessonManager, SetDepth), and the two small makers of the effects (AddBubble, AddBonusText).
//
// The three small helpers of the profile's statistics records (0x413de0, 0x413e30, 0x413e80; the export names them
// UserProfile members) are file-static functions of this file: only SaveProgress calls them. Their record is
// mGroups[group].mRecords[record].mField, a run of twelve ints in which each helper owns four (the last value, the best, the
// sum and the count).

#include "Board.h"
#include "Enemy.h"
#include "Bubble.h"
#include "BonusText.h"
#include "StatusBar.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "UserProfileMgr.h"
#include "GameText.h"
#include "Res.h"
#include "Image.h"
#include "SoundInstance.h"
#include "ImageFont.h"
#include <string.h>

using namespace Sexy;

// The CRT's _ftol (0x509890): the 64-bit truncation of the x87 value, which gives 0 for a NaN or a value that does not fit.
// (Dividing 0 by 0 for the accuracy of a game in which nothing was typed is such a NaN.)
static int Ftol(double theValue)
{
	if (!(theValue > -9.2233720368547758e18 && theValue < 9.2233720368547758e18))
		return 0;
	return (int)(long long)theValue;
}

// 0x440080: copies the numbers WinWSApp keeps between two levels of a game into mStats and returns the block. The double
// that fills the two words at +8 is the diver's EMP charge. The last word (mReserved[10]) is never written; the block
// keeps what it had.
AppStats Board::GetStats()
{
	mStats.mPointsSoFar = mPlayer->mScore;
	memcpy(&mStats.mReserved[1], &mPlayer->mEmpCharge, sizeof(double));
	mStats.mReserved[0] = mPlayer->mField114;
	mStats.mReserved[3] = mPlayer->mField110;
	mStats.mReserved[4] = mRating->mStreak;
	mStats.mReserved[5] = mField0e8c;
	mStats.mReserved[6] = mPausedTime;
	mStats.mReserved[7] = mGameTimer->Elapsed();
	mStats.mReserved[8] = mField0f68;
	mStats.mReserved[9] = mField0f6c;
	AppStats aStats = mStats;
	return aStats;
}

// 0x440130: the reverse of GetStats: puts the saved numbers back into the diver, the rating popup and the counters of the
// board that continues the game, and takes the diver's score as the start of this level.
void Board::StartGame(const AppStats& theStats)
{
	memcpy(&mPlayer->mEmpCharge, &theStats.mReserved[1], sizeof(double));
	mPlayer->mField114 = theStats.mReserved[0];
	mPlayer->mField110 = theStats.mReserved[3];
	mRating->mStreak = theStats.mReserved[4];
	mPausedTime = theStats.mReserved[6];
	mField0e8c = theStats.mReserved[5];
	mPlayer->SetScore(theStats.mPointsSoFar);
	mField0e90 = mPlayer->mScore;
}

// 0x440570
void Board::SetFastCPU(bool theFlag)
{
	mFastCPU = theFlag;
}

// 0x413de0: the record's first four ints: the last value (+1), the best (+0), the sum (+2) and the count (+3).
static void AddRecordValue0(UserProfile* theUser, int theGroup, int theRecord, int theValue)
{
	int* aField = theUser->mGroups[theGroup].mRecords[theRecord].mField;
	aField[1] = theValue;
	if (theValue > aField[0])
		aField[0] = theValue;
	aField[2] += theValue;
	aField[3]++;
}

// 0x413e30: the same for the second four ints (the accuracy of the game).
static void AddRecordValue4(UserProfile* theUser, int theGroup, int theRecord, int theValue)
{
	int* aField = theUser->mGroups[theGroup].mRecords[theRecord].mField;
	aField[5] = theValue;
	if (theValue > aField[4])
		aField[4] = theValue;
	aField[6] += theValue;
	aField[7]++;
}

// 0x413e80: the same for the third four ints (the typing speed of the game).
static void AddRecordValue8(UserProfile* theUser, int theGroup, int theRecord, int theValue)
{
	int* aField = theUser->mGroups[theGroup].mRecords[theRecord].mField;
	aField[9] = theValue;
	if (theValue > aField[8])
		aField[8] = theValue;
	aField[10] += theValue;
	aField[11]++;
}

// 0x440580: puts the game into the user's profile and writes the profile out. The level is the diver's mField114. A game
// that is still at its first level with the diver's mField110 below zero (and not the abyss mode) takes the short way: the
// last game and the best bonus words. Otherwise a normal game records the typing speed and the accuracy of the game as
// well, and an abyss game does so only once it reached a score of 400 and 25 (seconds) in mStats (what GetStats saw last);
// a shorter abyss game only moves the depths. theFlag says whether the typing speed goes into the profile's list too.
void Board::SaveProgress(bool theFlag, bool theWithDouble)
{
	int aLevel = mPlayer->mField114;
	if (aLevel != 0 || mPlayer->mField110 >= 0 || mLessonManager->mAbyssMode)
	{
		bool anAbyss = mLessonManager->mAbyssMode;
		if (!anAbyss || (mScore >= 400 && mStats.mReserved[7] >= 25))
		{
			int aSpeed = GetTypingSpeed();
			if (aSpeed < 0)
				aSpeed = 0;
			int anAccuracy = Ftol((double)mField0f68 / (double)(mField0f6c + mField0f68) * 100.0);

			if (!anAbyss)
			{
				// the normal modes: the group is the difficulty
				int aDifficulty = mApp->mDifficulty;
				mApp->mUser->SetLastGame(aDifficulty, mPlayer->mScore, aLevel + 1);
				mApp->mUser->SetGroupField8(mApp->mDifficulty, mPlayer->mField114);
				AddRecordValue8(mApp->mUser, mApp->mDifficulty, mPlayer->mField114, aSpeed);
				AddRecordValue4(mApp->mUser, mApp->mDifficulty, mPlayer->mField114, anAccuracy);
				if (mField0f30 > 0)
					AddRecordValue0(mApp->mUser, mApp->mDifficulty, mPlayer->mField114, mField0f30);
			}
			else
			{
				// the abyss mode: group 5, one record
				UserProfile* aUser = mApp->mUser;
				if (mScore > aUser->mBestDepth)
					aUser->mBestDepth = mScore;
				aUser->mLastDepth = mScore;
				mApp->mUser->SetLastGame(5, mPlayer->mScore, aLevel + 1);
				mApp->mUser->SetGroupField8(5, mScore);
				AddRecordValue8(mApp->mUser, 5, 0, aSpeed);
				AddRecordValue4(mApp->mUser, 5, 0, anAccuracy);
			}
			if (mPlayer->mField110 >= 0 && theFlag)
				mApp->mUser->AddWPM(aSpeed);
		}
		else
		{
			// an abyss game too short for the lists: the depths only
			UserProfile* aUser = mApp->mUser;
			if (mScore > aUser->mBestDepth)
				aUser->mBestDepth = mScore;
			aUser->mLastDepth = mScore;
			mApp->mUser->SetLastGame(5, mPlayer->mScore, aLevel + 1);
			mApp->mUser->SetGroupField8(5, mScore);
		}
	}
	else
	{
		// the short way
		mApp->mUser->SetLastGame(mApp->mDifficulty, mPlayer->mScore, aLevel + 1);
		mApp->mUser->SetGroupField8(mApp->mDifficulty, mPlayer->mField114);
		if (mField0f30 > 0)
			AddRecordValue0(mApp->mUser, mApp->mDifficulty, mPlayer->mField114, mField0f30);
	}

	mApp->mProfileMgr->WriteUserProfileToRegistry(mApp->mUser, theWithDouble, false);
}

// 0x442100: pauses (the music of the sea goes silent, the status bar hides its buttons, the pause timer starts) or resumes
// (everything that counts the game's time is told how long the pause lasted, so that it does not count it). Pauses nest: the
// game runs again when the last of them is lifted. A diver in the bonus-level state 6 loses and regains the continue link; a
// diver in state 7 or 0xc cannot be paused.
void Board::SetPaused(bool thePaused)
{
	if (thePaused)
	{
		mApp->SetCursor(0);
		int aState = mPlayer->mState;
		if (aState == 6)
		{
			if (mLessonManager->IsBonusLevel())
				mContinueLink->SetVisible(false);
		}
		else if (aState == 7 || aState == 0xc)
			return;

		if (!mPaused)
			mPauseTimer->Reset();
		mPaused = true;
		if (mPauseCount == 0)
			mApp->mSoundInstance38->SetVolume(0.0);
		mPauseCount++;

		StatusBar* aStatusBar = mApp->mStatusBar;
		if (aStatusBar != NULL)
		{
			aStatusBar->HideButtons();
			mApp->mStatusBar->ShowButton(false);
			MarkDirty();
		}
		MarkDirty();
	}
	else
	{
		mPauseCount--;
		if (mPauseCount == 0)
		{
			if (mLessonManager->IsBonusLevel() && mPlayer->mState == 6)
				mContinueLink->SetVisible(true);
			mApp->mSoundInstance38->SetVolume(1.0);
			mPlayer->SetVisible(true);
			mPauseCount = 0;
			mPaused = false;
			int aPauseTime = mPauseTimer->Elapsed();
			mPausedTime += aPauseTime;
			mTimer0f00->mPausedTotal += aPauseTime;
			mBannerTimer->mPausedTotal += aPauseTime;
			mTimer0e50->mPausedTotal += aPauseTime;
			mTypeTimer->mPausedTotal += aPauseTime;
			mRatingTimer->mPausedTotal += aPauseTime;
			mGameTimer->mPausedTotal += aPauseTime;

			StatusBar* aStatusBar = mApp->mStatusBar;
			if (aStatusBar != NULL)
			{
				aStatusBar->ShowButton(true);
				mApp->mStatusBar->MouseLeave();
			}
		}
		if (mPauseCount < 0)
			mPauseCount = 0;
	}
}

// 0x451cb0: takes the lesson that now runs: the diver's speed and the ocean depth of its level, and builds the depth ruler
// down the left edge: a tick every 10 pixels (a label with the depth every 20 ticks, a short tick every 4) for 600 pixels.
// An abyss game starts the diver's mField110 at 0 and freezes his score-driven growth (mFlag13c). The y of the ruler's
// last mark (mField0e10) and of the three scrolling pictures of the sea (mField0d20, mField0d18, mField0d10) follow from
// the ocean depth; an endless game (-1) has none.
void Board::SetLessonManager(LessonManager* theLessonManager)
{
	mLessonManager = theLessonManager;
	mDiverSpeed = theLessonManager->GetLevelSettings()->mDiverSpeed;
	if (mLessonManager->mAbyssMode)
	{
		mPlayer->mField110 = 0;
		mPlayer->mFlag13c = true;
	}
	mPlayer->SetVelY(1.0);
	mOceanDepth = mLessonManager->GetLevelSettings()->mOceanDepth;

	// the old markers (a level that follows another one)
	size_t aMarkerIndex = 0;
	while (aMarkerIndex < mDepthMarkers.size())
	{
		delete mDepthMarkers[aMarkerIndex];
		aMarkerIndex++;
	}
	mDepthMarkers.clear();

	ImageFont* aFont = mApp->mFontMyriadCondensed12Bold;
	int aDepth = 0;
	int anOffset = 0;
	while (anOffset < 600)
	{
		std::string aText;
		bool aMakeMarker = false;
		if (aDepth % 20 == 0)
		{
			aText = StrFormat(GTC(0x00530a6c), aDepth);			// "- %d"
			aMakeMarker = true;
		}
		else if (aDepth % 4 == 0)
		{
			aText = "-";											// 0x530a68, the short tick (not in GameTextTable.inc)
			aMakeMarker = true;
		}
		if (aMakeMarker)
		{
			DepthMarker* aMarker = new DepthMarker();
			aMarker->mText = aText;
			aMarker->mDepth = aDepth;
			aMarker->mX = 0.0;
			aMarker->mY = (double)(aFont->GetHeight() + mSurfaceHeight + anOffset);
			mDepthMarkers.push_back(aMarker);
		}
		aDepth++;
		anOffset += 10;
	}

	if (mOceanDepth == -1)
	{
		mField0e10 = -1.0;
		mField0d20 = -1.0;
		mField0d18 = -1.0;
		mField0d10 = -1.0;
	}
	else
	{
		mField0e10 = (double)(aFont->GetHeight() + (mOceanDepth * 10 - mApp->mImages[IMAGE_OCEANFLOOR]->mHeight) + mSurfaceHeight);
		mField0d20 = mField0e10 - 30.0;
		mField0d18 = mField0d20 + 150.0;
		mField0d10 = mField0d18 + 150.0;
	}
}

// 0x452cf0: starts an abyss game at a depth (WinWSApp passes the multiple of 400 the player chose): the score is the depth,
// the diver's EMP charge is what the profile's ad value gives for it, the ruler is rebuilt around the depth (32 ticks
// above it, 16 below), the lesson is taken forward one level for every 200 units of the depth, and the diver is put just
// below the surface. A depth that is a multiple of 1000 sets mFlag0d28, which makes SpawnWave build an abyss boss.
void Board::SetDepth(int theDepth)
{
	mScore = theDepth;
	mField1c0 = theDepth;
	mPlayer->mEmpCharge = mApp->mUser->GetAdValue(theDepth);

	// port: the original forgets the old markers without deleting them (a leak)
	size_t aMarkerIndex = 0;
	while (aMarkerIndex < mDepthMarkers.size())
	{
		delete mDepthMarkers[aMarkerIndex];
		aMarkerIndex++;
	}
	mDepthMarkers.clear();

	int aDepth = theDepth - 32;
	int anOffset = 0;
	while (aDepth < theDepth + 16)
	{
		anOffset = (aDepth - theDepth + 32) * 10;
		std::string aText;
		bool aMakeMarker = false;
		if (aDepth % 20 == 0)
		{
			aText = StrFormat(GTC(0x00530a6c), aDepth);			// "- %d"
			aMakeMarker = true;
		}
		else if (aDepth % 4 == 0)
		{
			aText = "-";											// 0x530a68, the short tick (not in GameTextTable.inc)
			aMakeMarker = true;
		}
		if (aMakeMarker)
		{
			DepthMarker* aMarker = new DepthMarker();
			aMarker->mText = aText;
			aMarker->mDepth = aDepth;
			aMarker->mX = 0.0;
			aMarker->mY = (double)anOffset;
			mDepthMarkers.push_back(aMarker);
		}
		aDepth++;
	}

	if (theDepth > 0)
	{
		unsigned int aLevels = (unsigned int)(theDepth - 1) / 200 + 1;
		do
		{
			mLessonManager->NextLevel();
			aLevels--;
		}
		while (aLevels != 0);
	}

	mDivePos = (double)(mSurfaceHeight + 1);
	int aThousands = mScore / 1000;
	if (aThousands > 0)
	{
		mField0d2c += aThousands;
		if (mScore % 1000 == 0)
			mFlag0d28 = true;
	}
	mApp->mUser->SetField1e4(mField0d2c);
}

// 0x452f30: a bubble at the given place
void Board::AddBubble(int theX, int theY)
{
	Bubble* aBubble = new Bubble(mApp);
	aBubble->SetX((double)theX);
	aBubble->SetY((double)theY);
	mBubbles.push_back(aBubble);
}

// 0x452fd0: a floating text at the given place (the points of a word), in the given font or the app's large one
void Board::AddBonusText(const std::string& theText, int theX, int theY, Font* theFont)
{
	if (theFont == NULL)
		theFont = mApp->mFontUnsteadyOversteer22;
	BonusText* aText = new BonusText(theText, theFont);
	aText->SetX((double)theX);
	aText->SetY((double)theY);
	mBonusTexts.push_back(aText);
}
