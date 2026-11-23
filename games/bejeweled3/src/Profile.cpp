// Sexy::Profile: see the header.
#include "Profile.h"

#include "Bej3App.h"
#include "Buffer.h"
#include "Common.h"
#include "Dialog.h"

#include <cwctype>

using namespace Sexy;

namespace {

const char* const USERS_FILE_NAME = "users/users.dat";
const char* const PROFILE_FILE_NAME = "/profile.dat";

// The original's wcsicmp: two names are the same player when they differ in case only
bool SameName(const std::wstring& theFirst, const std::wstring& theSecond)
{
	if (theFirst.size() != theSecond.size())
		return false;
	for (size_t i = 0; i < theFirst.size(); i++) {
		if (std::towlower(theFirst[i]) != std::towlower(theSecond[i]))
			return false;
	}
	return true;
}

// FUN_0054fc70: bytes written as they are
void WriteRawBytes(Buffer* theBuffer, const void* theBytes, int theCount)
{
	theBuffer->WriteBytes((const uchar*)theBytes, theCount);
}

// FUN_00550390: bytes read as they are; the original's byte reader (0x54fd80) gives 0 once the data has run out
void ReadRawBytes(const Buffer& theBuffer, void* theBytes, int theCount)
{
	uchar* aBytes = (uchar*)theBytes;
	for (int i = 0; i < theCount; i++)
		aBytes[i] = theBuffer.AtEnd() ? 0 : theBuffer.ReadByte();
}

// 0x54fe80: four bytes, the first the lowest
int ReadLongOrZero(const Buffer& theBuffer)
{
	unsigned aValue = 0;
	for (int i = 0; i < 4; i++)
		aValue |= (unsigned)(theBuffer.AtEnd() ? 0 : theBuffer.ReadByte()) << (8 * i);
	return (int)aValue;
}

}  // namespace

// 0x71c4b0 (bejeweled3). port: the exe's temporary-folder move (0x71b940, when +0x80 is set) is not made: nothing sets it
Profile::~Profile()
{
	SavePlayer();
}

Profile::Profile()
{
	ResetPlayer();
}

void Profile::ResetPlayer()
{
	mIsNew = true;
	mFlagAfterText = false;
	mRank = 0;
	mLifetimePoints = 0;
	mRankSecond = 0;
	mSecondPoints = 0;
	mNumberAfterPoints = 0;
	mNumberAfterThat = 0;
	mSeenBits = 0;
	mQuestSection = 0;
	mQuestMenuFlag = false;
	for (int& aStat : mStats)
		aStat = 0;
	for (int& aCounter : mCounters)
		aCounter = 0;
	for (int& aLevel : mBadgeLevels)
		aLevel = 0;
	for (auto& aRow : mQuestsDone)
		for (uint8_t& aFlag : aRow)
			aFlag = 0;
	for (uint8_t& aFlag : mTutorialFlags)
		aFlag = 0;
	for (uint32_t& aFlag : mRankFlags)
		aFlag = 0;
	for (uint8_t& aFlag : mQuestHelpSeen)
		aFlag = 0;
	mFlagAfterTutorials = false;
	mNumberAfterTutorialFlag = 0;
	mShortAfterNumber = 0;
	mTemporary = false;
	mSettings.clear();
	mFraction = 0.42f;
	mOptionFirst = false;
	mOptionSecond = true;
	mTextOptionFirstOn = false;
	mTextOptionFirst.clear();
	mTextOptionSecondOn = false;
	mTextOptionSecond.clear();
	mTextOptionThirdOn = false;
	mTextOptionThird.clear();
	mFractionFirst = 0.5f;
	mFlagBeforeFractions = false;
	mFractionSecond = 0.5f;
}

std::string Profile::GetUserFolder(const std::wstring& theName)
{
	// 0x71b640: the name as bytes (every character cut to one); each below 'A' other than a space or a digit (and
	// every one past 0x7f, which a signed byte reads as negative) becomes '_'
	std::string aFolderName;
	for (wchar_t aChar : theName)
		aFolderName += (char)aChar;
	for (char& aChar : aFolderName) {
		if (aChar != ' ' && aChar < 'A' && (aChar < '0' || '9' < aChar))
			aChar = '_';
	}
	if (theName.empty())
		return "users/_temp";
	return "users/" + aFolderName;
}

int Profile::GetRankForPoints(uint64_t thePoints)
{
	uint64_t aTotal = 0;
	int aRank = 0;
	while (true) {
		aTotal += 150000 + (uint64_t)aRank * 100000;
		if (thePoints < aTotal)
			return aRank;
		aRank++;
	}
}

bool Profile::AddUserName(const std::wstring& theName)
{
	for (const std::wstring& aUserName : mUserNames) {
		if (SameName(aUserName, theName))
			return false;
	}
	mUserNames.push_back(theName);
	return true;
}

bool Profile::RemoveUserName(const std::wstring& theName)
{
	bool wasThere = false;
	for (size_t i = 0; i < mUserNames.size();) {
		if (SameName(mUserNames[i], theName)) {
			mUserNames.erase(mUserNames.begin() + i);
			wasThere = true;
		} else {
			i++;
		}
	}
	return wasThere;
}

void Profile::LoadUsers()
{
	Buffer aFile;
	if (!gApp->ReadBufferFromFile(USERS_FILE_NAME, &aFile))
		return;
	if (ReadLongOrZero(aFile) != FILE_MAGIC || aFile.ReadShort() != USERS_FILE_VERSION)
		return;
	bool isListChanged = false;
	// The player and the second name the file begins with are read and not kept
	aFile.ReadUTF8String();
	aFile.ReadUTF8String();
	ReadRawBytes(aFile, mUsersBlockFirst, USERS_BLOCK_SIZE);
	ReadRawBytes(aFile, mUsersBlockSecond, USERS_BLOCK_SIZE);
	mUsersFileCount = ReadLongOrZero(aFile);
	ReadRawBytes(aFile, mUsersBlockThird, USERS_BLOCK_SIZE);
	ReadRawBytes(aFile, mUsersBlockFourth, USERS_BLOCK_SIZE);
	while (true) {
		std::wstring aUserName = aFile.ReadUTF8String();
		if (aUserName.empty())
			break;
		if (!LoadPlayer(aUserName)) {
			// port: 0x71ba40 also has the platform layer delete the player's folder (slot 26 of the object at 0x8e1734);
			// the port never deletes a save, so the unreadable folder stays
			isListChanged = true;
			RemoveUserName(aUserName);
		} else {
			AddUserName(aUserName);
		}
	}
	if (isListChanged)
		SaveUsers();
}

// 0x71c820 (bejeweled3). port: the exe's temporary-folder move (0x71b940, when +0x80 is set) is not made: nothing sets it
bool Profile::CreateProfile(const std::wstring& theName)
{
	if (theName.empty() || AddUserName(theName)) {
		SavePlayer();
		ResetPlayer();
		mName = theName;
		SaveUsers();
		return true;
	}
	// the name matches a player already there: its text is the one the list has
	std::wstring aShownName = theName;
	for (const std::wstring& aUserName : mUserNames) {
		if (SameName(aUserName, theName)) {
			aShownName = aUserName;
			break;
		}
	}
	std::string aLines = StrFormat(gApp->GetString("412", "There is already a user named %s").c_str(),
		WStringToString(aShownName).c_str());
	gApp->DoDialog(0, true, gApp->GetString("413", "SAME NAME"), aLines, gApp->GetString("414", "OK"), Dialog::BUTTONS_FOOTER);
	return false;
}

void Profile::SaveUsers()
{
	Buffer aFile;
	aFile.WriteLong(FILE_MAGIC);
	aFile.WriteShort(USERS_FILE_VERSION);
	aFile.WriteUTF8String(mName);
	gApp->RegistryWriteString("LastUser", WStringToString(mName));
	aFile.WriteUTF8String(mUsersFileSecondName);
	WriteRawBytes(&aFile, mUsersBlockFirst, USERS_BLOCK_SIZE);
	WriteRawBytes(&aFile, mUsersBlockSecond, USERS_BLOCK_SIZE);
	aFile.WriteLong(mUsersFileCount);
	WriteRawBytes(&aFile, mUsersBlockThird, USERS_BLOCK_SIZE);
	WriteRawBytes(&aFile, mUsersBlockFourth, USERS_BLOCK_SIZE);
	for (const std::wstring& aUserName : mUserNames)
		aFile.WriteUTF8String(aUserName);
	gApp->WriteBufferToFile(USERS_FILE_NAME, &aFile);
}

bool Profile::LoadPlayer(const std::wstring& theName)
{
	Buffer aFile;
	if (!gApp->ReadBufferFromFile(GetUserFolder(theName) + PROFILE_FILE_NAME, &aFile))
		return false;
	if (ReadLongOrZero(aFile) != FILE_MAGIC)
		return false;
	int aVersion = ReadLongOrZero(aFile);
	if (aVersion < MIN_FILE_VERSION)
		return false;

	mName = theName;
	mIsNew = false;
	mRank = ReadLongOrZero(aFile);
	for (int& aStat : mStats)
		aStat = ReadLongOrZero(aFile);
	for (int& aCounter : mCounters)
		aCounter = ReadLongOrZero(aFile);
	for (int& aLevel : mBadgeLevels)
		aLevel = ReadLongOrZero(aFile);
	for (uint8_t& aFlag : mTutorialFlags)
		aFlag = aFile.ReadBoolean();
	mFlagAfterTutorials = aFile.ReadBoolean();
	mNumberAfterTutorialFlag = ReadLongOrZero(aFile);
	mShortAfterNumber = aFile.ReadShort();
	if (aVersion > 0x42) {
		mQuestSection = ReadLongOrZero(aFile);
		mQuestMenuFlag = aFile.ReadBoolean();
	}
	for (auto& aRow : mQuestsDone) {
		for (uint8_t& aFlag : aRow)
			aFlag = aFile.ReadBoolean();
	}
	mProfileTextFirst = aFile.ReadUTF8String();
	mProfileTextSecond = aFile.ReadUTF8String();
	mFlagAfterText = aFile.ReadBoolean();
	mSettings.clear();
	int aSettingCount = ReadLongOrZero(aFile);
	for (int i = 0; i < aSettingCount; i++) {
		std::wstring aSettingName = aFile.ReadUTF8String();
		std::wstring aSettingValue = aFile.ReadUTF8String();
		mSettings[aSettingName] = aSettingValue;
	}
	for (uint32_t& aFlag : mRankFlags)
		aFlag = aFile.AtEnd() ? 0 : aFile.ReadByte();
	mRankSecond = ReadLongOrZero(aFile);
	mSecondPoints = (uint32_t)ReadLongOrZero(aFile);
	if (aVersion < 0x3d) {
		mSecondPoints = (uint32_t)mStats[1];
		mLifetimePoints = 0;
	} else {
		mLifetimePoints = (uint32_t)ReadLongOrZero(aFile);
	}
	mNumberAfterPoints = ReadLongOrZero(aFile);
	mNumberAfterThat = ReadLongOrZero(aFile);
	mSeenBits = (uint32_t)ReadLongOrZero(aFile);
	if (aVersion > 0x3c) {
		mLifetimePoints |= (uint64_t)(uint32_t)ReadLongOrZero(aFile) << 32;
		mSecondPoints |= (uint64_t)(uint32_t)ReadLongOrZero(aFile) << 32;
	}
	mRankSecond = GetRankForPoints(mSecondPoints);
	mRank = GetRankForPoints(mLifetimePoints);
	mOptionFirst = aFile.ReadBoolean();
	mOptionSecond = aFile.ReadBoolean();
	ReadRawBytes(aFile, &mFraction, 4);
	mTextOptionFirstOn = aFile.ReadBoolean();
	mTextOptionFirst = aFile.ReadString();
	mTextOptionSecondOn = aFile.ReadBoolean();
	mTextOptionSecond = aFile.ReadString();
	mTextOptionThirdOn = aFile.ReadBoolean();
	mTextOptionThird = aFile.ReadString();
	mFlagBeforeFractions = aFile.ReadBoolean();
	ReadRawBytes(aFile, &mFractionFirst, 4);
	ReadRawBytes(aFile, &mFractionSecond, 4);
	if (aVersion > 0x44) {
		// The short before the flags says how many there are; only a full set of 0x28 is kept
		int aHintFlagCount = aFile.ReadShort();
		bool isFullSet = aHintFlagCount == HINT_FLAG_COUNT;
		for (int i = 0; i < aHintFlagCount; i++) {
			uint8_t aFlag = aFile.ReadBoolean();
			if (isFullSet)
				mQuestHelpSeen[i] = aFlag;
		}
	}
	AddUserName(theName);
	return true;
}

void Profile::SavePlayer()
{
	// 0x71bb10 saves only when a player is loaded: the length of the name (+0x74) is not 0
	if (mName.empty())
		return;
	// port: the original makes the player's folder first (0x43b4b0 and 0x43b460); the framework's file writing does
	Buffer aFile;
	aFile.WriteLong(FILE_MAGIC);
	aFile.WriteLong(FILE_VERSION);
	aFile.WriteLong(mRank);
	for (int aStat : mStats)
		aFile.WriteLong(aStat);
	for (int aCounter : mCounters)
		aFile.WriteLong(aCounter);
	for (int aLevel : mBadgeLevels)
		aFile.WriteLong(aLevel);
	for (uint8_t aFlag : mTutorialFlags)
		aFile.WriteBoolean(aFlag != 0);
	aFile.WriteBoolean(mFlagAfterTutorials);
	aFile.WriteLong(mNumberAfterTutorialFlag);
	aFile.WriteShort((short)mShortAfterNumber);
	aFile.WriteLong(mQuestSection);
	aFile.WriteBoolean(mQuestMenuFlag);
	for (const auto& aRow : mQuestsDone) {
		for (uint8_t aFlag : aRow)
			aFile.WriteBoolean(aFlag != 0);
	}
	aFile.WriteUTF8String(mProfileTextFirst);
	aFile.WriteUTF8String(mProfileTextSecond);
	aFile.WriteBoolean(mFlagAfterText);
	aFile.WriteLong((long)mSettings.size());
	for (const auto& aSetting : mSettings) {
		aFile.WriteUTF8String(aSetting.first);
		aFile.WriteUTF8String(aSetting.second);
	}
	for (uint32_t aFlag : mRankFlags)
		aFile.WriteByte((uchar)aFlag);
	aFile.WriteLong(mRankSecond);
	aFile.WriteLong((long)(uint32_t)mSecondPoints);
	aFile.WriteLong((long)(uint32_t)mLifetimePoints);
	aFile.WriteLong(mNumberAfterPoints);
	aFile.WriteLong(mNumberAfterThat);
	aFile.WriteLong((long)mSeenBits);
	aFile.WriteLong((long)(uint32_t)(mLifetimePoints >> 32));
	aFile.WriteLong((long)(uint32_t)(mSecondPoints >> 32));
	aFile.WriteBoolean(mOptionFirst);
	aFile.WriteBoolean(mOptionSecond);
	WriteRawBytes(&aFile, &mFraction, 4);
	aFile.WriteBoolean(mTextOptionFirstOn);
	aFile.WriteString(mTextOptionFirst);
	aFile.WriteBoolean(mTextOptionSecondOn);
	aFile.WriteString(mTextOptionSecond);
	aFile.WriteBoolean(mTextOptionThirdOn);
	aFile.WriteString(mTextOptionThird);
	aFile.WriteBoolean(mFlagBeforeFractions);
	WriteRawBytes(&aFile, &mFractionFirst, 4);
	WriteRawBytes(&aFile, &mFractionSecond, 4);
	aFile.WriteShort(HINT_FLAG_COUNT);
	for (uint8_t aFlag : mQuestHelpSeen)
		aFile.WriteBoolean(aFlag != 0);
	gApp->WriteBufferToFile(GetUserFolder(mName) + PROFILE_FILE_NAME, &aFile);
}

bool Profile::SelectUser(const std::wstring& theName)
{
	// port: a temporary player's folder is moved by 0x71b940 before another is chosen; temporary players come from
	// the new-player dialog, which is translated later
	SavePlayer();
	if (!LoadPlayer(theName)) {
		RemoveUserName(theName);
		return false;
	}
	// port: Steam's achievements and stats for the loaded player (0x71c000, 0x71a700) are left out
	return true;
}

bool Profile::SelectFirstLoadableUser()
{
	for (size_t i = 0; i < mUserNames.size();) {
		std::wstring aUserName = mUserNames[i];
		SavePlayer();
		if (LoadPlayer(aUserName))
			return true;
		RemoveUserName(aUserName);
	}
	return false;
}
