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

// port: a number of the profile file, which holds each as text
static int ReadFileInt(Buffer& theBuffer)
{
	return (int)atol(theBuffer.ReadString().c_str());
}

// the defaults both constructors store (0x421f50 and 0x4222d0 store the same ones, the second then takes its arguments
// and the seed). port: one function for the two constructors; the stores are in the order of 0x421f50
void UserProfile::InitDefaults()
{
	int i;

	mHintBoxesWanted = true;
	mAbyssBossIndex = -1;
	mSeenTutorial = false;
	mSavedScoreBackup = 0;
	mEndlessLevelsPlayed = 0;
	mSpacePauses = false;
	mGameSharksKilled = 0;
	mGamePiranhasKilled = 0;
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
	mVisitedDifficulty = 0;
	mGameVersion = mApp->mProductVersion;       // 0x3b8 of the app
	mLastLesson = 0;
	for (i = 0; i < 0x24; i++)
		mVisitedLevels[i] = 0;
	mLastScore = 0;
	mLastMode = 0;
	mLastDepth = 0;
	mLastLevelReached = 0;
	mGamesCompleted = 0;
	mOpenChestHintSeen = false;
	mSavedLevel = -1;
	mSavedEmpCharge = -1.0;                          // 0xbff00000 in the high half
	mSavedScore = -1;
	mSavedDifficulty = -1;
	mSavedLevelNumber = 0;                              // port: neither constructor stores +0x1c8 (new memory); zero here
	mShowHints = true;
	mSavedLives = -1;
	mSavedPausedTime = -1;
	mSavedGameTime = -1;
	mSavedTypingTime = 0;
	mSavedLettersWrong = -1;
	mSavedLettersRight = -1;
	mCompletionPending = true;
	mSavedRouteNodeIds.clear();
	mSavedRouteDots.clear();
	for (i = 0; i < 10; i++)
		mWPM[i] = 0;
	mNextWPMIndex = 0;

	for (int g = 0; g < 6; g++)
	{
		mGroups[g].mBestLevel = 0;
		mGroups[g].mBestScore = 0;
		mGroups[g].mLastLevel = 0;
		mGroups[g].mLastScore = 0;
		mGroups[g].mRecords.clear();
		for (i = 0; i < NUM_GROUP_RECORDS; i++)
			mGroups[g].mRecords.push_back(StatRecord());
	}

	mLessonRecords.clear();
	for (i = 0; i < NUM_RECORDS_E0; i++)
	{
		mLessonOutcome[i] = -1;
		mLessonRecords.push_back(StatRecord());
	}
	mBestBonusWordPoints = 0;
	mBestTreasureWords = 0;
	for (i = 0; i < 6; i++)
	{
		mGemsFound[i] = 0;
		mGameGemsFound[i] = 0;
	}
	for (i = 0; i < 4; i++)
	{
		mSecretsFound[i] = 0;
		mGameSecretsFound[i] = 0;
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
		if (i >= mSavedRouteDots.size())
			break;
		delete mSavedRouteDots[i];
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
int UserProfile::GetLessonTwoOutcome()
{
	return mLessonOutcome[2];
}

// 0x412350
int UserProfile::GetNecklacesFound()
{
	return mSecretsFound[2];
}

// 0x412390
void UserProfile::SetEndlessLevelsPlayed(int theValue)
{
	mEndlessLevelsPlayed = theValue;
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
	mWPM[mNextWPMIndex] = theSpeed;
	int anIndex = mNextWPMIndex + 1;
	mNextWPMIndex = anIndex;
	if (9 < anIndex)
		mNextWPMIndex = 0;
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

// 0x412520: the saved game with the level configuration's double (nothing is stored when theSavedDifficulty is -1)
void UserProfile::RecordResultWithDouble(bool theStoreScore, int theSavedLives, int theSavedScore, double theSavedEmpCharge,
	int theSavedLevel, int theSavedDifficulty, int theSavedLevelNumber)
{
	if (theSavedDifficulty != -1)
	{
		mSavedLives = theSavedLives;
		if (!theStoreScore)
			theSavedScore = mSavedScoreBackup;
		mSavedScore = theSavedScore;
		mSavedEmpCharge = theSavedEmpCharge;
		mSavedDifficulty = theSavedDifficulty;
		mSavedLevel = theSavedLevel;
		mSavedLevelNumber = theSavedLevelNumber;
	}
}

// 0x412580: the same without the double (nothing is stored when theSavedDifficulty is -1)
void UserProfile::RecordResult(bool theStoreScore, int theSavedLives, int theSavedScore, int theSavedLevel, int theSavedDifficulty,
	int theSavedLevelNumber)
{
	if (theSavedDifficulty != -1)
	{
		mSavedLives = theSavedLives;
		if (!theStoreScore)
			theSavedScore = mSavedScoreBackup;
		mSavedScore = theSavedScore;
		mSavedDifficulty = theSavedDifficulty;
		mSavedLevel = theSavedLevel;
		mSavedLevelNumber = theSavedLevelNumber;
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

// added for W1
// 0x413d10: the lesson's record holds the sum of the speeds at +8 and the number of runs at +0xc
int UserProfile::GetAverageWPM(int theLesson)
{
	StatRecord& aRecord = mLessonRecords[theLesson];
	int aRuns = aRecord.mField[3];
	if (aRuns == 0)
		return 0;
	return aRecord.mField[2] / aRuns;
}

// 0x413d40: the sum of the adjusted speeds is at +0x18, the number of runs at +0x1c
int UserProfile::GetAverageAdjustedWPM(int theLesson)
{
	StatRecord& aRecord = mLessonRecords[theLesson];
	int aRuns = aRecord.mField[7];
	if (aRuns == 0)
		return 0;
	return aRecord.mField[6] / aRuns;
}

// 0x413d70: the sum of the accuracies is at +0x28, the number of runs at +0x2c
int UserProfile::GetAverageAccuracy(int theLesson)
{
	StatRecord& aRecord = mLessonRecords[theLesson];
	int aRuns = aRecord.mField[11];
	if (aRuns == 0)
		return 0;
	return aRecord.mField[10] / aRuns;
}

// 0x412610: a shark killed (in the abyss mode or not), counted for the profile and for the round
void UserProfile::CountSharkKilled(bool theAbyss)
{
	if (!theAbyss)
	{
		mSharksKilled = mSharksKilled + 1;
		mGameSharksKilled = mGameSharksKilled + 1;
		return;
	}
	mAbyssSharksKilled = mAbyssSharksKilled + 1;
	mGameSharksKilled = mGameSharksKilled + 1;
}

// 0x412640
void UserProfile::SetGameSharksKilled(int theValue)
{
	mGameSharksKilled = theValue;
}

// 0x412650
void UserProfile::SetGamePiranhasKilled(int theValue)
{
	mGamePiranhasKilled = theValue;
}

// 0x412660: a piranha killed (in the abyss mode or not), counted for the profile and for the round
void UserProfile::CountPiranhaKilled(bool theAbyss)
{
	if (!theAbyss)
	{
		mPiranhasKilled = mPiranhasKilled + 1;
		mGamePiranhasKilled = mGamePiranhasKilled + 1;
		return;
	}
	mAbyssPiranhasKilled = mAbyssPiranhasKilled + 1;
	mGamePiranhasKilled = mGamePiranhasKilled + 1;
}

// 0x412690: one more in mGemsFound[theIndex] and in mGameGemsFound[theIndex] (the registry GTT<i>)
void UserProfile::CountGemFound(int theIndex)
{
	mGemsFound[theIndex] = mGemsFound[theIndex] + 1;
	mGameGemsFound[theIndex] = mGameGemsFound[theIndex] + 1;
}

// 0x4126b0: one more in mSecretsFound[theIndex] and in mGameSecretsFound[theIndex] (the registry SGTT<i>)
void UserProfile::CountSecretFound(int theIndex)
{
	mSecretsFound[theIndex] = mSecretsFound[theIndex] + 1;
	mGameSecretsFound[theIndex] = mGameSecretsFound[theIndex] + 1;
}

// 0x4126d0
int UserProfile::GetAbyssBossIndex()
{
	return mAbyssBossIndex;
}

// 0x4126e0
void UserProfile::SetAbyssBossIndex(int theValue)
{
	mAbyssBossIndex = theValue;
}

// 0x4126f0
void UserProfile::ClearSavedGameTotals()
{
	mSavedTypingTime = 0;
	mSavedGameTime = 0;
	mSavedPausedTime = 0;
	mSavedLettersRight = 0;
	mSavedLettersWrong = 0;
}

// 0x412720: the value of the mode's group, and its best
void UserProfile::RecordGroupLevel(int theGroup, int theValue)
{
	mGroups[theGroup].mLastLevel = theValue;
	if (mGroups[theGroup].mBestLevel < theValue)
		mGroups[theGroup].mBestLevel = theValue;
}

// 0x412750: the result of a game of the mode (5 is the abyss mode, kept as -1 in mLastMode): the profile's last score
// and the mode's last and best score
void UserProfile::SetLastGame(int theMode, int theScore, int theLevelReached)
{
	if (theMode == 5)
		mLastMode = -1;
	else
		mLastMode = theMode;
	mLastLevelReached = theLevelReached;
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
// the six stat groups four ints and 35 records of twelve ints; mLessonOutcome (19); the 19 records at +0xe0; mBestBonusWordPoints; mGemsFound
// (6); mSecretsFound (4); mBestTreasureWords; mSavedLives; mSavedScoreBackup; the double * 100 as an int; mSavedLevel to mSavedLettersRight; the number of
// map pairs and the pairs; the number of node ids and the ids; mSavedTypingTime; the four kill counts.
// theCopyScoreToBackup makes mSavedScoreBackup the value of mSavedScore first.
void UserProfile::WriteDataToDisk(bool theCopyScoreToBackup)
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
		WriteFileInt(aBuffer, mGroups[g].mBestLevel);
		WriteFileInt(aBuffer, mGroups[g].mBestScore);
		WriteFileInt(aBuffer, mGroups[g].mLastLevel);
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
		WriteFileInt(aBuffer, mLessonOutcome[i]);

	i = 0;
	do
	{
		StatRecord aRecord = mLessonRecords[i];
		for (int k = 0; k < 12; k++)
			WriteFileInt(aBuffer, aRecord.mField[k]);
		i++;
	} while (i < NUM_RECORDS_E0);

	WriteFileInt(aBuffer, mBestBonusWordPoints);
	for (i = 0; i < 6; i++)
		WriteFileInt(aBuffer, mGemsFound[i]);
	for (i = 0; i < 4; i++)
		WriteFileInt(aBuffer, mSecretsFound[i]);
	WriteFileInt(aBuffer, mBestTreasureWords);

	if (theCopyScoreToBackup)
		mSavedScoreBackup = mSavedScore;
	WriteFileInt(aBuffer, mSavedLives);
	WriteFileInt(aBuffer, mSavedScoreBackup);
	WriteFileInt(aBuffer, (int)(mSavedEmpCharge * 100.0));       // _ftol
	WriteFileInt(aBuffer, mSavedLevel);
	WriteFileInt(aBuffer, mSavedDifficulty);
	WriteFileInt(aBuffer, mSavedLevelNumber);
	WriteFileInt(aBuffer, mSavedGameTime);
	WriteFileInt(aBuffer, mSavedPausedTime);
	WriteFileInt(aBuffer, mSavedLettersWrong);
	WriteFileInt(aBuffer, mSavedLettersRight);

	WriteFileInt(aBuffer, (int)mSavedRouteDots.size());
	unsigned int aPairIndex = 0;
	while (true)
	{
		if (aPairIndex >= mSavedRouteDots.size())
			break;
		WriteFileInt(aBuffer, mSavedRouteDots[aPairIndex]->mFirst);
		WriteFileInt(aBuffer, mSavedRouteDots[aPairIndex]->mSecond);
		aPairIndex++;
	}

	WriteFileInt(aBuffer, (int)mSavedRouteNodeIds.size());
	unsigned int aNodeIndex = 0;
	while (true)
	{
		if (aNodeIndex >= mSavedRouteNodeIds.size())
			break;
		WriteFileInt(aBuffer, mSavedRouteNodeIds[aNodeIndex]);
		aNodeIndex++;
	}

	WriteFileInt(aBuffer, mSavedTypingTime);
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

	mBestBonusWordPoints = 0;
	mBestTreasureWords = 0;
	for (i = 0; i < 6; i++)
		mGemsFound[i] = 0;
	mSecretsFound[0] = 0;
	mSecretsFound[1] = 0;
	mSecretsFound[2] = 0;
	mSecretsFound[3] = 0;

	mGameVersion = aBuffer.ReadString();
	double aFileVersion = atof(mGameVersion.c_str());
	atof(mApp->mProductVersion.c_str());
	if (aFileVersion < 0.48)
		return false;

	g = 0;
	do
	{
		mGroups[g].mBestLevel = ReadFileInt(aBuffer);
		mGroups[g].mBestScore = ReadFileInt(aBuffer);
		mGroups[g].mLastLevel = ReadFileInt(aBuffer);
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
		mLessonOutcome[i] = ReadFileInt(aBuffer);

	mLessonRecords.clear();
	i = 0;
	do
	{
		mLessonRecords.push_back(StatRecord());
		for (int k = 0; k < 12; k++)
		{
			mLessonRecords[i].mField[k] = ReadFileInt(aBuffer);
			if (aBuffer.AtEnd())
			{
				mLessonRecords.clear();
				for (int j = 0; j < NUM_RECORDS_E0; j++)
					mLessonRecords.push_back(StatRecord());
				return true;
			}
		}
		i++;
	} while (i < NUM_RECORDS_E0);

	if (!aBuffer.AtEnd())
		mBestBonusWordPoints = ReadFileInt(aBuffer);
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 6; i++)
			mGemsFound[i] = ReadFileInt(aBuffer);
	}
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 4; i++)
			mSecretsFound[i] = ReadFileInt(aBuffer);
	}
	if (!aBuffer.AtEnd())
		mBestTreasureWords = ReadFileInt(aBuffer);

	if (!aBuffer.AtEnd())
		mSavedLives = ReadFileInt(aBuffer);
	else
		mSavedLives = -1;
	if (!aBuffer.AtEnd())
		mSavedScore = ReadFileInt(aBuffer);
	else
		mSavedScore = -1;
	if (!aBuffer.AtEnd())
		mSavedEmpCharge = (double)ReadFileInt(aBuffer) * 0.01;
	else
		mSavedEmpCharge = -1.0;
	if (!aBuffer.AtEnd())
		mSavedLevel = ReadFileInt(aBuffer);
	else
		mSavedLevel = -1;
	if (!aBuffer.AtEnd())
		mSavedDifficulty = ReadFileInt(aBuffer);
	else
		mSavedDifficulty = -1;
	if (!aBuffer.AtEnd())
		mSavedLevelNumber = ReadFileInt(aBuffer);
	else
		mSavedLevelNumber = -1;
	if (!aBuffer.AtEnd())
		mSavedGameTime = ReadFileInt(aBuffer);
	else
		mSavedGameTime = -1;
	if (!aBuffer.AtEnd())
		mSavedPausedTime = ReadFileInt(aBuffer);
	else
		mSavedPausedTime = -1;
	if (!aBuffer.AtEnd())
		mSavedLettersWrong = ReadFileInt(aBuffer);
	else
		mSavedLettersWrong = -1;
	if (!aBuffer.AtEnd())
		mSavedLettersRight = ReadFileInt(aBuffer);
	else
		mSavedLettersRight = -1;

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
				mSavedRouteDots.push_back(aPair);
				aPairsLeft--;
			} while (aPairsLeft != 0);
		}
		int aNodeCount = ReadFileInt(aBuffer);
		if (0 < aNodeCount)
		{
			int aNodesLeft = aNodeCount;
			do
			{
				mSavedRouteNodeIds.push_back(ReadFileInt(aBuffer));
				aNodesLeft--;
			} while (aNodesLeft != 0);
		}
	}

	if (!aBuffer.AtEnd())
		mSavedTypingTime = ReadFileInt(aBuffer);
	if (!aBuffer.AtEnd())
	{
		for (i = 0; i < 4; i++)
			mKillCount[i] = ReadFileInt(aBuffer);
	}
	return true;
}
