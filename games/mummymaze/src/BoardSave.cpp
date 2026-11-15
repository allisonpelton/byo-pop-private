// The classic game in progress: WinMM.exe 0x410b70 writes data\saves\classic.sav, 0x41c600 reads it, 0x410dd0 deletes it.
//
// The original's file is version 2: the format number (an int 2), the board's random generator (0x432760: an int 0x9c0 and
// its 624 state words), the app's Buffer of the data for the Internet score list (+0x103ec: an int length and its web
// string), then the level, the pyramid, the maze, the file, the score, whether the level was done, the time played and the
// game mode. The port writes and reads the same layout into the user folder's classic.sav, so that a player's own file from
// the original loads (the reader restores the state words into the app's generator at +0x104cc, as 0x41c600 does).

#include "Board.h"
#include "Buffer.h"
#include "Compat2004.h"
#include "GameText.h"
#include "ImageFont.h"
#include "LiveSplitObserver.h"
#include "Screens.h"
#include "MummyApp.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace Sexy;

#include "GameLevels.h"
#include "Tables.inc"

// port: the saved classic game's file in the user folder
static std::string ClassicSavePath()
{
	return "classic.sav";
}

namespace
{
// the bytes of the file, read the way the original's fread calls do, but a short file is an error here
struct SaveReader
{
	const std::string&	mData;
	size_t				mPos;
	bool				mOk;
	SaveReader(const std::string& theData) : mData(theData), mPos(0), mOk(true) {}
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
	int Byte()
	{
		if (mPos + 1 > mData.size())
		{
			mOk = false;
			return 0;
		}
		return (unsigned char) mData[mPos++];
	}
	std::string String()
	{
		int aLen = Int();
		if (!mOk || aLen < 0 || (size_t) aLen > mData.size() - mPos)
		{
			mOk = false;
			return std::string();
		}
		std::string aString = mData.substr(mPos, aLen);
		mPos += aLen;
		return aString;
	}
};

// Buffer::FromWebString (0x434680) reads as many characters as the leading bit count asks for; a damaged file may hold fewer
bool WebStringOk(const std::string& theString)
{
	if (theString.size() < 4)
		return true;
	if (theString.size() < 8)
		return false;
	unsigned aBits = 0;
	for (int i = 0; i < 8; i++)
	{
		char c = theString[i];
		int aVal = 0;
		if (c >= '0' && c <= '9')
			aVal = c - '0';
		else if (c >= 'A' && c <= 'F')
			aVal = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			aVal = c - 'f' + 10;
		aBits = (aBits << 4) + aVal;
	}
	return aBits <= 0x1000000 && theString.size() >= 8 + (aBits + 5) / 6;
}
}

// 0x410b70: not written for an adventure game or when the level is lost
bool Board::SaveGame()
{
	if (mShowingSolution || mApp->mGameMode == MODE_ADVENTURE)
		return false;
	std::string aData;
	auto aInt = [&aData](int v) { aData.append((const char*) &v, 4); };
	auto aString = [&aData, &aInt](const std::string& s) { aInt((int) s.size()); aData += s; };
	aInt(2);
	aString(mRand.Serialize());
	aString(mApp->mScoreBuffer.ToWebString());
	aInt(mApp->mClassicChamber);
	aInt(mApp->mClassicPyramid);
	aInt(mMaze);
	aInt(mFile);
	aInt(mApp->mScore);
	aData.push_back(mState == STATE_DONE ? 1 : 0);
	aInt((int) (mUpdateCnt - mTimeBase));
	aInt(mApp->mGameMode);
	return Platform::WriteUserFileAtomic(ClassicSavePath(), aData);
}

// 0x410dd0
bool Board::DeleteSavedGame()
{
	return Platform::RemoveUserFile(ClassicSavePath());
}

// 0x41ca40
bool MMApp::HasSavedGame()
{
	std::string aData;
	return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + ClassicSavePath(), &aData);
}

// 0x41c600: takes the saved game's numbers over; the next board loads that maze. A level that was done is not played again:
// the game moves on to the next chamber (and pyramid) and picks its file and maze the way a new board would.
bool MMApp::LoadGame()
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + ClassicSavePath(), &aData))
		return false;
	SaveReader aReader(aData);
	if (aReader.Int() != 2)
		return false;
	std::string aRandom = aReader.String();
	std::string aBuffer = aReader.String();
	int aChamber = aReader.Int();
	int aPyramid = aReader.Int();
	int aMaze = aReader.Int();
	int aFile = aReader.Int();
	int aScore = aReader.Int();
	int aDone = aReader.Byte();
	int aTime = aReader.Int();
	int aMode = aReader.Int();
	if (!aReader.mOk || !WebStringOk(aBuffer))
		return false;

	mRand.SRand(aRandom);	// the state words when the size is right (0x4326d0), else the seed 0x1105
	mScoreBuffer.FromWebString(aBuffer);
	mClassicChamber = aChamber;
	mClassicPyramid = aPyramid;
	mRestoreMaze = aMaze;
	mRestoreFile = aFile;
	mRestoring = true;
	mScore = aScore;
	if (mSidePanel != NULL)
		mSidePanel->mScore = mScore;
	if (aDone != 0)
	{
		mClassicChamber++;
		if (mClassicChamber == 0xf)
		{
			mClassicChamber = 0;
			mClassicPyramid++;
		}
		double aSum = (double) mClassicPyramid * (1.0 / 15.0) * 99.0 + (double) mClassicChamber * (1.0 / 15.0) * 7.0;
		if (99.0 < aSum)
			aSum = 99.0;
		mRestoreFile = (int) aSum;
		bool aExcluded;
		do
		{
			aExcluded = false;
			// the first draw only decides whether the second one is made (a number of 100 or more would stay 100)
			int aDraw = abs(CrtRand() % 7);
			int aNumber = (int) ((double) aDraw + (double) mClassicChamber * (1.0 / 15.0) * 99.0 + 1.0);
			if (aNumber > 100)
				aNumber = 100;
			else
			{
				aDraw = abs(CrtRand() % 7);
				aNumber = (int) ((double) aDraw + (double) mClassicChamber * (1.0 / 15.0) * 99.0 + 1.0);
			}
			mRestoreMaze = aNumber;
			for (int i = 0; i < 483; i++)
				if (kExcludedResume[i * 2] == mRestoreFile && kExcludedResume[i * 2 + 1] == aNumber)
					aExcluded = true;
		} while (aExcluded);
	}
	mElapsedBase = aTime;	// signed, as the original's int: a save made before the clock started holds a negative number
	mGameMode = aMode;
	LiveSplitObserver::SavedGameLoaded();   // port(mod): the game that starts next resumes a save (the autosplit block's RESUMED)
	return true;
}
