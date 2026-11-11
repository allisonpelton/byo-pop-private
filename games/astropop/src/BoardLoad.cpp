// Board: LoadLevel (0x4663f0), the start of a stage or of a game: the field is emptied and filled with the first
// rows, from a level file (data\level<n>.xcf) or by the rules of the stage.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "XcfFile.h"
#include "Res.h"
#include <stdlib.h>

using namespace Sexy;

// 0x4663f0 (the trace lines of the original, "Start of level" and "Exiting start next level", are left out)
void Board::LoadLevel(bool theFromRules)
{
	XcfFile aFile;

	mBonusFieldLoaded = false;
	mWarpGateAnim.SetStopped(true);
	ClearField();
	mScriptRowIndex = -1;
	memset(&mScriptRows, 0, sizeof(mScriptRows));
	ResetGame();
	mStreaksFadingOut = true;
	mShipFadeAlpha = 0;
	mApp->SetCursor(CURSOR_POINTER);
	if (!theFromRules)
	{
		for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
				mFieldBricks[aRow][aCol] = NULL;
		}
	}
	else
	{
		std::string aFileName = StrFormat("data\\level%d.xcf", mStageNumber);
		if (mStageNumber == -100)
		{
			// a level from a file (no level file of the game is named like this; the original's path is kept)
			aFile.AddKeyName("DropTime");
			aFile.AddKeyName("LevelUp");
			for (int i = 0; i < 10; i++)
				aFile.AddKeyName(StrFormat("Brick%d", i));
			aFile.Parse(aFileName);
			aFile.EnterFirstChild();
			mLevelFileLevelUp = aFile.GetInt("\\LEVEL\\LevelUp");
			mRowInterval = aFile.GetInt("\\LEVEL\\DropTime") * 100;
			bool aDone = false;
			mScriptRowIndex = -1;
			do
			{
				std::string aName = aFile.GetCurrentName();
				std::string aNumber = aName.size() > 3 ? aName.substr(3) : std::string();
				long aRowNumber = atol(aNumber.c_str());
				if (mScriptRowIndex == -1)
					mScriptRowIndex = (int) aRowNumber;
				for (int i = 0; i < 10; i++)
				{
					long aCode = aFile.GetInt(StrFormat("Brick%d", i));
					if (aRowNumber >= 0 && aRowNumber < 100)
						mScriptRows.mCode[aRowNumber][i] = (int) aCode;
				}
				if (aFile.NextSibling() == XcfFile::CURSOR_END)
					aDone = true;
			}
			while (!aDone);

			int aY = 4;
			for (int aRow = 0; aY < 0x1f8; aRow++)
			{
				int aX = 0xa6;
				for (int aCol = 0; aX < 0x25e; aCol++)
				{
					if (mRowsAtStart < aRow)
						mFieldBricks[aRow][aCol] = NULL;
					else
					{
						int aScriptRow = 17 - aRow;
						int aKind = (aScriptRow >= 0 && aScriptRow < 100) ? mScriptRows.mCode[aScriptRow][aCol] : 0;
						switch (aKind)
						{
						case 0:
							mFieldBricks[aRow][aCol] = NULL;
							break;
						case 1:
						case 2:
						case 3:
						case 4:
							mFieldBricks[aRow][aCol] = CreateBrick(aKind - 1);
							break;
						case 5:
							mFieldBricks[aRow][aCol] = CreateSpecialBrick(7);
							break;
						case 6:
						case 7:
						case 8:
						case 9:
							mFieldBricks[aRow][aCol] = CreateKillAll(aKind - 6);
							break;
						case 10:
							mFieldBricks[aRow][aCol] = CreateSpecialBrick(6);
							break;
						}
						// (the original calls through the cell, which is null for kind 0)
						Brick* aBrick = mFieldBricks[aRow][aCol];
						if (aBrick != NULL)
						{
							aBrick->SetParticleX((float) aX);
							aBrick->SetParticleY((float) (aY > 4 ? aY + 5 : aY));
						}
					}
					aX += 0x2c;
				}
				aY += 0x19;
			}
			mScriptRowIndex = mScriptRowIndex + mRowsAtStart + 1;
			bool aAnyBrick = false;
			for (int i = 0; i < 10; i++)
			{
				if (mScriptRowIndex >= 0 && mScriptRowIndex < 100 && mScriptRows.mCode[mScriptRowIndex][i] != 0)
					aAnyBrick = true;
			}
			if (!aAnyBrick)
				mScriptRowIndex = -1;
		}
		else
		{
			int aStageRows = mRowsAtStart;
			mScriptRowIndex = -1;
			for (int aRow = 0; aRow < 0x14; aRow++)
			{
				if (aStageRows < aRow)
				{
					for (int aCol = 0; aCol < 10; aCol++)
						mFieldBricks[aRow][aCol] = NULL;
				}
				else
					FillTopRow(aRow < aStageRows - 1, aRow == aStageRows, false, mGameType == 2, aRow);
			}
		}
	}

	// the warp stars of the last warp go
	for (unsigned int i = 0; i < mWarpStars.size(); i++)
		delete mWarpStars[i];
	std::vector<WarpStar*>().swap(mWarpStars);
	mShipX = 0x182;
	mWarpFrameInterval = 8;
	mEffectsUpdateCount = 0;
	std::vector<Brick*>().swap(mTargeted);

	// the stage's picture and the first hint start from the beginning
	if ((unsigned int) mStageNumber < mStageHintPanels.size())
	{
		StageBg* aBg = mStageHintPanels[mStageNumber];
		aBg->mPicAlpha = 0xff;
		aBg->mPanelAlpha = 0xff;
		aBg->mCrossfadeTimer = 0;
		aBg->mPanelAlphaStep = 0;
		aBg->mPicAlphaStep = 0;
		aBg->mOverPicAlphaStep = 0;
		aBg->mOverPicAlpha = 0;
		aBg->mFadedOut = 0;
	}
	if (!mHintSpots.empty() && mHintSpots[0]->mActive)
	{
		StageBg* aBg = mHintSpots[0]->mBg;
		aBg->mPicAlpha = 0xff;
		aBg->mPanelAlpha = 0xff;
		aBg->mCrossfadeTimer = 0;
		aBg->mPanelAlphaStep = 0;
		aBg->mPicAlphaStep = 0;
		aBg->mOverPicAlphaStep = 0;
		aBg->mOverPicAlpha = 0;
		aBg->mFadedOut = 0;
	}
	if (mGameType == 1)
	{
		mLevelEndScreenKind = mNextLevelScreenKind;
		mNextLevelScreenKind = UpgradeScreenKind(mLevelNumber + 1);
	}
	Fire(0x2a, 1);
}
