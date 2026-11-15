// Board.cpp, part 6: the saved game (WinSS.exe 0x40b4e0 writes it, 0x40b870 loads it; the original's file is
// data//savegame.dat next to the exe).
//
// The original's file is version 1: two strings with the internet score check's state, then the board's fields
// one by one. The port writes the same fields in the same order into the user folder's savegame.dat, with the
// score check's two strings empty (that check is not ported).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "Compat2004.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cstring>
#include <string>

using namespace Sexy;

namespace
{

class Writer
{
public:
	std::string mData;
	void Int(int v) { mData.append((const char*) &v, 4); }
	void Byte(bool v) { mData.push_back(v ? 1 : 0); }
	void String(const std::string& v)
	{
		Int((int) v.size());
		mData.append(v);
	}
};

class Reader
{
public:
	std::string mData;
	size_t mPos;
	bool mOk;
	Reader() : mPos(0), mOk(true) {}
	int Int()
	{
		int v = 0;
		if (mPos + 4 > mData.size())
			mOk = false;
		else
		{
			memcpy(&v, mData.data() + mPos, 4);
			mPos += 4;
		}
		return v;
	}
	bool Byte()
	{
		if (mPos + 1 > mData.size())
		{
			mOk = false;
			return false;
		}
		return mData[mPos++] != 0;
	}
	std::string String()
	{
		int aLength = Int();
		if (!mOk || (aLength < 0) || (mPos + (size_t) aLength > mData.size()))
		{
			mOk = false;
			return "";
		}
		std::string aResult = mData.substr(mPos, aLength);
		mPos += aLength;
		return aResult;
	}
};

}

// port: the saved game's file name
std::string Board::SavePath()
{
	return "savegame.dat";
}

// 0x40b4e0: the game in progress (not written once the high score list is up)
bool Board::SaveGame()
{
	if (mApp->mHighScoreWidget != NULL)
		return false;
	Writer aWriter;
	aWriter.Int(1);
	aWriter.String("");
	aWriter.String("");
	aWriter.Int(mScore);
	aWriter.Int(mKills);
	aWriter.Int((int) mShips.size());
	for (std::list<Ship*>::iterator anItr = mShips.begin(); anItr != mShips.end(); ++anItr)
	{
		Ship* aShip = *anItr;
		aWriter.Int(aShip->mX);
		aWriter.Int(aShip->mY);
		aWriter.Int(aShip->mHeading);
		aWriter.Int(aShip->mType);
	}
	aWriter.Int(mPlayerX);
	aWriter.Int(mPlayerY);
	aWriter.Int(mHeading);
	for (std::list<Port*>::iterator anItr = mPorts.begin(); anItr != mPorts.end(); ++anItr)
		aWriter.Byte((*anItr)->mActive);
	for (int i = 0; i < GRID; i++)
		for (int j = 0; j < GRID; j++)
		{
			aWriter.Byte(mVisited[i][j]);
			aWriter.Int(mTerrain[i][j]);
		}
	aWriter.Int((int) mStartTick);
	aWriter.Int(mSeconds);
	aWriter.Int(mLevel);
	aWriter.Int(mSavedLevels);
	aWriter.Int(mLives);
	aWriter.Int(mKillsTotal);
	aWriter.Int(mTreasures);
	aWriter.Byte(mApp->mTutorial);
	return Platform::WriteUserFileAtomic(SavePath(), aWriter.mData);
}

// 0x40b850
bool Board::DeleteSave()
{
	return Platform::RemoveUserFile(SavePath());
}

// 0x40b870: takes the saved game back (and deletes the file)
void Board::LoadGame()
{
	Reader aReader;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + SavePath(), &aReader.mData))
		return;
	if (aReader.Int() != 1)
		return;
	aReader.String();
	aReader.String();
	int aScore = aReader.Int();
	int aKills = aReader.Int();
	int aNumShips = aReader.Int();
	if (!aReader.mOk || (aNumShips < 0) || (aNumShips > GRID * GRID))
		return;
	mScore = aScore;
	if (mApp->mSidePanel != NULL)
		mApp->mSidePanel->mShownScore = mScore;   // the panel does not count up from 0
	mKills = aKills;
	ClearShips();
	for (int i = 0; i < aNumShips; i++)
	{
		Ship* aShip = new Ship();
		aShip->mMoveProgress = 0;
		aShip->mHeading = 8;
		aShip->mWantHeading = 8;
		aShip->mAnim = CrtRand() % 8;
		aShip->mX = aReader.Int();
		aShip->mY = aReader.Int();
		aShip->mHeading = aReader.Int();
		aShip->mType = aReader.Int();
		// port: the original leaves the next cell and the wake timers as they were allocated
		aShip->mNextX = aShip->mX;
		aShip->mNextY = aShip->mY;
		for (int j = 0; j < 9; j++)
			aShip->mTrail[j] = -1;
		mShips.push_back(aShip);
	}
	mPlayerX = aReader.Int();
	mPlayerY = aReader.Int();
	mHeading = aReader.Int();
	ClearPorts();
	for (int aPortX = 0; aPortX < GRID; aPortX += 10)
	{
		for (int aPortY = 0; aPortY < GRID; aPortY += 10)
		{
			Port* aPort = new Port();
			aPort->mX = aPortX;
			aPort->mY = aPortY;
			aPort->mActive = aReader.Byte();
			aPort->mFrame = 0;
			aPort->mAppear = 0;
			mPorts.push_back(aPort);
		}
	}
	for (int i = 0; i < GRID; i++)
		for (int j = 0; j < GRID; j++)
		{
			mVisited[i][j] = aReader.Byte();
			mTerrain[i][j] = aReader.Int();
		}
	mStartTick = (DWORD) aReader.Int();
	mSeconds = aReader.Int();
	mLevel = aReader.Int();
	mSavedLevels = aReader.Int();
	mLives = aReader.Int();
	mKillsTotal = aReader.Int();
	mTreasures = aReader.Int();
	mApp->mTutorial = aReader.Byte();
	DeleteSave();
	mTutorialShown = false;
}
