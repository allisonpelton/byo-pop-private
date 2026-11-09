// UserProfileMgr.cpp: the original's UserProfileMgr (the export calls it UserManager): the list of players, the current
// player's name and the two high score tables, and how they are kept in the registry (Users\<name>\..., Users\User) and in
// users\user.idx. The profile itself, its data file users\user<N>.dat and its small methods are in UserProfile.cpp.
//
// Not translated:
//   0x4ad5f0   the demo recording trace, called at the top of most of these methods (the texts are the method names; the
//              ones of 0x427440 and 0x41a1b0 also log the size of the high score buffer)
//   0x41f1d0   the test of the byte at app+0x470 (registered): removed (registration)
//   the standard-library instances of the original, replaced by std::list, std::vector and std::string: 0x4ad0d0,
//   0x4adde0, 0x464ce0, 0x4153e0, 0x420240 (the list of profiles), 0x414e70 (destroys a vector of strings), 0x421260
//   (copies one), 0x416620 / 0x413ad0 (SexyString upper-case and equality: LoadUser's compare is written out below)
// In other files: 0x4262b0 (GetDefaultUserNumber) and 0x413ff0 (FirstFreeUserNumber) are in WinWSAppGame.cpp, the
// profile's own methods in UserProfile.cpp; the XML element (0x427200, 0x429860) and the lesson manager (0x427300, 0x4270e0,
// 0x421c50) that the same address range holds are other jobs.

#include "UserProfileMgr.h"
#include "UserProfile.h"
#include "WinWSApp.h"
#include "LessonManager.h"
#include "GameText.h"
#include "Buffer.h"
#include "Common.h"
#include "MTRand.h"
#include <stdlib.h>

using namespace Sexy;

// 0x416620: the upper-case copy LoadUser compares (only the letters a to z change)
static std::string UpperAscii(const std::string& theString)
{
	std::string aResult = theString;
	for (size_t i = 0; i < aResult.length(); i++)
	{
		if (aResult[i] >= 'a' && aResult[i] <= 'z')
			aResult[i] = (char)(aResult[i] - 0x20);
	}
	return aResult;
}

static int ReadFileInt(Buffer& theBuffer)
{
	return (int)atol(theBuffer.ReadString().c_str());
}

static void WriteFileInt(Buffer& theBuffer, int theValue)
{
	theBuffer.WriteString(StrFormat("%d", theValue));
}

// the high score file holds each entry as the score, the mode, the name and the fourth value
static void ReadHighScoreEntry(Buffer& theBuffer, HighScoreEntry& theEntry)
{
	theEntry.mScore = ReadFileInt(theBuffer);
	theEntry.mMode = ReadFileInt(theBuffer);
	theEntry.mName = theBuffer.ReadString();
	theEntry.mField24 = ReadFileInt(theBuffer);
}

static void WriteHighScoreEntry(Buffer& theBuffer, const HighScoreEntry& theEntry)
{
	WriteFileInt(theBuffer, theEntry.mScore);
	WriteFileInt(theBuffer, theEntry.mMode);
	theBuffer.WriteString(theEntry.mName);
	WriteFileInt(theBuffer, theEntry.mField24);
}

// 0x427440: the players of the registry key Users (one profile for each subkey), the current player's name, and the high
// score tables from users\user.idx (the defaults when the file is missing or older than 0.48)
UserProfileMgr::UserProfileMgr(WinWSApp* theApp)
{
	mApp = theApp;
	mUsers.clear();
	mCurrentUserName = "";

	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::UserProfileMgr"
	if (ExistsUserKey())
	{
		StringVector aSubKeys;
		if (mApp->RegistryGetSubKeys("Users", &aSubKeys))
		{
			unsigned int anIndex = 0;
			while (true)
			{
				if (anIndex >= aSubKeys.size())
					break;
				mUsers.push_back(LoadUserProfileFromRegistry(aSubKeys[anIndex]));
				anIndex++;
			}
		}
	}

	mCurrentUserName = LoadDefaultUserNameFromRegistry();

	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile("users\\user.idx", &aBuffer, false))
	{
		LoadDefaultHighScores();
		return;
	}

	// 0x4ad5f0 not ported: the trace "HS buffer size = %d"
	std::string aVersionText = aBuffer.ReadString();
	double aFileVersion = atof(aVersionText.c_str());
	atof(mApp->mProductVersion.c_str());        // the original converts the game's version too and does not use it
	if (aFileVersion < 0.48)
	{
		LoadDefaultHighScores();
		return;
	}

	aBuffer.ReadString();       // the number SaveHighScores wrote (the original reads it with atol and drops it)

	int i;
	for (i = 0; i < 10; i++)
		ReadHighScoreEntry(aBuffer, mScores[i]);
	for (i = 0; i < 10; i++)
		ReadHighScoreEntry(aBuffer, mAbyssScores[i]);
}

// 0x41f090: the manager frees the profiles
UserProfileMgr::~UserProfileMgr()
{
	std::list<UserProfile*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		delete *anItr;
		++anItr;
	}
	mUsers.clear();
}

// 0x4150a0
std::string UserProfileMgr::GetCurrentUserName()
{
	return mCurrentUserName;
}

// 0x41ac10: a copy of the entry (the abyss table or the score table)
HighScoreEntry UserProfileMgr::GetHighScore(bool theAbyss, int theIndex)
{
	if (theAbyss)
		return mAbyssScores[theIndex];
	return mScores[theIndex];
}

// 0x41a1b0: users\user.idx: the game's version, a number (drawn here from the game's generator, 0x60 to 0x84 with the
// signed remainder of the original), and the two tables, ten entries each
void UserProfileMgr::SaveHighScores()
{
	Buffer aBuffer;

	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::SaveHighScores"
	int aNumber = (int)gGameRand->NextNoAssert() % 0x25 + 0x60;
	aBuffer.WriteString(StrFormat("%s", mApp->mProductVersion.c_str()));
	WriteFileInt(aBuffer, aNumber);

	int i;
	for (i = 0; i < 10; i++)
		WriteHighScoreEntry(aBuffer, mScores[i]);
	for (i = 0; i < 10; i++)
		WriteHighScoreEntry(aBuffer, mAbyssScores[i]);

	mApp->WriteBufferToFile("users\\user.idx", &aBuffer);
}

// 0x41ef10: the tables of a new installation. The scores go from 500000 down in steps of 30000 while above 200000, and
// the mode follows the score (above 410000: 4, 320000: 3, 260000: 2, 200000: 1); the fourth value counts down from 10 and
// stops at 6. The abyss table goes from 2000 down in steps of 150 while above 500, with mode -1 and fourth value -1. The
// names come from two arrays of std::string objects at 0x55f988 and 0x55faa0 (ten each, 0x1c bytes apart); the static
// initializer 0x527890-0x527cc1 fills them from texts in the exe, which GT names by address (the table of the texts is
// extended to that initializer). Ends by saving the file.
static const unsigned int gDefaultScoreNames[10] =
{
	0x0052d8cc, 0x0052d8c4, 0x0052d8b8, 0x0052d8b0, 0x0052d8a8, 0x0052d8a0, 0x0052d898, 0x0052d890, 0x0052d884, 0x0052d87c
};
static const unsigned int gDefaultAbyssNames[10] =
{
	0x0052d924, 0x0052d91c, 0x0052d914, 0x0052d90c, 0x0052d904, 0x0052d8fc, 0x0052d8f4, 0x0052d8ec, 0x0052d8e4, 0x0052d8d8
};

void UserProfileMgr::LoadDefaultHighScores()
{
	int aScore = 500000;
	int aCountDown = 10;
	int i = 0;
	do
	{
		int aMode;
		int aField24 = 6;
		if (aCountDown >= 6)
			aField24 = aCountDown;
		if (aScore > 410000)
			aMode = 4;
		else if (aScore > 320000)
			aMode = 3;
		else if (aScore > 260000)
			aMode = 2;
		else
			aMode = aScore > 200000 ? 1 : 0;
		mScores[i].mName = GT(gDefaultScoreNames[i]);     // 0x55f988 + 0x1c * i
		mScores[i].mScore = aScore;
		mScores[i].mMode = aMode;
		mScores[i].mField24 = aField24;
		aScore -= 30000;
		aCountDown--;
		i++;
	} while (aScore > 200000);

	aScore = 2000;
	i = 0;
	do
	{
		mAbyssScores[i].mName = GT(gDefaultAbyssNames[i]);  // 0x55faa0 + 0x1c * i
		mAbyssScores[i].mMode = -1;
		mAbyssScores[i].mScore = aScore;
		mAbyssScores[i].mField24 = -1;
		aScore -= 150;
		i++;
	} while (aScore > 500);

	SaveHighScores();
}

// 0x420db0: true when the registry has a key Users. A registry that can not be read gets a popup and false.
bool UserProfileMgr::ExistsUserKey()
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::ExistsUserKey"
	StringVector aSubKeys;
	if (!mApp->RegistryGetSubKeys("", &aSubKeys))
		mApp->Popup(GT(0x0052c4c0));

	for (unsigned int i = 0; i < aSubKeys.size(); i++)
	{
		if (aSubKeys[i] == "Users")
			return true;
	}
	return false;
}

// 0x419fc0: the name in the registry value Users\User. When there is none, every profile is offered to
// RegisterDefaultUserName (which makes the last one the current name and the registry value) and the name stays empty:
// the original tests that function's result and it is always false.
std::string UserProfileMgr::LoadDefaultUserNameFromRegistry()
{
	std::string aName = "";

	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::LoadDefaultUserNameFromRegistry"
	std::string aValue;
	if (mApp->RegistryReadString("Users\\User", &aValue))
		aName = aValue;

	if (aName.length() == 0)
	{
		std::list<UserProfile*>::iterator anItr = mUsers.begin();
		while (anItr != mUsers.end())
		{
			if (RegisterDefaultUserName((*anItr)->GetName()))
			{
				aName = (*anItr)->GetName();
				break;
			}
			++anItr;
		}
	}
	return aName;
}

// 0x419da0: finds the first profile with this name (a full, case-sensitive match) and writes the name to the registry
// value Users\User; when that works it is also the current name. Returns false whatever happens (the original's last
// instruction clears al). port: the original copies the name through a 256-byte buffer first; a std::string needs none.
bool UserProfileMgr::RegisterDefaultUserName(const std::string& theName)
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::RegisterDefaultUserName: %s"
	std::list<UserProfile*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		if ((*anItr)->GetName() == theName)
		{
			if (mApp->RegistryWriteString("Users\\User", theName))
				mCurrentUserName = theName;
			break;
		}
		++anItr;
	}
	return false;
}

// 0x420ff0: removes the profiles with this name (a full, case-sensitive match): the registry key Users\<name>, the files
// users\user<N>.dat and users\user<N>.idx, and the profile. A key that can not be erased leaves the profile.
void UserProfileMgr::DeregisterUserProfile(const std::string& theName)
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::DeregisterUserProfile: %s"
	if (ExistsUserKey())
	{
		std::list<UserProfile*>::iterator anItr = mUsers.begin();
		while (anItr != mUsers.end())
		{
			UserProfile* aProfile = *anItr;
			if (aProfile->GetName() == theName)
			{
				int aUserNumber = aProfile->mUserNumber;
				if (mApp->RegistryEraseKey(StrFormat("Users\\%s", theName.c_str())))
				{
					mApp->EraseFile(StrFormat("users\\user%d.dat", aUserNumber));
					mApp->EraseFile(StrFormat("users\\user%d.idx", aUserNumber));
					delete aProfile;
					anItr = mUsers.erase(anItr);
					continue;
				}
			}
			++anItr;
		}
	}
}

// 0x420c60: the profile takes the place of the one with the same name, or goes to the end of the list. The profile it
// replaces is freed. leak fix (the original leaks this, 0x2c8 bytes and the profile's pairs for each replacement): nothing
// else points to the old profile when this is called (both callers, the select dialog's OK for a profile whose file did not
// load and ShowMapScreen with no current user, have just lost their mUser to a failed LoadUser or never had one).
void UserProfileMgr::SaveUser(UserProfile* theProfile)
{
	std::list<UserProfile*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		if ((*anItr)->GetName() == theProfile->GetName())
		{
			if (*anItr != theProfile)
				delete *anItr;
			*anItr = theProfile;
			return;
		}
		++anItr;
	}
	mUsers.push_back(theProfile);
}

// 0x425f20: the profile with this name (the letters a to z compared as capitals) with its data file loaded, or NULL. A
// profile whose file does not load keeps its registry values (written back first when its game version is not this
// game's; NULL when it is). The profile gets a new seed from the game's generator.
UserProfile* UserProfileMgr::LoadUser(const std::string& theName)
{
	UserProfile* aProfile = NULL;
	std::list<UserProfile*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		aProfile = *anItr;
		if (UpperAscii(aProfile->GetName()) == UpperAscii(theName))
			break;
		aProfile = NULL;
		++anItr;
	}

	if (aProfile == NULL)
		return NULL;

	if (!aProfile->LoadDataFromDisk())
	{
		if (aProfile->mGameVersion == mApp->mProductVersion)
			return NULL;
		WriteUserProfileToRegistry(aProfile, false, false);
	}
	aProfile->mSeed = (unsigned int)gGameRand->NextNoAssert();
	return aProfile;
}

// 0x413ef0: the registry string (a value written as text) read as a number; the number is set only when the value exists
bool UserProfileMgr::ReadIntString(const std::string& theValueName, int* theValue)
{
	std::string aString;
	if (mApp->RegistryReadString(theValueName, &aString))
	{
		*theValue = (int)atol(aString.c_str());
		return true;
	}
	return false;
}

// 0x425390: a profile from the registry values Users\<name>\...; what is missing keeps the default (0, except AbyssBoss -1
// and RNGC true). The profile constructor draws a seed when the value is 0. SetSpacePauses writes the value back.
UserProfile* UserProfileMgr::LoadUserProfileFromRegistry(const std::string& theName)
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::LoadUserProfileFromRegistry: %s"
	std::string aPrefix = StrFormat("Users\\%s\\", theName.c_str());
	int aUserNumber = 0;
	int aSeed = 0;
	int aBestDepth = 0;
	int aSharksKilled = 0;
	int aPiranhasKilled = 0;
	int aBossesKilled = 0;
	int aLastLesson = 0;
	int aAbyssSharksKilled = 0;
	int aAbyssPiranhasKilled = 0;
	int aNBTL = 0;
	bool aSpacePauses = false;
	int aPKTR = 0;
	int aSKTR = 0;
	int aNGC = 0;
	bool aRNGC = true;
	bool aSeenTutorial = false;
	int aAbyssBoss = -1;
	int aValue;
	bool aBool;
	int i;

	ReadIntString(aPrefix + "UserNumber", &aUserNumber);
	ReadIntString(aPrefix + "Seed", &aSeed);
	ReadIntString(aPrefix + "AnnoDomini", &aBestDepth);
	ReadIntString(aPrefix + "NumSharksKilled", &aSharksKilled);
	ReadIntString(aPrefix + "NumPiranhasKilled", &aPiranhasKilled);
	ReadIntString(aPrefix + "NumBossesKilled", &aBossesKilled);
	ReadIntString(aPrefix + "LastLesson", &aLastLesson);
	ReadIntString(aPrefix + "NumAbyssSharksKilled", &aAbyssSharksKilled);
	ReadIntString(aPrefix + "NumAbyssPiranhasKilled", &aAbyssPiranhasKilled);
	ReadIntString(aPrefix + "NBTL", &aNBTL);
	if (mApp->RegistryReadBoolean(aPrefix + "SpacePauses", &aBool))
		aSpacePauses = aBool;
	ReadIntString(aPrefix + "PKTR", &aPKTR);
	ReadIntString(aPrefix + "SKTR", &aSKTR);
	ReadIntString(aPrefix + "NGC", &aNGC);
	if (mApp->RegistryReadBoolean(aPrefix + "RNGC", &aBool))
		aRNGC = aBool;
	if (ReadIntString(aPrefix + "SeenTutorial", &aValue) && aValue != 0)
		aSeenTutorial = true;
	if (ReadIntString(aPrefix + "AbyssBoss", &aValue))
		aAbyssBoss = aValue;

	// 0x4ad5f0 not ported: the trace "UserProfileMgr::WriteUserProfileToRegistry:new profile made " (the original's text)
	UserProfile* aProfile = new UserProfile(mApp, theName, aUserNumber, aBestDepth, (unsigned int)aSeed, aSharksKilled,
		aPiranhasKilled, aAbyssSharksKilled, aAbyssPiranhasKilled);
	aProfile->SetField1e4(aAbyssBoss);
	aProfile->mLastLesson = aLastLesson;
	aProfile->SetField1e8(aNBTL);
	aProfile->SetSpacePauses(aSpacePauses);
	aProfile->SetField1f4(aSKTR);
	aProfile->SetField1f8(aPKTR);
	aProfile->mGamesCompleted = aNGC;
	aProfile->mFlag230 = aRNGC;

	if (mApp->RegistryReadBoolean(StrFormat("%ssvm", aPrefix.c_str()), &aBool))
		aProfile->mFlag284 = aBool;

	for (i = 0; i < 6; i++)
	{
		if (ReadIntString(StrFormat("%sGTT%d", aPrefix.c_str(), i), &aValue))
			aProfile->mField234[i] = aValue;
	}
	for (i = 0; i < 4; i++)
	{
		if (ReadIntString(StrFormat("%sSGTT%d", aPrefix.c_str(), i), &aValue))
			aProfile->mField24c[i] = aValue;
	}
	for (i = 0; i < 0x24; i++)
	{
		aBool = false;
		if (mApp->RegistryReadBoolean(StrFormat("%svl%d", aPrefix.c_str(), i), &aBool))
			aProfile->mField25c[i] = aBool;
		else
			aProfile->mField25c[i] = 0;
	}

	if (aBossesKilled > 0)
		aProfile->mBossesKilled = aProfile->mBossesKilled + aBossesKilled;

	if (ReadIntString(StrFormat("%svd", aPrefix.c_str()), &aValue))
		aProfile->mField280 = aValue;

	if (aSeenTutorial)
		aProfile->mFlag1dc = true;

	for (i = 0; i < 10; i++)
	{
		if (ReadIntString(StrFormat("%sWPM%d", aPrefix.c_str(), i), &aValue))
			aProfile->mWPM[i] = aValue;
	}
	if (ReadIntString(aPrefix + "LastWPMIndex", &aValue))
		aProfile->mField228 = aValue;

	return aProfile;
}

// 0x41f1d0: the profile's values to the registry (every number as the text of "%d", the flags as registry integers), then
// the data file and the score tables.
//   theRegistryOnly: nothing but the registry (the file and the tables are left alone).
//   theRecordScore: the last game's result goes into the score table (the abyss table when the last mode is -1) at the
//   first entry it is not below, replacing that entry (nothing below it moves), and the tables are saved; the data file is
//   written first with mField1ec taken from mField1b4 when theRecordScore is false.
void UserProfileMgr::WriteUserProfileToRegistry(UserProfile* theProfile, bool theRecordScore, bool theRegistryOnly)
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::WriteUserProfileToRegistry: %d, %d"
	std::string aPrefix = StrFormat("Users\\%s\\", theProfile->GetName().c_str());
	int i;

	theProfile->EnsureUsersUserValue();

	mApp->RegistryWriteString(aPrefix + "Version", mApp->mProductVersion);
	mApp->RegistryWriteString(aPrefix + "UserNumber", StrFormat("%d", theProfile->mUserNumber));
	mApp->RegistryWriteString(aPrefix + "Seed", StrFormat("%d", (int)theProfile->mSeed));
	mApp->RegistryWriteString(aPrefix + "AnnoDomini", StrFormat("%d", theProfile->mBestDepth));
	mApp->RegistryWriteString(aPrefix + "NumSharksKilled", StrFormat("%d", theProfile->mSharksKilled));
	mApp->RegistryWriteString(aPrefix + "NumPiranhasKilled", StrFormat("%d", theProfile->mPiranhasKilled));
	mApp->RegistryWriteString(aPrefix + "NumBossesKilled", StrFormat("%d", theProfile->mBossesKilled));
	mApp->RegistryWriteString(aPrefix + "LastLesson", StrFormat("%d", theProfile->mLastLesson));
	mApp->RegistryWriteString(aPrefix + "NumAbyssSharksKilled", StrFormat("%d", theProfile->mAbyssSharksKilled));
	mApp->RegistryWriteString(aPrefix + "NumAbyssPiranhasKilled", StrFormat("%d", theProfile->mAbyssPiranhasKilled));

	int aNBTL = 0;
	if (mApp->mLessonManager != NULL)
		aNBTL = mApp->mLessonManager->mEndlessCount - 1;        // 0x412370
	mApp->RegistryWriteString(aPrefix + "NBTL", StrFormat("%d", aNBTL));

	mApp->RegistryWriteBoolean(aPrefix + "SpacePauses", theProfile->mSpacePauses);
	mApp->RegistryWriteString(aPrefix + "PKTR", StrFormat("%d", theProfile->mField1f8));
	mApp->RegistryWriteString(aPrefix + "SKTR", StrFormat("%d", theProfile->mField1f4));

	for (i = 0; i < 10; i++)
		mApp->RegistryWriteString(StrFormat("%sWPM%d", aPrefix.c_str(), i), StrFormat("%d", theProfile->mWPM[i]));
	mApp->RegistryWriteString(aPrefix + "LastWPMIndex", StrFormat("%d", theProfile->mField228));

	mApp->RegistryWriteString(aPrefix + "SeenTutorial", StrFormat("%d", theProfile->mFlag1dc ? 1 : 0));
	mApp->RegistryWriteString(aPrefix + "AbyssBoss", StrFormat("%d", theProfile->GetField1e4()));
	mApp->RegistryWriteString(aPrefix + "NGC", StrFormat("%d", theProfile->mGamesCompleted));
	mApp->RegistryWriteBoolean(aPrefix + "RNGC", theProfile->mFlag230);

	for (i = 0; i < 6; i++)
		mApp->RegistryWriteString(StrFormat("%sGTT%d", aPrefix.c_str(), i), StrFormat("%d", theProfile->mField234[i]));
	for (i = 0; i < 4; i++)
		mApp->RegistryWriteString(StrFormat("%sSGTT%d", aPrefix.c_str(), i), StrFormat("%d", theProfile->mField24c[i]));
	for (i = 0; i < 0x24; i++)
		mApp->RegistryWriteBoolean(StrFormat("%svl%d", aPrefix.c_str(), i), theProfile->mField25c[i] != 0);
	mApp->RegistryWriteString(StrFormat("%svd", aPrefix.c_str()), StrFormat("%d", theProfile->mField280));
	mApp->RegistryWriteBoolean(StrFormat("%ssvm", aPrefix.c_str()), theProfile->mFlag284);

	if (!theRegistryOnly)
	{
		theProfile->WriteDataToDisk(!theRecordScore);

		if (theRecordScore)
		{
			if (theProfile->mLastMode == -1)
			{
				int aDepth = theProfile->mLastDepth;
				for (i = 0; i < 10; i++)
				{
					if (aDepth >= mAbyssScores[i].mScore)
					{
						mAbyssScores[i].mScore = aDepth;
						mAbyssScores[i].mMode = -1;
						mAbyssScores[i].mName = theProfile->GetName();
						mAbyssScores[i].mField24 = -1;
						SaveHighScores();
						break;
					}
				}
			}
			else
			{
				int aScore = theProfile->mLastScore;
				for (i = 0; i < 10; i++)
				{
					if (aScore >= mScores[i].mScore)
					{
						mScores[i].mScore = aScore;
						mScores[i].mMode = theProfile->mLastMode;
						mScores[i].mName = theProfile->GetName();
						mScores[i].mField24 = theProfile->mField0c;
						SaveHighScores();
						break;
					}
				}
			}
		}
	}
}
