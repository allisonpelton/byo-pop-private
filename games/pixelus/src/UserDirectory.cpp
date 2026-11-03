#include "UserDirectory.h"
#include "PixelusApp.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "Temples.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

static const long USER_VERSION = 4;			// 0x53df8c

UserProfile::UserProfile(PixelusApp* theApp)
{
	mApp = theApp;
	mId = 0;
	mCurrent = false;
	mFlag2D = true;
	mFlag38 = false;
	memset(mFlags, 0, sizeof(mFlags));
	mFlag79 = false;
	mFlag7A = false;
}

// 0x4383a0: the records of the puzzles the temple holds
int UserProfile::CountSolved(const std::string& theLevel) const
{
	const LevelData::Level* aLevel = mApp->mLevelData->GetLevel(theLevel);
	if (aLevel == NULL)
		return 0;
	int aCount = 0;
	for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
		if (mRecords.find(aLevel->mPuzzles[i].mUuid) != mRecords.end())
			aCount++;
	return aCount;
}

// 0x437f50: no more moves than the best known solution
bool UserProfile::IsMaxed(const PuzzleRecord& theRecord, const std::string& theUuid) const
{
	return theRecord.mMoves <= mApp->mProgress->GetBestMoves(theUuid);
}

// 0x438310
int UserProfile::CountMaxed(const std::string& theLevel) const
{
	const LevelData::Level* aLevel = mApp->mLevelData->GetLevel(theLevel);
	if (aLevel == NULL)
		return 0;
	int aCount = 0;
	for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
	{
		std::map<std::string, PuzzleRecord>::const_iterator anItr = mRecords.find(aLevel->mPuzzles[i].mUuid);
		if (anItr != mRecords.end() && IsMaxed(anItr->second, anItr->first))
			aCount++;
	}
	return aCount;
}

// 0x439340: over all temples
int UserProfile::CountSolved() const
{
	int aCount = 0;
	for (std::map<std::string, LevelData::Level>::const_iterator anItr = mApp->mLevelData->mLevels.begin();
		anItr != mApp->mLevelData->mLevels.end(); ++anItr)
		aCount += CountSolved(anItr->first);
	return aCount;
}

const PuzzleRecord* UserProfile::GetRecord(const std::string& theUuid) const
{
	std::map<std::string, PuzzleRecord>::const_iterator anItr = mRecords.find(UuidKey(theUuid));
	return anItr == mRecords.end() ? NULL : &anItr->second;
}

// 0x437f10 (the first result) and 0x438f30: later results only improve the record
void UserProfile::SetRecord(const std::string& theUuid, int theSolved, int theMoves, int theMoves2)
{
	const std::string aKey = UuidKey(theUuid);
	std::map<std::string, PuzzleRecord>::iterator anItr = mRecords.find(aKey);
	if (anItr == mRecords.end())
	{
		PuzzleRecord aRecord;
		aRecord.mSolved = theSolved;
		aRecord.mMoves2 = theMoves2;
		aRecord.mMoves = theMoves;
		mRecords[aKey] = aRecord;
		return;
	}
	PuzzleRecord& aRecord = anItr->second;
	aRecord.mSolved = std::max(aRecord.mSolved, theSolved);
	aRecord.mMoves = std::min(aRecord.mMoves, theMoves);
	aRecord.mMoves2 = std::min(aRecord.mMoves2, theMoves2);
}

// the original's strings in these files: a four-byte length, then the characters
static void WriteText(Buffer* theBuffer, const std::string& theString)
{
	theBuffer->WriteLong((long) theString.size());
	theBuffer->WriteBytes((const uchar*) theString.data(), (int) theString.size());
}

static std::string ReadText(const Buffer* theBuffer)
{
	long aLength = theBuffer->ReadLong();
	if (aLength < 0 || aLength > 255)
		return "";
	char aText[256];
	theBuffer->ReadBytes((uchar*) aText, (int) aLength);
	return std::string(aText, aLength);
}

// the original keeps the records of a temple in a map by UUID: ordered by the first four bytes as a signed number
static bool UuidBefore(const std::string& theA, const std::string& theB)
{
	int aA = (int) strtoul(theA.substr(0, 8).c_str(), NULL, 16), aB = (int) strtoul(theB.substr(0, 8).c_str(), NULL, 16);
	return aA != aB ? aA < aB : theA < theB;
}

// 0x4391c0 (and 0x438450 for each temple): version 4, the player, then the temples that have something to keep
void UserProfile::Write(Buffer* theBuffer) const
{
	theBuffer->WriteLong(USER_VERSION);
	WriteText(theBuffer, mName);
	theBuffer->WriteLong(mId);
	theBuffer->WriteBoolean(mCurrent);
	theBuffer->WriteBoolean(mFlag2D);
	theBuffer->WriteBoolean(mFlag38);
	for (int i = 0; i < NUM_FLAGS; i++)
		theBuffer->WriteByte(mFlags[i]);
	theBuffer->WriteBoolean(mFlag79);
	theBuffer->WriteBoolean(mFlag7A);

	std::vector<std::string> aUuids[NUM_TEMPLES];
	int aTemples = 0;
	for (int t = 0; t < NUM_TEMPLES; t++)
	{
		const LevelData::Level* aLevel = mApp->mLevelData->GetLevel(gTemples[t].mName);
		if (aLevel != NULL)
			for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
				if (mRecords.find(aLevel->mPuzzles[i].mUuid) != mRecords.end())
					aUuids[t].push_back(aLevel->mPuzzles[i].mUuid);
		std::sort(aUuids[t].begin(), aUuids[t].end(), UuidBefore);
		std::map<std::string, int>::const_iterator aFlags = mTempleFlags.find(gTemples[t].mName);
		if (!aUuids[t].empty() || (aFlags != mTempleFlags.end() && aFlags->second != 0))
			aTemples++;
	}

	theBuffer->WriteLong(aTemples);
	for (int t = 0; t < NUM_TEMPLES; t++)
	{
		std::map<std::string, int>::const_iterator aFlags = mTempleFlags.find(gTemples[t].mName);
		int aBits = aFlags != mTempleFlags.end() ? aFlags->second : 0;
		if (aUuids[t].empty() && aBits == 0)
			continue;
		theBuffer->WriteLong(t);
		theBuffer->WriteLong((long) aUuids[t].size());
		theBuffer->WriteBoolean((aBits & 1) != 0);
		theBuffer->WriteBoolean((aBits & 2) != 0);
		theBuffer->WriteBoolean((aBits & 4) != 0);
		for (size_t i = 0; i < aUuids[t].size(); i++)
		{
			const PuzzleRecord& aRecord = mRecords.find(aUuids[t][i])->second;
			WriteText(theBuffer, aUuids[t][i]);
			theBuffer->WriteLong(IsMaxed(aRecord, aUuids[t][i]) ? 1 : 0);
			theBuffer->WriteLong(aRecord.mMoves);
			theBuffer->WriteLong(aRecord.mMoves2);
		}
	}
}

// 0x439fb0: the fields a version has (it adds the flags one by one), then the temples with their records
bool UserProfile::Read(const Buffer* theBuffer)
{
	theBuffer->SeekFront();
	if (theBuffer->GetDataLen() < 8)
		return false;
	int aVersion = theBuffer->ReadLong();
	if (aVersion < 0 || aVersion > 100)
		return false;
	mName = ReadText(theBuffer);
	mId = theBuffer->ReadLong();
	mCurrent = theBuffer->ReadBoolean();
	mFlag2D = theBuffer->ReadBoolean();
	if (aVersion > 0)
		mFlag38 = theBuffer->ReadBoolean();
	if (aVersion > 1)
		for (int i = 0; i < NUM_FLAGS; i++)
			mFlags[i] = theBuffer->ReadByte();
	if (aVersion > 2)
		mFlag79 = theBuffer->ReadBoolean();
	if (aVersion > 3)
		mFlag7A = theBuffer->ReadBoolean();

	mTempleFlags.clear();
	mRecords.clear();
	int aTemples = theBuffer->ReadLong();
	for (int t = 0; t < aTemples && !theBuffer->AtEnd(); t++)
	{
		int aKey = theBuffer->ReadLong();
		int aCount = theBuffer->ReadLong();
		bool aVisited = theBuffer->ReadBoolean();
		bool aSecond = theBuffer->ReadBoolean();
		bool aThird = theBuffer->ReadBoolean();
		for (int i = 0; i < aCount && !theBuffer->AtEnd(); i++)
		{
			std::string aUuid = ReadText(theBuffer);
			int aSolved = theBuffer->ReadLong();
			int aMoves = theBuffer->ReadLong();
			int aMoves2 = theBuffer->ReadLong();
			SetRecord(aUuid, aSolved, aMoves, aMoves2);
		}
		if (aKey < 0 || aKey >= NUM_TEMPLES)
			continue;
		int& aBits = mTempleFlags[gTemples[aKey].mName];
		if (aSecond)
			aBits |= 2;
		if (aThird)
		{
			aBits |= 4;
			if (aVisited)
				aBits |= 1;
		}
	}
	return true;
}

// 0x43da10: the directory lists the players' numbers one per line; the player flagged as current plays
UserDirectory::UserDirectory(PixelusApp* theApp)
{
	mApp = theApp;
	mCurrent = NULL;

	Buffer aDirectory;
	if (mApp->ReadBufferFromFile("users/directory.txt", &aDirectory))
	{
		std::string aText((const char*) aDirectory.GetDataPtr(), aDirectory.GetDataLen());
		std::string aLine;
		for (size_t i = 0; i <= aText.size(); i++)
		{
			char c = i < aText.size() ? aText[i] : '\n';
			if (c == '\r' || c == '\n')
			{
				int anId = atoi(aLine.c_str());
				if (anId > 0)
					mIds.push_back(anId);
				aLine.clear();
			}
			else
				aLine += c;
		}
	}

	for (size_t i = 0; i < mIds.size(); i++)
	{
		char aName[64];
		sprintf(aName, "users/%d", mIds[i]);
		Buffer aBuffer;
		if (!mApp->ReadBufferFromFile(aName, &aBuffer))
			continue;
		UserProfile* aUser = new UserProfile(mApp);
		if (!aUser->Read(&aBuffer))
		{
			delete aUser;
			continue;
		}
		mUsers.push_back(aUser);
		if (aUser->mCurrent && mCurrent == NULL)
			mCurrent = aUser;
	}
}

UserDirectory::~UserDirectory()
{
	for (size_t i = 0; i < mUsers.size(); i++)
		delete mUsers[i];
}

UserProfile* UserDirectory::FindUser(int theId)
{
	for (size_t i = 0; i < mUsers.size(); i++)
		if (mUsers[i]->mId == theId)
			return mUsers[i];
	return NULL;
}

UserProfile* UserDirectory::FindUserByName(const std::string& theName)
{
	for (size_t i = 0; i < mUsers.size(); i++)
		if (strcasecmp(mUsers[i]->mName.c_str(), theName.c_str()) == 0)
			return mUsers[i];
	return NULL;
}

// 0x43b6a0: the one player flagged as current
void UserDirectory::Select(int theId)
{
	mCurrent = NULL;
	for (size_t i = 0; i < mUsers.size(); i++)
	{
		mUsers[i]->mCurrent = mUsers[i]->mId == theId;
		if (mUsers[i]->mCurrent)
			mCurrent = mUsers[i];
	}
	Save();
}

// 0x43c7f0: a new player gets the next free number
UserProfile* UserDirectory::AddUser(const std::string& theName)
{
	int anId = 1;
	for (size_t i = 0; i < mIds.size(); i++)
		anId = std::max(anId, mIds[i] + 1);
	UserProfile* aUser = new UserProfile(mApp);
	aUser->mId = anId;
	aUser->mName = theName;
	mUsers.push_back(aUser);
	mIds.push_back(anId);
	Select(anId);
	return aUser;
}

// 0x43c420
void UserDirectory::DeleteUser(int theId)
{
	UserProfile* aUser = FindUser(theId);
	if (aUser == NULL)
		return;
	bool wasCurrent = aUser == mCurrent;
	mUsers.erase(std::find(mUsers.begin(), mUsers.end(), aUser));
	mIds.erase(std::find(mIds.begin(), mIds.end(), theId));
	delete aUser;

	char aName[64];
	sprintf(aName, "users/%d", theId);
	mApp->EraseFile(aName);
	sprintf(aName, "users/his%d", theId);
	mApp->EraseFile(aName);

	mCurrent = NULL;
	if (wasCurrent && !mUsers.empty())
		Select(mUsers.back()->mId);
	else
		Save();
}

// 0x43d650: false when another player already has the new name
bool UserDirectory::RenameUser(const std::string& theOld, const std::string& theNew)
{
	UserProfile* aUser = FindUserByName(theOld);
	UserProfile* anOther = FindUserByName(theNew);
	if ((anOther != NULL) && (anOther != aUser))
		return false;
	if (aUser != NULL)
		aUser->mName = theNew;
	return true;
}

// 0x43c2a0: every player's file, then the directory
void UserDirectory::Save()
{
	for (size_t i = 0; i < mUsers.size(); i++)
	{
		Buffer aBuffer;
		mUsers[i]->Write(&aBuffer);
		char aName[64];
		sprintf(aName, "users/%d", mUsers[i]->mId);
		mApp->WriteBufferToFile(aName, &aBuffer);
	}
	Buffer aDirectory;
	for (size_t i = 0; i < mIds.size(); i++)
	{
		char aLine[32];
		sprintf(aLine, "%d", mIds[i]);
		aDirectory.WriteLine(aLine);
	}
	mApp->WriteBufferToFile("users/directory.txt", &aDirectory);
#ifdef __EMSCRIPTEN__
	SiteProfilesChanged(mApp);	// the web page's players (framework/WebProfiles.h): every change of the list ends here
#endif
}
