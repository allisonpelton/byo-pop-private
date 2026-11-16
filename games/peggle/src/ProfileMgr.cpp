// ProfileMgr.cpp: see ProfileMgr.h.

#include "ProfileMgr.h"

#include "DataSync.h"
#include "SexyAppBase.h"

using namespace Sexy;

static const int PROFILE_LIST_VERSION = 1; // 0x649ad0

ProfileMgr::ProfileMgr() : mNextUserId(0), mNextUseSeq(0)
{
}

// 0x473280
void ProfileMgr::SyncState(DataSync& theSync)
{
	int aVersion = PROFILE_LIST_VERSION;
	theSync.SyncLong(aVersion);
	theSync.mVersion = aVersion;
	if (aVersion > PROFILE_LIST_VERSION)
		return;

	if (!theSync.IsReading())
	{
		int aCount = (int)mProfiles.size();
		theSync.SyncShort(aCount);
		for (auto& aPair : mProfiles)
			aPair.second.SyncSummary(theSync);
		return;
	}

	mProfiles.clear();
	int aCount = 0;
	theSync.SyncShort(aCount);
	int aMaxUseSeq = 0;
	int aMaxUserId = 0;
	for (; aCount != 0; aCount--)
	{
		PlayerInfo aProfile;
		aProfile.SyncSummary(theSync);
		if (aMaxUseSeq < aProfile.mUseSeq)
			aMaxUseSeq = aProfile.mUseSeq;
		if (aMaxUserId < aProfile.mUserId)
			aMaxUserId = aProfile.mUserId;
		mProfiles[aProfile.mName] = aProfile;
	}
	mNextUserId = aMaxUserId + 1;
	mNextUseSeq = aMaxUseSeq + 1;
}

// 0x473490
void ProfileMgr::Load()
{
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile("userdata/users.dat", &aBuffer))
		return;
	DataSync aSync;
	aSync.StartReadMemory(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	try
	{
		SyncState(aSync);
	}
	catch (DataSyncException&)
	{
		// a file cut short keeps what was read up to there
	}
}

// 0x4735f0
void ProfileMgr::Save()
{
	DataSync aSync;
	aSync.StartWriteMemory();
	SyncState(aSync);
	gSexyAppBase->WriteBytesToFile("userdata/users.dat", aSync.GetWrittenData().data(), (unsigned long)aSync.GetWrittenData().size());
}

// 0x442e10
PlayerInfo* ProfileMgr::GetProfile(const std::string& theName)
{
	ProfileMap::iterator anItr = mProfiles.find(theName);
	if (anItr == mProfiles.end())
		return nullptr;
	PlayerInfo* aProfile = &anItr->second;
	aProfile->LoadDetails();
	aProfile->mUseSeq = mNextUseSeq++;
	return aProfile;
}

// 0x441ca0
PlayerInfo* ProfileMgr::GetAnyProfile()
{
	if (mProfiles.empty())
		return nullptr;
	PlayerInfo* aProfile = &mProfiles.begin()->second;
	aProfile->LoadDetails();
	aProfile->mUseSeq = mNextUseSeq++;
	return aProfile;
}

// 0x4717c0
PlayerInfo* ProfileMgr::AddProfile(const std::string& theName)
{
	std::pair<ProfileMap::iterator, bool> anInsert = mProfiles.insert(std::make_pair(theName, PlayerInfo()));
	if (!anInsert.second)
		return nullptr;

	PlayerInfo* aProfile = &anInsert.first->second;
	aProfile->mName = theName;
	aProfile->mUserId = mNextUserId++;
	aProfile->mUseSeq = mNextUseSeq++;
	return aProfile;
}

// 0x471640: a profile keeps its details (they are copied to the entry of the new name, which is where the list puts it) and
// its files. Only the letters' case changing leaves the entry where it is.
bool ProfileMgr::RenameProfile(const std::string& theOldName, const std::string& theNewName)
{
	ProfileMap::iterator anOld = mProfiles.find(theOldName);
	if (anOld == mProfiles.end())
		return false;

	PlayerInfo* aRenamed;
	if (sexy_stricmp(theOldName.c_str(), theNewName.c_str()) == 0)
		aRenamed = &anOld->second;
	else
	{
		std::pair<ProfileMap::iterator, bool> anInsert = mProfiles.insert(std::make_pair(theNewName, anOld->second));
		if (!anInsert.second)
			return false; // there is one of that name
		mProfiles.erase(anOld);
		aRenamed = &anInsert.first->second;
	}
	aRenamed->mName = theNewName;
	return true;
}

// 0x4526b0, with the erase of 0x452670 that deletes the profile's files first
bool ProfileMgr::DeleteProfile(const std::string& theName)
{
	ProfileMap::iterator aProfile = mProfiles.find(theName);
	if (aProfile == mProfiles.end())
		return false;

	aProfile->second.DeleteUserFiles();
	mProfiles.erase(aProfile);
	return true;
}
