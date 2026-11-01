#include "User.h"

#include "App.h"
#include "JBBase.h"

namespace mahjongchina
{

User gUser;

// A player's checksum covers the folder of the exe (0x416100), so the original loads a player only from the install
// that saved it. The port signs with the original's usual install folder, so players move between the original and
// the port whatever folder the port runs from, and it loads a player signed with any of these (DEVIATIONS.md)
static const char* const kSignFolders[] = {
	"C:\\Program Files (x86)\\Popcap Game Collection\\Mahjong Escape Ancient China\\",
	"C:\\Program Files\\Popcap Game Collection\\Mahjong Escape Ancient China\\",
	nullptr,  // the port's own folder, which signed its players before
};
static const char* gSignFolder = kSignFolders[0];

// 0x415f40
void User::Reset(int theId)
{
	mId = theId;
	mAutoHint = true;
	mShadeCovered = true;
	mSelectedLayout = 0;
	mQuadPercent = 0x19;
	mStyleFree = 1;
	mBackground = -1;
	mFreeTipShown = false;
	int k = 0;
	int i = 0x96;
	do
	{
		mBestTimes[k] = 36000;
		mBestScores[k] = 0;
		k++;
		i--;
	} while (i != 0);
	mProgress = 0;
	mStyleEscape = 0;
	k = 0;
	i = 0x34;
	do
	{
		mEscBestTimes[k] = 36000;
		mEscBestScores[k] = 0;
		k++;
		i--;
	} while (i != 0);
	mShowQuitNote = true;
}

// 0x415fc0
void User::Serialize(jb::Stream& theStream)
{
	mMagic = 0x4d45;
	mVersion = 1;
	theStream.Short(mMagic);
	theStream.Char(mVersion);
	theStream.Short(mId);
	theStream.String(mName);
	theStream.Bool(mAutoHint);
	theStream.Bool(mShadeCovered);
	theStream.Short(mSelectedLayout);
	theStream.Short(mQuadPercent);
	theStream.Char(mStyleFree);
	theStream.Char(mBackground);
	theStream.Bool(mFreeTipShown);
	int k = 0;
	int i = 0x96;
	do
	{
		theStream.Word(mBestTimes[k]);
		theStream.Int(mBestScores[k]);
		k++;
		i--;
	} while (i != 0);
	theStream.Short(mProgress);
	theStream.Char(mStyleEscape);
	k = 0;
	i = 0x34;
	do
	{
		theStream.Word(mEscBestTimes[k]);
		theStream.Int(mEscBestScores[k]);
		k++;
		i--;
	} while (i != 0);
	theStream.Bool(mShowQuitNote);
	std::string aFolder = gSignFolder != nullptr ? gSignFolder : jb::GetExeFolder();
	theStream.ChecksumBlock((const uint8_t*)aFolder.c_str(), (int)aFolder.length());
}

// 0x416100
void User::Load(int theId, const char* theName)
{
	jb::Stream aStream;
	std::string aPath;
	GetUserPath(theId, aPath);
	if (!aStream.OpenRead(aPath.c_str()))
		Reset(theId);
	else
	{
		bool isOk = false;
		for (size_t i = 0; !isOk && i < sizeof(kSignFolders) / sizeof(kSignFolders[0]); i++)
		{
			gSignFolder = kSignFolders[i];
			jb::Stream aTry;
			if (aTry.OpenRead(aPath.c_str()))
			{
				aTry.Serialize(this);
				isOk = mId == theId && mMagic == 0x4d45 && mVersion == 1 && aTry.ChecksumOk();
			}
		}
		gSignFolder = kSignFolders[0];
		if (isOk)
			return;
		Reset(theId);
	}
	mName = theName != nullptr ? theName : "";
}

// 0x4161f0
void User::Save()
{
	if (mId != -1)
	{
		jb::Stream aStream;
		std::string aPath;
		GetUserPath(mId, aPath);
		if (aStream.OpenWrite(aPath.c_str()))
			aStream.Serialize(this);
	}
}

// 0x4162a0
void User::GetUserPath(int theId, std::string& thePath)
{
	thePath = jb::Format("%s%d.usr", jb::GetUserFolder().c_str(), theId);
}

// 0x416310
void User::GetSavePattern(int theId, std::string& thePattern, std::string& theFolder)
{
	theFolder = jb::GetUserFolder();
	thePattern = jb::Format("%s%d_*.sav", theFolder.c_str(), theId);
}

// 0x416390
void User::GetFreeSave(int theLevel, std::string& thePath)
{
	thePath = jb::Format("%s%d_%d.sav", jb::GetUserFolder().c_str(), mId, theLevel);
}

// 0x416410
void User::GetEscapeSave(int theLevel, std::string& thePath)
{
	thePath = jb::Format("%s%d_%d_D.sav", jb::GetUserFolder().c_str(), mId, theLevel);
}

} // namespace mahjongchina
