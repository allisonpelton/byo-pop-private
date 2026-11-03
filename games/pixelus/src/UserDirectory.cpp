#include "UserDirectory.h"
#include "PixelusApp.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

using namespace Sexy;

static const long USER_MAGIC = 0x31555850;		// "PXU1"

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

// 0x4391c0
void UserProfile::Write(Buffer* theBuffer) const
{
	theBuffer->WriteLong(USER_MAGIC);
	theBuffer->WriteString(mName);
	theBuffer->WriteLong(mId);
	theBuffer->WriteBoolean(mCurrent);
	theBuffer->WriteBoolean(mFlag2D);
	theBuffer->WriteBoolean(mFlag38);
	for (int i = 0; i < NUM_FLAGS; i++)
		theBuffer->WriteByte(mFlags[i]);
	theBuffer->WriteBoolean(mFlag79);
	theBuffer->WriteBoolean(mFlag7A);

	theBuffer->WriteLong((long) mTempleFlags.size());
	for (std::map<std::string, int>::const_iterator anItr = mTempleFlags.begin(); anItr != mTempleFlags.end(); ++anItr)
	{
		theBuffer->WriteString(anItr->first);
		theBuffer->WriteLong(anItr->second);
	}

	theBuffer->WriteLong((long) mRecords.size());
	for (std::map<std::string, PuzzleRecord>::const_iterator anItr = mRecords.begin(); anItr != mRecords.end(); ++anItr)
	{
		theBuffer->WriteString(anItr->first);
		theBuffer->WriteLong(anItr->second.mSolved);
		theBuffer->WriteLong(anItr->second.mMoves2);
		theBuffer->WriteLong(anItr->second.mMoves);
	}
}

// 0x439fb0
bool UserProfile::Read(const Buffer* theBuffer)
{
	theBuffer->SeekFront();
	if (theBuffer->GetDataLen() < 8 || theBuffer->ReadLong() != USER_MAGIC)
		return false;
	mName = theBuffer->ReadString();
	mId = theBuffer->ReadLong();
	mCurrent = theBuffer->ReadBoolean();
	mFlag2D = theBuffer->ReadBoolean();
	mFlag38 = theBuffer->ReadBoolean();
	for (int i = 0; i < NUM_FLAGS; i++)
		mFlags[i] = theBuffer->ReadByte();
	mFlag79 = theBuffer->ReadBoolean();
	mFlag7A = theBuffer->ReadBoolean();

	mTempleFlags.clear();
	int aCount = theBuffer->ReadLong();
	for (int i = 0; i < aCount && !theBuffer->AtEnd(); i++)
	{
		std::string aName = theBuffer->ReadString();
		mTempleFlags[aName] = theBuffer->ReadLong();
	}

	mRecords.clear();
	aCount = theBuffer->ReadLong();
	for (int i = 0; i < aCount && !theBuffer->AtEnd(); i++)
	{
		std::string aUuid = theBuffer->ReadString();
		PuzzleRecord aRecord;
		aRecord.mSolved = theBuffer->ReadLong();
		aRecord.mMoves2 = theBuffer->ReadLong();
		aRecord.mMoves = theBuffer->ReadLong();
		mRecords[UuidKey(aUuid)] = aRecord;
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
