#include "UserProfile.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Buffer.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

namespace
{

// The kind of each setting, in the order of the constructor's table (0x42dc60): {id, kind}. 13 (the name) and 15 (the
// product name) are strings (kind 2); 16 (the version), 80 and 82 are floats (kind 1); every other setting is an int.
const int kKinds[][2] = {
	{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {12, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 0}, {13, 2},
	{14, 0}, {15, 2}, {16, 1}, {17, 0}, {19, 0}, {20, 0}, {22, 0}, {115, 0}, {24, 0}, {26, 0}, {28, 0}, {30, 0}, {32, 0},
	{34, 0}, {36, 0}, {38, 0}, {40, 0}, {21, 0}, {23, 0}, {25, 0}, {27, 0}, {29, 0}, {31, 0}, {33, 0}, {35, 0}, {37, 0},
	{39, 0}, {41, 0}, {46, 0}, {47, 0}, {49, 0}, {48, 0}, {50, 0}, {51, 0}, {52, 0}, {53, 0}, {54, 0}, {55, 0}, {58, 0},
	{59, 0}, {60, 0}, {61, 0}, {62, 0}, {63, 0}, {64, 0}, {114, 0}, {65, 0}, {66, 0}, {18, 0}, {67, 0}, {68, 0}, {69, 0},
	{70, 0}, {71, 0}, {72, 0}, {73, 0}, {74, 0}, {79, 0}, {80, 1}, {81, 0}, {82, 1}, {83, 0}, {84, 0}, {85, 0}, {95, 0},
	{42, 0}, {43, 0}, {44, 0}, {45, 0}, {111, 0}, {112, 0}, {86, 0}, {87, 0}, {88, 0}, {89, 0}, {90, 0}, {91, 0}, {92, 0},
	{93, 0}, {96, 0}, {94, 0}, {97, 0}, {98, 0}, {99, 0}, {100, 0}, {113, 0}, {101, 0}, {102, 0}, {103, 0}, {104, 0},
	{105, 0}, {106, 0}, {107, 0}, {108, 0}, {109, 0}, {110, 0}, {75, 0}, {76, 0}, {77, 0}, {78, 0}};

}  // namespace

UserProfile::UserProfile() : mLoaded(false), mValid(true), mUnknown62(true)
{
	for (size_t i = 0; i < sizeof(kKinds) / sizeof(kKinds[0]); i++)
		mKinds[kKinds[i][0]] = kKinds[i][1];

	// The original sets the product name (id 0xf) and the version (id 0x10) from the app, then resets the scores
	// (the constructor's last three calls, 0x4271a0, 0x424400, 0x42dbd0)
	SetString(15, gApp->mProdName);
	float aVersion = 0;
	sscanf(gApp->mProductVersion.c_str(), "%f", &aVersion);
	SetFloat(16, aVersion);
	mValid = true;
	mLoaded = false;
	ClearScores();
}

UserProfile::~UserProfile()
{
}

bool UserProfile::GetInt(int theId, int* theValue) const
{
	std::map<int, int>::const_iterator anItr = mInts.find(theId);
	if (anItr == mInts.end())
		return false;
	*theValue = anItr->second;
	return true;
}

bool UserProfile::GetFloat(int theId, float* theValue) const
{
	std::map<int, float>::const_iterator anItr = mFloats.find(theId);
	if (anItr == mFloats.end())
		return false;
	*theValue = anItr->second;
	return true;
}

bool UserProfile::GetString(int theId, std::string* theValue) const
{
	std::map<int, std::string>::const_iterator anItr = mStrings.find(theId);
	if (anItr == mStrings.end())
		return false;
	*theValue = anItr->second;
	return true;
}

void UserProfile::SetInt(int theId, int theValue)
{
	mInts[theId] = theValue;
}

void UserProfile::SetFloat(int theId, float theValue)
{
	mFloats[theId] = theValue;
}

void UserProfile::SetString(int theId, const std::string& theValue)
{
	mStrings[theId] = theValue;
}

// 0x40e8b0: the unlock flags (id 0x30) count only while they equal the check value (id 0x32) minus 0x193
bool UserProfile::HasModeUnlocked(int theMode) const
{
	int aFlags = -1;
	int aCheck = 0;
	GetInt(PROFILE_UNLOCK_A, &aFlags);
	GetInt(PROFILE_UNLOCK_B, &aCheck);
	if (aFlags == aCheck - 0x193)
	{
		if (theMode == 2)
			return (((int) (char) aFlags) & 4) >> 2;
		if (theMode == 3)
			return (((int) (char) aFlags) & 0x40) >> 6;
	}
	return false;
}

// 0x424540: no saved game (the original sets setting 99 twice)
void UserProfile::ResetGame()
{
	SetInt(0x4f, -1);
	SetInt(0x51, -1);
	SetInt(0x59, -1);
	SetFloat(0x50, 0);
	SetInt(0x53, 0);
	SetInt(0x54, 0);
	SetInt(0x55, 0);
	SetInt(0x56, 0);
	SetInt(100, 0);
	SetInt(0x57, 0);
	SetInt(0x58, 0);
	for (int i = 0x5a; i <= 0x62; i++)
		SetInt(i, 0);
	SetInt(99, 0);
	SetInt(99, 0);
	SetFloat(0x52, 0);
}

// 0x42dbd0
void UserProfile::ClearScores()
{
	mScores.clear();
	mScores.resize(20);
	for (size_t i = 0; i < mScores.size(); i++)
		mScores[i].resize(10);   // 0x423510: ten (0, 0) pairs
}

// 0x42eca0: the registry first (ReadRegistry), then the player's file
bool UserProfile::Load(float* theVersion, std::string* theName, bool theSkipFile)
{
	bool aRegistryOk = ReadRegistry();
	if (theSkipFile)
		return aRegistryOk;

	int aNumber = 0;
	if (!GetInt(PROFILE_NUMBER, &aNumber))
	{
		mLoaded = false;
		return false;
	}

	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(StrFormat("users\\data%d.usr", aNumber), &aBuffer))
	{
		mLoaded = false;
		return false;
	}

	if (aBuffer.AtEnd())
		return false;
	std::string aString = aBuffer.ReadString();
	if (theName != NULL)
		*theName = aString;
	if (aBuffer.AtEnd())
		return false;
	aString = aBuffer.ReadString();
	if (theVersion != NULL)
	{
		*theVersion = (float) atof(aString.c_str());
	}

	for (int anId = 0xf; anId < NUM_PROFILE_SETTINGS; anId++)
	{
		aString = aBuffer.ReadString();
		int aKind = mKinds[anId];
		if (aKind == 0)
			mInts[anId] = atoi(aString.c_str());
		else if (aKind == 1)
		{
			float aValue = 0;
			sscanf(aString.c_str(), "%f", &aValue);
			mFloats[anId] = aValue;
		}
		else if (aKind == 2)
			mStrings[anId] = aString;
	}

	ClearScores();
	for (size_t i = 0; i < mScores.size(); i++)
	{
		for (size_t j = 0; j < mScores[i].size(); j++)
		{
			mScores[i][j].mValue = aBuffer.ReadLong();
			mScores[i][j].mAux = aBuffer.ReadLong();
		}
	}

	if (!aBuffer.AtEnd())
	{
		for (int aList = 0; aList < 2; aList++)
		{
			std::vector<ProfileRecord>& aRecords = aList == 0 ? mRecordsB : mRecordsA;
			int aCount = aBuffer.ReadLong();
			for (; aCount > 0; aCount--)
			{
				ProfileRecord aRecord;
				for (int k = 0; k < 6; k++)
					aRecord.mField[k] = aBuffer.ReadLong();
				aRecords.push_back(aRecord);
			}
		}
	}
	mLoaded = true;
	return true;
}

// 0x4276b0
bool UserProfile::Save()
{
	if (!mValid)
		return false;
	WriteRegistry();
	if (!mValid)
		return false;

	Buffer aBuffer;
	int aNumber = 0;
	GetInt(PROFILE_NUMBER, &aNumber);
	std::string aPath = StrFormat("users\\data%d.usr", aNumber);
	aBuffer.WriteString(gApp->mProdName);
	aBuffer.WriteString(gApp->mProductVersion);
	for (int anId = 0xf; anId < NUM_PROFILE_SETTINGS; anId++)
	{
		int aKind = mKinds[anId];
		if (aKind == 0)
			aBuffer.WriteString(StrFormat("%d", mInts[anId]));
		else if (aKind == 1)
			aBuffer.WriteString(StrFormat("%f", (double) mFloats[anId]));
		else if (aKind == 2)
			aBuffer.WriteString(mStrings[anId]);
	}
	for (size_t i = 0; i < mScores.size(); i++)
	{
		for (size_t j = 0; j < mScores[i].size(); j++)
		{
			aBuffer.WriteLong(mScores[i][j].mValue);
			aBuffer.WriteLong(mScores[i][j].mAux);
		}
	}
	for (int aList = 0; aList < 2; aList++)
	{
		const std::vector<ProfileRecord>& aRecords = aList == 0 ? mRecordsB : mRecordsA;
		int aCount = aRecords.size() > 5 ? 5 : (int) aRecords.size();
		aBuffer.WriteLong(aCount);
		for (int i = 0; i < aCount; i++)
			for (int k = 0; k < 6; k++)
				aBuffer.WriteLong(aRecords[i].mField[k]);
	}
	if (!gApp->WriteBufferToFile(aPath, &aBuffer))
	{
		gApp->Popup(GT(0x5625e0));
		return false;
	}
	return true;
}

// 0x425b30: ids below 0xf go to the registry under Users\<name>\<id>
void UserProfile::WriteRegistry()
{
	std::string aName;
	if (!GetString(PROFILE_NAME, &aName))
		gApp->Popup(GT(0x562518));
	std::string aPrefix = StrFormat("Users\\%s\\", aName.c_str());
	gApp->RegistryWriteString("Users\\LastUser", aName);

	int aNumber = 0;
	if (!GetInt(PROFILE_NUMBER, &aNumber))
		SetInt(PROFILE_NUMBER, NextUserNumber());

	for (std::map<int, int>::iterator anItr = mInts.begin(); anItr != mInts.end(); ++anItr)
	{
		if (anItr->first < 0xf)
		{
			if (!gApp->RegistryWriteInteger(StrFormat("%s%d", aPrefix.c_str(), anItr->first), anItr->second))
			{
				mValid = false;
				return;
			}
		}
	}
	for (std::map<int, float>::iterator anItr = mFloats.begin(); anItr != mFloats.end(); ++anItr)
	{
		if (anItr->first < 0xf)
		{
			if (!gApp->RegistryWriteString(StrFormat("%s%d", aPrefix.c_str(), anItr->first),
					StrFormat("%f", (double) anItr->second)))
			{
				mValid = false;
				return;
			}
		}
	}
	for (std::map<int, std::string>::iterator anItr = mStrings.begin(); anItr != mStrings.end(); ++anItr)
	{
		if (anItr->first < 0xf)
		{
			if (!gApp->RegistryWriteString(StrFormat("%s%d", aPrefix.c_str(), anItr->first), anItr->second))
				mValid = false;
		}
	}
}

// 0x427200: the settings 0..0xe from the registry; false when the profile has no number
bool UserProfile::ReadRegistry()
{
	std::string aName;
	GetString(PROFILE_NAME, &aName);
	bool aResult = true;
	std::string aPrefix = StrFormat("Users\\%s\\", aName.c_str());

	// An old version kept its version number (id 0x10) in the registry; such a profile cannot be read
	std::string aVersionString;
	if (gApp->RegistryReadString(StrFormat("%s%d", aPrefix.c_str(), 0x10), &aVersionString))
	{
		float aVersion = 0;
		sscanf(aVersionString.c_str(), "%f", &aVersion);
		if (aVersion < 0.2f)
		{
			gApp->Popup(GT(0x562570));
			gApp->Shutdown();
		}
	}

	for (int anId = 0; anId <= 0xe; anId++)
	{
		int aKind = mKinds[anId];
		std::string aKey = StrFormat("%s%d", aPrefix.c_str(), anId);
		if (aKind == 0)
		{
			int aValue = 0;
			if (gApp->RegistryReadInteger(aKey, &aValue))
				mInts[anId] = aValue;
			else if (anId == PROFILE_NUMBER)
				aResult = false;
		}
		else if (aKind > 0 && aKind < 3)
		{
			std::string aValue;
			if (gApp->RegistryReadString(aKey, &aValue))
			{
				if (aKind == 2)
					mStrings[anId] = aValue;
				else
				{
					float aFloat = 0;
					sscanf(aValue.c_str(), "%f", &aFloat);
					mFloats[anId] = aFloat;
				}
			}
		}
	}
	return aResult;
}

// 0x4258b0: the next player number, kept in Users\LastUserNum (found from the players' own numbers the first time)
int UserProfile::NextUserNumber()
{
	int aLast = 0;
	if (!gApp->RegistryReadInteger("Users\\LastUserNum", &aLast))
	{
		aLast = 0;
		StringVector aNames;
		if (gApp->RegistryGetSubKeys("Users", &aNames))
		{
			for (size_t i = 0; i < aNames.size(); i++)
			{
				int aNumber = -1;
				if (gApp->RegistryReadInteger(StrFormat("Users\\%s\\%d", aNames[i].c_str(), PROFILE_NUMBER), &aNumber) &&
					aNumber > aLast)
					aLast = aNumber;
			}
		}
	}
	aLast++;
	gApp->RegistryWriteInteger("Users\\LastUserNum", aLast);
	return aLast;
}
