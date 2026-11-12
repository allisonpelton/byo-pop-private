// Profile.cpp: the profiles of Bejeweled 2. See Profile.h.
//
// A profile is the folder users/<name>/ in the user's data folder; its profile.dat holds the progress that the
// profile keeps (the puzzle packs' progress, the counters of the Board's end of game, the turned-off hints).
// The saved game of a profile sits in the same folder (the Board writes it).

#include "Profile.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "Buffer.h"
#include "Common.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

using namespace Sexy;

static std::string ProfileFolder(const std::string& theName)
{
	return GetAppDataFolder() + "users/" + theName;
}

std::string Profile::GetFolder() const
{
	return ProfileFolder(mName);
}

// the original compares names with _stricmp
static bool EqualNoCase(const std::string& a, const std::string& b)
{
	return Lower(a) == Lower(b);
}

// 0x5d0716 (the original's constructor only makes the roster, the name, the progress map and the pack name; the port also
// zeroes the counters that the original leaves unset)
Profile::Profile()
{
	mIsNew = false;
	mFlag78 = false;
	mTotal7C = 0;
	mHints = 0;
	for (int i = 0; i < NUM_STATS; i++)
		mStat[i] = 0;
	for (int i = 0; i < NUM_MODE_LEVELS; i++)
		mModeLevel[i] = 0;
}

// 0x5d07cd: the profiles are the folders of users/
void Profile::ReadProfileList()
{
	mNames.clear();
	std::vector<std::string> anEntries = Platform::ListDirectory(GetAppDataFolder() + "users");
	// FindFirstFile lists a folder in the order of the names, which the platform's listing does not promise
	std::sort(anEntries.begin(), anEntries.end(), [](const std::string& a, const std::string& b) { return Lower(a) < Lower(b); });
	for (size_t i = 0; i < anEntries.size(); i++)
	{
		if (Platform::IsDirectory(GetAppDataFolder() + "users/" + anEntries[i]))
			ListAddName(anEntries[i]);
	}
}

// 0x5d075d
bool Profile::ListAddName(const std::string& theName)
{
	for (int i = 0; i < (int) mNames.size(); i++)
	{
		if (EqualNoCase(mNames[i], theName))
			return false;
	}
	mNames.push_back(theName);
	return true;
}

// 0x5d017c: the loop goes on at the next index after an erase, as the original's does
bool Profile::ListRemoveName(const std::string& theName)
{
	bool aRemoved = false;
	for (int i = 0; i < (int) mNames.size(); i++)
	{
		if (EqualNoCase(mNames[i], theName))
		{
			mNames.erase(mNames.begin() + i);
			aRemoved = true;
		}
	}
	return aRemoved;
}

// 0x5d0bd1
bool Profile::LoadProfile(const std::string& theName)
{
	bool aLoaded = LoadProfileHelper(theName);
	if (!aLoaded)
		ListRemoveName(theName);
	return aLoaded;
}

// 0x5d08cb: the profile that was loaded is written first
bool Profile::LoadProfileHelper(const std::string& theName)
{
	WriteProfile();

	std::string aFileName = ProfileFolder(theName) + "/profile.dat";
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(aFileName, &aBuffer, false))
		return false;

	if (aBuffer.ReadLong() != PROFILE_MAGIC)
		return false;
	if (aBuffer.ReadLong() < PROFILE_VERSION)
		return false;

	mName = theName;
	mProgress.clear();
	mPuzzlePack = aBuffer.ReadString();
	short aNumPacks = aBuffer.ReadShort();
	for (int aPack = aNumPacks; aPack > 0; aPack--)
	{
		std::string aPackName = aBuffer.ReadString();
		PuzzleProgress aProgress;
		for (int aCount = aBuffer.ReadShort(); aCount > 0; aCount--)
			aProgress.mSolved.push_back(aBuffer.ReadByte());
		aProgress.mPuzzleNum = aBuffer.ReadShort();
		mProgress[aPackName] = aProgress;
	}

	mIsNew = false;
	for (int i = 0; i < NUM_STATS; i++)
		mStat[i] = aBuffer.ReadLong();
	mFlag78 = aBuffer.ReadBoolean();
	mHints = aBuffer.ReadLong();
	mTotal7C = aBuffer.ReadLong();
	for (int i = 0; i < NUM_MODE_LEVELS; i++)
		mModeLevel[i] = aBuffer.ReadLong();

	ListAddName(mName);
	return aBuffer.AtEnd() && !aBuffer.PastEnd();
}

// 0x5cfb45
bool Profile::WriteProfile()
{
	if (mName.empty())
		return false;

	std::string aFileName = ProfileFolder(mName) + "/profile.dat";
	MkDir(GetFileDir(aFileName));

	Buffer aBuffer;
	aBuffer.WriteLong(PROFILE_MAGIC);
	aBuffer.WriteLong(PROFILE_VERSION);
	aBuffer.WriteString(mPuzzlePack);
	aBuffer.WriteShort((short) mProgress.size());
	for (std::map<std::string, PuzzleProgress>::iterator anItr = mProgress.begin(); anItr != mProgress.end(); ++anItr)
	{
		aBuffer.WriteString(anItr->first);
		aBuffer.WriteShort((short) anItr->second.mSolved.size());
		if (!anItr->second.mSolved.empty())
			aBuffer.WriteBytes(&anItr->second.mSolved[0], (int) anItr->second.mSolved.size());
		aBuffer.WriteShort(anItr->second.mPuzzleNum);
	}
	for (int i = 0; i < NUM_STATS; i++)
		aBuffer.WriteLong(mStat[i]);
	aBuffer.WriteBoolean(mFlag78);
	aBuffer.WriteLong((long) mHints);
	aBuffer.WriteLong(mTotal7C);
	for (int i = 0; i < NUM_MODE_LEVELS; i++)
		aBuffer.WriteLong(mModeLevel[i]);

	gSexyAppBase->WriteBufferToFile(aFileName, &aBuffer);
	return true;
}

// 0x5d0d76
bool Profile::CreateProfile(const std::string& theName)
{
	WriteProfile();
	if (!ListAddName(theName))
		return false;

	mIsNew = true;
	mName = theName;
	mProgress.clear();
	mPuzzlePack = "Galaxy One";
	for (int i = 0; i < NUM_STATS; i++)
		mStat[i] = 0;
	mFlag78 = false;
	for (int i = 0; i < NUM_MODE_LEVELS; i++)
		mModeLevel[i] = 0;
	mTotal7C = 0;
	mHints = 0;
	return true;
}

// 0x5d0bf8: the profile's folder takes the new name
bool Profile::RenameProfile(const std::string& theOldName, const std::string& theNewName)
{
	WriteProfile();

	std::string aOldFolder = ProfileFolder(theOldName);
	std::string aNewFolder = ProfileFolder(theNewName);
	if (rename(aOldFolder.c_str(), aNewFolder.c_str()) != 0)
		return false;
	ListRemoveName(theOldName);
	ListAddName(theNewName);

	if (EqualNoCase(theOldName, mName))
		mName = theNewName;
	return true;
}

// 0x5d020e
void Profile::DeleteProfile(const std::string& theName)
{
	if (EqualNoCase(theName, mName))
		mName = "";
	if (ListRemoveName(theName))
		Deltree(ProfileFolder(theName));
}

// 0x5d0eae: each name that fails to load leaves the roster, and the loop goes on at the next index
bool Profile::GetAnyProfile()
{
	for (int i = 0; i < (int) mNames.size(); i++)
	{
		if (LoadProfile(mNames[i]))
			return true;
	}
	return false;
}

// 0x5d0df3
PuzzleProgress* Profile::GetPuzzleProgress()
{
	return &mProgress[mPuzzlePack];
}

// 0x5d0ee6
int Profile::GetPuzzleNum()
{
	PuzzleProgress* aProgress = GetPuzzleProgress();
	if (aProgress == NULL)
		return -1;
	return aProgress->mPuzzleNum;
}

// 0x5cf3ae
bool Profile::WantsHint(int theHint) const
{
	return (mHints & (1u << (theHint & 31))) == 0;
}

// 0x5cf3cc
void Profile::DisableHint(int theHint)
{
	mHints |= 1u << (theHint & 31);
}

// 0x5cf3e3: the modes 4 to 7 open with the counters of the profile
bool Profile::IsModeUnlocked(int theMode) const
{
	switch (theMode)
	{
	case 4: return mModeLevel[0] >= 17;
	case 5: return mModeLevel[1] >= 8;
	case 6: return mTotal7C >= 80;
	case 7: return mModeLevel[3] >= 280;
	}
	return true;
}
