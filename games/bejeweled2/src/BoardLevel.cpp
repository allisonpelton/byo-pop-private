// BoardLevel.cpp: setting up a level of Sexy::Board: the backdrop images, the level's point values, the end of a game
// and the transition between two levels.
//   PrepareLevelImages 0x59becc, SetupLevelData 0x59c11b, PrepareLevelImagesStub 0x5a5024, GameOver 0x5a5030,
//   NewGame 0x5a50a0, UpdateLevelTransition 0x5a510a, StartPrepareLevelImages 0x5a571e, InitialBackdropProcessing 0x5a573d.

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "GalaxyParser.h"
#include "CustomResourceManager.h"
#include "WorkerThread.h"
#include "SoundInstance.h"
#include "MemoryImage.h"
#include "Common.h"

using namespace Sexy;

// 0x59becc: loads the backdrop of the next level (the images of the classic game, the planet's picture of a puzzle) and
// prepares it for the mode: the twilight game mirrors each row and darkens it, the finity game swaps red and blue
void Board::PrepareLevelImages()
{
	delete mNextBackdropImage;
	mNextBackdropImage = NULL;

	CustomResourceManager* aResourceManager = (CustomResourceManager*)mApp->mResourceManager;
	if (!mIsPuzzle)
	{
		std::string aPath = "images\\backdrops\\" + mApp->mBackdrops[mBackdropNum];
		mNextBackdropImage = (MemoryImage*)aResourceManager->GetCachedImage(aPath);
		if (mIsTwilight)
		{
			ulong* aBits = mNextBackdropImage->GetBits();
			int aWidth = mNextBackdropImage->mWidth;
			int aHalf = aWidth / 2;
			ulong* aLeft = aBits;
			for (int aRow = 0; aRow < mNextBackdropImage->mHeight; aRow++)
			{
				ulong* aRight = (ulong*)((char*)aLeft + aWidth * 4 - 4);
				for (int i = 0; i < aHalf; i++)
				{
					ulong aOld = *aLeft;
					*aLeft = (*aRight >> 1 & 0x7f7f7f) | 0xff000000;
					*aRight = (aOld >> 1 & 0x7f7f7f) | 0xff000000;
					aLeft++;
					aRight--;
				}
				aLeft += aHalf;
			}
			mNextBackdropImage->BitsChanged();
		}
		if (mIsFinity)
		{
			ulong* aBits = mNextBackdropImage->GetBits();
			int aCount = mNextBackdropImage->GetWidth() * mNextBackdropImage->GetHeight();
			for (int i = 0; i < aCount; i++)
			{
				ulong aPixel = *aBits;
				*aBits = (aPixel >> 16 & 0xff) | ((aPixel & 0xff) << 16) | (aPixel & 0xff00ff00);
				aBits++;
			}
			mNextBackdropImage->BitsChanged();
		}
		mNextBackdropImage->mPurgeBits = true;
		mNextBackdropImage->PurgeBits();
	}
	else
	{
		mApp->mProfile->GetPuzzleProgress();
		GalaxyParser* aGalaxy = mApp->mGalaxyParser;
		if (m15f6c < aGalaxy->GetPlanetCount())
		{
			mNextBackdropImage = (MemoryImage*)aResourceManager->GetCachedImage(aGalaxy->GetPlanet(m15f6c).mBackdrop);
			mNextBackdropImage->mPurgeBits = true;
			mNextBackdropImage->PurgeBits();
		}
	}
	if (mNextBackdropImage != NULL)
	{
		mNextBackdropImage->mD3DFlags = 1;
		mNextBackdropImage->SetImageMode(false, false);
	}
	m15f6a = false;
}

// 0x5a5024
void Board::PrepareLevelImagesStub(void* theBoard)
{
	((Board*)theBoard)->PrepareLevelImages();
}

// 0x5a571e: the original hands this to its worker thread
void Board::StartPrepareLevelImages()
{
	m15f6a = true;
	mApp->mWorker->DoTask(PrepareLevelImagesStub, this);
}

// 0x5a573d: the first backdrop is made at once and the one after it is made for the next level
void Board::InitialBackdropProcessing()
{
	if (!mIsPuzzle)
	{
		if (mBackdropNum == -1)
		{
			if (!mApp->mProfile->mFlag78)
				mBackdropNum = (unsigned int)mLevel % (unsigned int)mApp->mBackdrops.size();
			else
				mBackdropNum = (unsigned int)Rand() % (unsigned int)mApp->mBackdrops.size();
		}
	}
	else
	{
		m15f6c = mApp->mProfile->GetPuzzleProgress()->mPuzzleNum / 5;
	}
	StartPrepareLevelImages();
	ProcessLevelImages();
	if (!mIsPuzzle)
		mBackdropNum = (mBackdropNum + 1U) % (unsigned int)mApp->mBackdrops.size();
	else
		m15f6c++;
	StartPrepareLevelImages();
	mCoversScreenFlag = true;
}

// 0x59c11b: the points that one cleared gem is worth in this level, and what finishing the level pays
void Board::SetupLevelData()
{
	double anOldValue = mLevelValue;
	m15f50 = true;
	if (!mIsAction)
	{
		double aBonus;
		if (mIsEndless)
		{
			mLevelValue = mApp->mEndlessPMBase;
			aBonus = (double)mLevel * mApp->mEndlessBonusMult + (double)mApp->mEndlessBonus;
		}
		else if (mIsSecret)
		{
			mLevelValue = (double)mLevel * 0.5 + 1.0;
			aBonus = (double)(mLevel * 150 + 500);
		}
		else
		{
			mLevelValue = mApp->mNormPMMult * (double)mLevel + mApp->mNormPMBase;
			aBonus = (double)mApp->mNormBonus + mApp->mNormBonusMult * (double)mLevel;
		}
		m15ef0 = (int)(aBonus * mLevelValue);
	}
	else
	{
		mLevelValue = (double)mLevel * mApp->mTimedPMMult + mApp->mTimedPMBase;
		m15ef0 = (int)(mLevelValue * m15e30);
		m15e30 = m15e38 + m15e30;
		m15e38 = mApp->mTimedBonusAccel + m15e38;
		if (anOldValue == 0.0)
		{
			m15f00 = mApp->GetDouble("TimedPV");
			m15f08 = mApp->GetDouble("TimedPA");
			m15f10 = mApp->GetDouble("TimedPJ");
		}
		else
		{
			double aRatio = mLevelValue / anOldValue;
			m15f00 = aRatio * m15f00;
			m15f08 = m15f08 * aRatio;
			m15f10 = aRatio * m15f10;
			double anAdvance = mApp->GetDouble("TimedLevelAdvance");
			m15f00 = m15f08 * anAdvance + m15f00;
			m15f08 = anAdvance * m15f10 + m15f08;
		}
	}
	m15ef8 = 0.0;
}

// 0x5a5030: the game ends (the gems fall away after theDelay updates)
void Board::GameOver(int theDelay)
{
	DeleteSavedGame();
	mSelectedPiece = NULL;
	m1960 = false;
	mApp->KillDialog(0x25);
	mState = STATE_GAME_OVER_FALL;
	m15e14 = (unsigned int)(mUpdateCnt - mPlayStartTick) / 100 + mPlayedSeconds;
	m15e58 = theDelay;
	ProcessEndGameInfo();
	// (the original counts the game in the trial's counter at app+0x718 here)
}

// 0x5a50a0
void Board::NewGame()
{
	GameOver(0x32);
	m15ebc = true;
}

// 0x5a510a: the screen is swept away and the music fades; when the sweep has covered the board the next level starts
void Board::UpdateLevelTransition()
{
	double aVolume = m15f90 - 0.015;
	m15f90 = aVolume;
	if (0.0 <= aVolume)
	{
		if (m1728 != NULL)
			m1728->SetVolume(m15f90);
	}
	else
	{
		m15f90 = 0.0;
		if (m1728 != NULL)
		{
			m1728->Release();
			m1728 = NULL;
		}
	}
	if (mUpdateCnt % 4 == 0)
		m15f80 = Rand() % 0x80;
	MarkDirty();
	m15f7c += 3;
	if (mHeight < m15f7c)
	{
		mNextLevel = mLevel + 1;
		CreateFontGlowImages();
		m15f88 = 100;
		NewBoard();
		// (the trial version asks for the registration here now and then)
	}
}
