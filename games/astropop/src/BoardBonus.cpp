// Board: the start of a bonus round (0x465840). The field is emptied and filled from the bonus stage's file
// (data\bonus<n>.xcf): a wall of stones on top, then the rows of the file, one brick kind per entry.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "XcfFile.h"
#include "Particle.h"
#include "Res.h"

using namespace Sexy;

// 0x465840. theStage is the number of the file; -1 picks one: in the first thirty levels one of six stages around the
// level, later one of the stages -2 to 30, and a stage is not picked again until thirty have been played.
void Board::StartBonusRound(int theStage)
{
	mBonusFieldLoaded = true;

	if (theStage == -1)
	{
		if (mLevelNumber < 0x1e)
		{
			int aBase = mLevelNumber - 5;
			theStage = ((int) AstroRand() % 500000000) % 6 + aBase;
			while (mPlayedBonusStages.find(theStage) != mPlayedBonusStages.end())
				theStage = ((int) AstroRand() % 500000000) % 6 + aBase;
			if (mPlayedBonusStages.size() > 0x1d)
				mPlayedBonusStages.clear();
		}
		else
		{
			int aDraw = ((int) AstroRand() % 500000000) % 0x21 - 2;
			theStage = aDraw;
			if (mPlayedBonusStages.find(theStage) != mPlayedBonusStages.end())
			{
				// the first stage that has not been played
				for (int aStage = -2; aStage < 0x1f; aStage++)
				{
					if (mPlayedBonusStages.find(aStage) == mPlayedBonusStages.end())
					{
						theStage = aStage;
						aDraw = aStage;
						break;
					}
				}
			}
			if (aDraw > 0x1e)
				theStage = 0x1e;
			if (mPlayedBonusStages.size() > 0x1d)
				mPlayedBonusStages.clear();
		}
	}
	mPlayedBonusStages.insert(theStage);
	mBonusRoundStarted = false;

	// the old field goes (the bricks are deleted later)
	for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL)
			{
				mDoomedBricks.insert(aBrick);
				mFieldBricks[aRow][aCol] = NULL;
			}
		}
	}
	mStartBannerDoneInBonus = false;

	delete mBonusRoundFile;
	mBonusRoundFile = new XcfFile();
	std::string aFileName = StrFormat("data\\bonus%d.xcf", theStage);
	mBonusRoundFile->AddKeyName("Brick");
	int aResult = mBonusRoundFile->Parse(aFileName);
	if (aResult != 0)
	{
		mApp->Popup(StrFormat("There was an error loading the file: %s. Error number: %d, line %d", aFileName.c_str(), aResult, mBonusRoundFile->mLine));
		mApp->mShutdown = true;
		return;
	}

	// the marks of supercharged colours end
	for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
	{
		int anArg = -mSuperchargeMarks[i].mType;
		if (anArg == 0)
			anArg = -500000000;
		Fire(0x20, anArg);
	}
	std::vector<MarkRec>().swap(mSuperchargeMarks);

	mApp->mUser->SetInt(0x3f, theStage);
	m1a80 = 0;
	mBonusRoundLoaded = true;
	m1b74 = 0xff;

	// the top row is a wall of stones
	for (int aCol = 0; aCol < 10; aCol++)
	{
		Brick* aStone = CreateSpecialBrick(7);
		mFieldBricks[0][aCol] = aStone;
		aStone->SetParticleX((float) (0xa6 + 0x2c * aCol));
		aStone->SetParticleY(4.0f);
	}

	// the rows of the file, from the second down; the first entry the file lacks ends it
	int aY = 0x1d;
	for (int aRow = 1; aY < 0x1df; aRow++)
	{
		int aX = 0xa6;
		for (int aCol = 0; aX < 0x25e; aCol++)
		{
			std::string aPath = StrFormat("LEVEL\\ROW%d\\Brick%d", aRow, aCol);
			if (!mBonusRoundFile->HasKey(aPath))
			{
				mApp->mUser->SetInt(100, m1b98);
				return;
			}

			long aKind = mBonusRoundFile->GetInt(aPath);
			if (aKind != -1)
			{
				Brick* aBrick = NULL;
				switch (aKind)
				{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
					aBrick = CreateBrick((int) aKind);
					break;
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
					aBrick = CreateKillAll((int) aKind - 5);
					break;
				case 10:
					aBrick = CreatePowerBrick(0xd);
					break;
				case 0xb:
					aBrick = CreatePowerBrick(10);
					break;
				case 0xc:
					aBrick = CreateSpecialBrick(0xb);
					break;
				case 0xd:
					aBrick = CreateSpecialBrick(6);
					break;
				case 0xe:
					aBrick = CreatePowerBrick(0xe);
					break;
				case 0xf:
					aBrick = CreateFuelCell(500, 5);
					break;
				case 0x10:
					aBrick = CreateSpecialBrick(7);
					break;
				case 0x11:
					aBrick = CreateSpecialBrick(9);
					break;
				case 0x12:
					aBrick = CreateMutator();
					FireTo(aBrick, 0x28, 1);
					break;
				}
				if (aBrick == NULL)     // (a kind the files do not hold: the original would crash here)
					return;
				aBrick->SetParticleX((float) aX);
				aBrick->SetParticleY((float) (aY > 4 ? aY + 5 : aY));
				mFieldBricks[aRow][aCol] = aBrick;
				aBrick->SetVisible(false);
				mBricksRevealing = true;
			}
			aX += 0x2c;
		}
		aY += 0x19;
	}
	mApp->mUser->SetInt(100, m1b98);
}
