#include "UserProfile.h"
#include "WinWSApp.h"
#include "Buffer.h"
#include "Common.h"
#include "MTRand.h"
#include <stdlib.h>

using namespace Sexy;

// the number of records in a stat group and in the vector at +0xe0 (the constructors push them, the file holds them)
static const int NUM_GROUP_RECORDS = 0x23;
static const int NUM_RECORDS_E0 = 0x13;

// 0x4172b0 / 0x413ef0 shapes: a number in the file is the text of "%d", read back with atol
static void WriteFileInt(Buffer& theBuffer, int theValue)
{
	theBuffer.WriteString(StrFormat("%d", theValue));
}

static int ReadFileInt(Buffer& theBuffer)
{
	return (int)atol(theBuffer.ReadString().c_str());
}

// the defaults both constructors store (0x421f50 and 0x4222d0 store the same ones, the second then takes its arguments
// and the seed). port: one function for the two constructors; the stores are in the order of 0x421f50
void UserProfile::InitDefaults()
{
	int i;

	mFlag1fc = true;
	mField1e4 = -1;
	mFlag1dc = false;
	mField1ec = 0;
	mField1e8 = 0;
	mSpacePauses = false;
	mField1f4 = 0;
	mField1f8 = 0;
	mName = "";
	mUserNumber = 0;
	mBestDepth = 0;
	mBossesKilled = 0;
	mSharksKilled = 0;
	mPiranhasKilled = 0;
	mKillCount[0] = 0;
	mKillCount[1] = 0;
	mKillCount[2] = 0;
	mKillCount[3] = 0;
	mAbyssSharksKilled = 0;
	mAbyssPiranhasKilled = 0;
	mField280 = 0;
	mGameVersion = mApp->mProductVersion;       // 0x3b8 of the app
	mLastLesson = 0;
	for (i = 0; i < 0x24; i++)
		mField25c[i] = 0;
	mLastScore = 0;
	mLastMode = 0;
	mLastDepth = 0;
	mField0c = 0;
	mField22c = 0;
	mFlag284 = false;
	mField1c0 = -1;
	mDouble1b8 = -1.0;                          // 0xbff00000 in the high half
	mField1b4 = -1;
	mField1c4 = -1;
	mField1c8 = 0;                              // port: neither constructor stores +0x1c8 (new memory); zero here
	mShowHints = true;
	mField1b0 = -1;
	mField1d0 = -1;
	mField1cc = -1;
	mField1e0 = 0;
	mField1d4 = -1;
	mField1d8 = -1;
	mFlag230 = true;
	mVector2b4.clear();
	mVector2a4.clear();
	for (i = 0; i < 10; i++)
		mWPM[i] = 0;
	mField228 = 0;

	for (int g = 0; g < 6; g++)
	{
		mGroups[g].mField0 = 0;
		mGroups[g].mBestScore = 0;
		mGroups[g].mField8 = 0;
		mGroups[g].mLastScore = 0;
		mGroups[g].mRecords.clear();
		for (i = 0; i < NUM_GROUP_RECORDS; i++)
			mGroups[g].mRecords.push_back(StatRecord());
	}

	mRecordsE0.clear();
	for (i = 0; i < NUM_RECORDS_E0; i++)
	{
		mField94[i] = -1;
		mRecordsE0.push_back(StatRecord());
	}
	mField64 = 0;
	mField68 = 0;
	for (i = 0; i < 6; i++)
	{
		mField6c[i] = 0;
		mField234[i] = 0;
	}
	for (i = 0; i < 4; i++)
	{
		mField84[i] = 0;
		mField24c[i] = 0;
	}
}

// 0x421f50: a new player (the profile manager makes one when the user types a name): everything is at its default and the
// seed is a draw from the game's generator
UserProfile::UserProfile(WinWSApp* theApp)
{
	mApp = theApp;
	InitDefaults();
	mSeed = (unsigned int)gGameRand->NextNoAssert();
}

// 0x4222d0: a player from the values in the registry (UserProfileMgr::LoadUserProfileFromRegistry): the name, the number,
// the deepest depth, the seed (a seed of 0 is drawn from the game's generator) and the four kill counts
UserProfile::UserProfile(WinWSApp* theApp, const std::string& theName, int theUserNumber, int theBestDepth,
	unsigned int theSeed, int theSharksKilled, int thePiranhasKilled, int theAbyssSharksKilled, int theAbyssPiranhasKilled)
{
	mApp = theApp;
	InitDefaults();
	mName = theName;
	mUserNumber = theUserNumber;
	mBestDepth = theBestDepth;
	mSeed = theSeed;
	if (theSeed == 0)
		mSeed = (unsigned int)gGameRand->NextNoAssert();
	mSharksKilled = theSharksKilled;
	mPiranhasKilled = thePiranhasKilled;
	mAbyssPiranhasKilled = theAbyssPiranhasKilled;
	mAbyssSharksKilled = theAbyssSharksKilled;
}

// 0x417170 (the destructor body): the map pairs were malloc'd, the profile frees them (and the vectors)
UserProfile::~UserProfile()
{
	unsigned int i = 0;
	while (true)
	{
		if (i >= mVector2a4.size())
			break;
		delete mVector2a4[i];
		i++;
	}
}

// 0x415060
std::string UserProfile::GetName()
{
	return mName;
}

// 0x416380
void UserProfile::SetName(const std::string& theName)
{
	mName = theName;
}

// 0x412380
void UserProfile::SetUserNumber(int theNumber)
{
	mUserNumber = theNumber;
}

// 0x412340
int UserProfile::GetField9c()
{
	return mField94[2];
}

// 0x412350
int UserProfile::GetField8c()
{
	return mField84[2];
}

// 0x412390
void UserProfile::SetField1e8(int theValue)
{
	mField1e8 = theValue;
}

// 0x4123a0: one more kill of the creature type (only the types 0xc, 0xe, 0xf and 0x10 are counted)
void UserProfile::CountKill(int theType)
{
	switch (theType)
	{
	case 0xc:
		mKillCount[0] = mKillCount[0] + 1;
		return;
	case 0xe:
		mKillCount[1] = mKillCount[1] + 1;
		return;
	case 0xf:
		mKillCount[2] = mKillCount[2] + 1;
		return;
	case 0x10:
		mKillCount[3] = mKillCount[3] + 1;
	}
}

// 0x4123f0: the speed goes into the ring of the last ten
void UserProfile::AddWPM(int theSpeed)
{
	mWPM[mField228] = theSpeed;
	int anIndex = mField228 + 1;
	mField228 = anIndex;
	if (9 < anIndex)
		mField228 = 0;
}

// 0x4124d0: the kill count of the creature type; any other type gives the type minus 0xc (the switch's default leaves
// that value in eax)
int UserProfile::GetKillCount(int theType)
{
	int aDelta = theType - 0xc;
	switch (aDelta)
	{
	case 0:
		return mKillCount[0];
	case 2:
		return mKillCount[1];
	case 3:
		return mKillCount[2];
	case 4:
		aDelta = mKillCount[3];
	}
	return aDelta;
}

// 0x412520: the saved game with the level configuration's double (nothing is stored when theField1c4 is -1)
void UserProfile::RecordResultWithDouble(bool theFlag, int theField1b0, int theField1b4, double theDouble1b8,
	int theField1c0, int theField1c4, int theField1c8)
{
	if (theField1c4 != -1)
	{
		mField1b0 = theField1b0;
		if (!theFlag)
			theField1b4 = mField1ec;
		mField1b4 = theField1b4;
		mDouble1b8 = theDouble1b8;
		mField1c4 = theField1c4;
		mField1c0 = theField1c0;
		mField1c8 = theField1c8;
	}
}

// 0x412580: the same without the double (nothing is stored when theField1c4 is -1)
void UserProfile::RecordResult(bool theFlag, int theField1b0, int theField1b4, int theField1c0, int theField1c4,
	int theField1c8)
{
	if (theField1c4 != -1)
	{
		mField1b0 = theField1b0;
		if (!theFlag)
			theField1b4 = mField1ec;
		mField1b4 = theField1b4;
		mField1c4 = theField1c4;
		mField1c0 = theField1c0;
		mField1c8 = theField1c8;
	}
}

// 0x4125d0: the best score of the modes 0 to 4 (at least 0)
int UserProfile::GetBestScore()
{
	int aScore = 0;
	if (0 < mGroups[0].mBestScore)
		aScore = mGroups[0].mBestScore;
	if (aScore < mGroups[1].mBestScore)
		aScore = mGroups[1].mBestScore;
	if (aScore < mGroups[2].mBestScore)
		aScore = mGroups[2].mBestScore;
	if (aScore < mGroups[3].mBestScore)
		aScore = mGroups[3].mBestScore;
	if (aScore < mGroups[4].mBestScore)
		aScore = mGroups[4].mBestScore;
	return aScore;
}

// 0x412610: a shark killed (in the abyss mode or not), counted for the profile and for the round
void UserProfile::CountSharkKilled(bool theAbyss)
{
	if (!theAbyss)
	{
		mSharksKilled = mSharksKilled + 1;
		mField1f4 = mField1f4 + 1;
		return;
	}
	mAbyssSharksKilled = mAbyssSharksKilled + 1;
	mField1f4 = mField1f4 + 1;
}

// 0x412640
void UserProfile::SetField1f4(int theValue)
{
	mField1f4 = theValue;
}

// 0x412650
void UserProfile::SetField1f8(int theValue)
{
	mField1f8 = theValue;
}

// 0x412660: a piranha killed (in the abyss mode or not), counted for the profile and for the round
void UserProfile::CountPiranhaKilled(bool theAbyss)
{
	if (!theAbyss)
	{
		mPiranhasKilled = mPiranhasKilled + 1;
		mField1f8 = mField1f8 + 1;
		return;
	}
	mAbyssPiranhasKilled = mAbyssPiranhasKilled + 1;
	mField1f8 = mField1f8 + 1;
}

// 0x412690: one more in mField6c[theIndex] and in mField234[theIndex] (the registry GTT<i>)
void UserProfile::CountField6c(int theIndex)
{
	mField6c[theIndex] = mField6c[theIndex] + 1;
	mField234[theIndex] = mField234[theIndex] + 1;
}

// 0x4126b0: one more in mField84[theIndex] and in mField24c[theIndex] (the registry SGTT<i>)
void UserProfile::CountField84(int theIndex)
{
	mField84[theIndex] = mField84[theIndex] + 1;
	mField24c[theIndex] = mField24c[theIndex] + 1;
}

// 0x4126d0
int UserProfile::GetField1e4()
{
	return mField1e4;
}

// 0x4126e0
void UserProfile::SetField1e4(int theValue)
{
	mField1e4 = theValue;
}

// 0x4126f0
void UserProfile::ClearFields1cc()
{
	mField1e0 = 0;
	mField1cc = 0;
	mField1d0 = 0;
	mField1d8 = 0;
	mField1d4 = 0;
}

// 0x412720: the value of the mode's group, and its best
void UserProfile::SetGroupField8(int theGroup, int theValue)
{
	mGroups[theGroup].mField8 = theValue;
	if (mGroups[theGroup].mField0 < theValue)
		mGroups[theGroup].mField0 = theValue;
}

// 0x412750: the result of a game of the mode (5 is the abyss mode, kept as -1 in mLastMode): the profile's last score
// and the mode's last and best score
void UserProfile::SetLastGame(int theMode, int theScore, int theField0c)
{
	if (theMode == 5)
		mLastMode = -1;
	else
		mLastMode = theMode;
	mField0c = theField0c;
	mLastScore = theScore;
	mGroups[theMode].mLastScore = theScore;
	if (mGroups[theMode].mBestScore < theScore)
		mGroups[theMode].mBestScore = theScore;
}

// port: 0x416970, 0x416c00, 0x416ed0 and 0x41f1d0 start with the same lines: when the registry holds no value Users\User,
// it is set to this player's name
void UserProfile::EnsureUsersUserValue()
{
	std::string aValue;
	if (!mApp->RegistryReadString("Users\\User", &aValue))
		mApp->RegistryWriteString("Users\\User", GetName());
}

// 0x416970: the registry value Users\<name>\ad<theIndex> is set to the text of "%f" of the value
void UserProfile::SetAdValue(int theIndex, double theValue)
{
	std::string aPrefix = StrFormat("Users\\%s\\", GetName().c_str());
	EnsureUsersUserValue();
	mApp->RegistryWriteString(aPrefix + StrFormat("ad%d", theIndex), StrFormat("%f", theValue));
}

// 0x416c00: the registry value Users\<name>\ad<theIndex> as a double (0 when there is none)
double UserProfile::GetAdValue(int theIndex)
{
	std::string aPrefix = StrFormat("Users\\%s\\", GetName().c_str());
	EnsureUsersUserValue();
	double aValue = 0.0;
	std::string aString;
	if (mApp->RegistryReadString(aPrefix + StrFormat("ad%d", theIndex), &aString))
		aValue = atof(aString.c_str());
	return aValue;
}

// 0x416ed0: the space bar pauses (or not): kept, and written to the registry
void UserProfile::SetSpacePauses(bool theValue)
{
	// 0x4ad5f0 not ported: the demo recording trace "UserProfile::SetSpaceToPause = %d"
	std::string aPrefix = StrFormat("Users\\%s\\", GetName().c_str());
	mSpacePauses = theValue;
	EnsureUsersUserValue();
	mApp->RegistryWriteBoolean(aPrefix + "SpacePauses", mSpacePauses);
}

// 0x4172f0: the profile's data file users\user<N>.dat. Every number is the text of "%d". Order: the version; for each of
// the six stat groups four ints and 35 records of twelve ints; mField94 (19); the 19 records at +0xe0; mField64; mField6c
// (6); mField84 (4); mField68; mField1b0; mField1ec; the double * 100 as an int; mField1c0 to mField1d8; the number of
// map pairs and the pairs; the number of node ids and the ids; mField1e0; the four kill counts.
// theCopyField1b4 makes mField1ec the value of mField1b4 first.
void UserProfile::WriteDataToDisk(bool theCopyField1b4)
{
	Buffer aBuffer;
	int g;
	int i;

	// 0x4ad5f0 not ported: the demo recording trace "UserProfileMgr::WriteDataToDisk: %d"
	aBuffer.WriteString(StrFormat("%s", mApp->mProductVersion.c_str()));

	// each group's records: the first 35 (a load appends after them, see LoadDataFromDisk)
	g = 0;
	do
	{
		WriteFileInt(aBuffer, mGroups[g].mField0);
		WriteFileInt(aBuffer, mGroups[g].mBestScore);
		WriteFileInt(aBuffer, mGroups[g].mField8);
		WriteFileInt(aBuffer, mGroups[g].mLastScore);
		i = 0;
		do
		{
			StatRecord aRecord = mGroups[g].mRecords[i];
			for (int k = 0; k < 12; k++)
				WriteFileInt(aBuffer, aRecord.mField[k]);
			i++;
		} while (i < NUM_GROUP_RECORDS);
		g++;
	} while (g < 6);

	for (i = 0; i < 0x13; i++)
		WriteFileInt(aBuffer, mField94[i]);

	i = 0;
	do
	{
		StatRecord aRecord = mRecordsE0[i];
		for (int k = 0; k < 12; k++)
			WriteFileInt(aBuffer, aRecord.mField[k]);
		i++;
	} while (i < NUM_RECORDS_E0);

	WriteFileInt(aBuffer, mField64);
	for (i = 0; i < 6; i++)
		WriteFileInt(aBuffer, mField6c[i]);
	for (i = 0; i < 4; i++)
		WriteFileInt(aBuffer, mField84[i]);
	WriteFileInt(aBuffer, mField68);

	if (theCopyField1b4)
		mField1ec = mField1b4;
	WriteFileInt(aBuffer, mField1b0);
	WriteFileInt(aBuffer, mField1ec);
	WriteFileInt(aBuffer, (int)(mDouble1b8 * 100.0));       // _ftol
	WriteFileInt(aBuffer, mField1c0);
	WriteFileInt(aBuffer, mField1c4);
	WriteFileInt(aBuffer, mField1c8);
	WriteFileInt(aBuffer, mField1cc);
	WriteFileInt(aBuffer, mField1d0);
	WriteFileInt(aBuffer, mField1d4);
	WriteFileInt(aBuffer, mField1d8);

	WriteFileInt(aBuffer, (int)mVector2a4.size());
	unsigned int aPairIndex = 0;
	while (true)
	{
		if (aPairIndex >= mVector2a4.size())
			break;
		WriteFileInt(aBuffer, mVector2a4[aPairIndex]->mFirst);
		WriteFileInt(aBuffer, mVector2a4[aPairIndex]->mSecond);
		aPairIndex++;
	}

	WriteFileInt(aBuffer, (int)mVector2b4.size());
	unsigned int aNodeIndex = 0;
	while (true)
	{
		if (aNodeIndex >= mVector2b4.size())
			break;
		WriteFileInt(aBuffer, mVector2b4[aNodeIndex]);
		aNodeIndex++;
	}

	WriteFileInt(aBuffer, mField1e0);
	for (i = 0; i < 4; i++)
		WriteFileInt(aBuffer, mKillCount[i]);

	mApp->WriteBufferToFile(StrFormat("users\\user%d.dat", mUserNumber), &aBuffer);
}

// 0x422630: reads users\user<N>.dat back (the order of WriteDataToDisk). False when the file is missing or its version is
// below 0.48; an old file may end early (every read after the 19 records is guarded by AtEnd, and a file that ends inside
// the 19 records gets the 19 zero records). The version read replaces mGameVersion.
// Kept from the original: the six groups' records are appended to the 35 the constructor made, so a profile that was
// loaded has 70 per group, the loaded ones at the end, and the writer only writes the first 35.
bool UserProfile::LoadDataFromDisk()
{
	Buffer aBuffer;
	int g;
	int i;

	// 0x4ad5f0 not ported: the demo recording traces "UserProfile::LoadDataFromDisk", "Failed  to load %s" and
	// "Buffer for %s is %d"
	if (!mApp->ReadBufferFromFile(StrFormat("users\\user%d.dat", mUserNumber), &aBuffer, false))
		return false;

	mField64 = 0;
	mField68 = 0;
	for (i = 0; i < 6; i++)
		mField6c[i] = 0;
	mField84[0] = 0;
	mField84[1] = 0;
	mField84[2] = 0;
	mField84[3] = 0;

	mGameVersion = aBuffer.ReadString();
	double aFileVersion = atof(mGameVersion.c_str());
	atof(mApp->mProductVersion.c_str());
	if (aFileVersion < 0.48)
		return false;

	g = 0;
	do
	{
		mGroups[g].mField0 = ReadFileInt(aBuffer);
		mGroups[g].mBestScore = ReadFileInt(aBuffer);
		mGroups[g].mField8 = ReadFileInt(aBuffer);
		mGroups[g].mLastScore = ReadFileInt(aBuffer);
		i = 0;
		do
		{
			StatRecord aRecord;
			for (int k = 0; k < 12; k++)
				aRecord.mField[k] = ReadFileInt(aBuffer);
			mGroups[g].mRecords.push_back(aRecord);
			i++;
		} while (i < NUM_GROUP_RECORDS);
		g++;
	} while (g < 6);

	for (i = 0; i < 0x13; i++)
		mField94[i] = ReadFileInt(aBuffer);

	mRecordsE0.clear();
	i = 0;
	do
	{
		mRecordsE0.push_back(StatRecord());
		for (int k = 0; k < 12; k++)
		{
			mRecordsE0[i].mField[k] = ReadFileInt(aBuffer);
			if (aBuffer.AtEnd())
			{
				mRecordsE0.clear();
				for (int j = 0; j < NUM_RECORDS_E0; j++)
					mRecordsE0.push_back(StatRecord());
				return true;
			}
		}
		i++;
	} while (i < NUM_RECORDS_E0);

	if (!aBuffer.AtEnd())
		mField64 = ReadFileInt(aBuffer);
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 6; i++)
			mField6c[i] = ReadFileInt(aBuffer);
	}
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 4; i++)
			mField84[i] = ReadFileInt(aBuffer);
	}
	if (!aBuffer.AtEnd())
		mField68 = ReadFileInt(aBuffer);

	if (!aBuffer.AtEnd())
		mField1b0 = ReadFileInt(aBuffer);
	else
		mField1b0 = -1;
	if (!aBuffer.AtEnd())
		mField1b4 = ReadFileInt(aBuffer);
	else
		mField1b4 = -1;
	if (!aBuffer.AtEnd())
		mDouble1b8 = (double)ReadFileInt(aBuffer) * 0.01;
	else
		mDouble1b8 = -1.0;
	if (!aBuffer.AtEnd())
		mField1c0 = ReadFileInt(aBuffer);
	else
		mField1c0 = -1;
	if (!aBuffer.AtEnd())
		mField1c4 = ReadFileInt(aBuffer);
	else
		mField1c4 = -1;
	if (!aBuffer.AtEnd())
		mField1c8 = ReadFileInt(aBuffer);
	else
		mField1c8 = -1;
	if (!aBuffer.AtEnd())
		mField1cc = ReadFileInt(aBuffer);
	else
		mField1cc = -1;
	if (!aBuffer.AtEnd())
		mField1d0 = ReadFileInt(aBuffer);
	else
		mField1d0 = -1;
	if (!aBuffer.AtEnd())
		mField1d4 = ReadFileInt(aBuffer);
	else
		mField1d4 = -1;
	if (!aBuffer.AtEnd())
		mField1d8 = ReadFileInt(aBuffer);
	else
		mField1d8 = -1;

	if (!aBuffer.AtEnd())
	{
		int aPairCount = ReadFileInt(aBuffer);
		if (0 < aPairCount)
		{
			int aPairsLeft = aPairCount;
			do
			{
				ProfilePair* aPair = new ProfilePair;
				aPair->mFirst = 0;
				aPair->mSecond = 0;
				aPair->mFirst = ReadFileInt(aBuffer);
				aPair->mSecond = ReadFileInt(aBuffer);
				mVector2a4.push_back(aPair);
				aPairsLeft--;
			} while (aPairsLeft != 0);
		}
		int aNodeCount = ReadFileInt(aBuffer);
		if (0 < aNodeCount)
		{
			int aNodesLeft = aNodeCount;
			do
			{
				mVector2b4.push_back(ReadFileInt(aBuffer));
				aNodesLeft--;
			} while (aNodesLeft != 0);
		}
	}

	if (!aBuffer.AtEnd())
		mField1e0 = ReadFileInt(aBuffer);
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 4; i++)
			mKillCount[i] = ReadFileInt(aBuffer);
	}
	return true;
}
