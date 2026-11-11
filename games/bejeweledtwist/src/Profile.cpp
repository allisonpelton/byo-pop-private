// The players (Profile.h).
#include "Profile.h"

#include "GameText.h"
#include "WinTwistApp.h"

#include <cwchar>

using namespace Sexy;

namespace {

const long kMagic = 0x691beef4;
const short kUsersVersion = 5;
const long kProfileVersion = 0x34;

int CompareNoCase(const SexyString& a, const SexyString& b)
{
	// The original compares its wide names with _wcsicmp, which folds only A-Z; so does _stricmp
	return _stricmp(a.c_str(), b.c_str());
}

// The original keeps three bit sets as 4-byte strings in the file (from version 0x30)
void ReadFlags(const Buffer& theBuffer, int theVersion, unsigned int* theFlags)
{
	if (theVersion < 0x30) {
		*theFlags = (unsigned int)theBuffer.ReadLong();
	} else {
		std::string aString = theBuffer.ReadString();
		unsigned char aBytes[4] = {};
		for (int i = 0; i < 4 && i < (int)aString.size(); i++)
			aBytes[i] = (unsigned char)aString[i];
		*theFlags = aBytes[0] | (aBytes[1] << 8) | (aBytes[2] << 16) | ((unsigned int)aBytes[3] << 24);
	}
}

void WriteFlags(Buffer& theBuffer, unsigned int theFlags)
{
	std::string aString(4, '\0');
	for (int i = 0; i < 4; i++)
		aString[i] = (char)((theFlags >> (i * 8)) & 0xff);
	theBuffer.WriteString(aString);
}

}  // namespace

void Profile::SavedGame::Read(const Buffer& theBuffer)
{
	mReplayStart = theBuffer.ReadShort();
	mReplayEnd = theBuffer.ReadShort();
	mBest = theBuffer.ReadLong();
	ByteVector aData;
	theBuffer.ReadBuffer(&aData);
	mData.SetData(aData);
}

void Profile::SavedGame::Write(Buffer& theBuffer) const
{
	theBuffer.WriteShort((short)mReplayStart);
	theBuffer.WriteShort((short)mReplayEnd);
	theBuffer.WriteLong(mBest);
	theBuffer.WriteBuffer(mData.mData);
}

Profile::Profile()
{
	for (SavedGame& aGame : mSavedGames) {
		aGame.mReplayStart = 0;
		aGame.mReplayEnd = 0;
		aGame.mBest = 0;
	}
}

Profile::~Profile() = default;

// 0x733e9a: the user folder scan, inline here
std::string Profile::GetUserFolder(const SexyString& theName)
{
	// 0x733e9a: the name as a file name
	std::string aFileName = SexyStringToString(theName);
	for (char& c : aFileName) {
		if (c != ' ' && c < 'A' && (c < '0' || '9' < c))
			c = '_';
	}
	return GetAppDataFolder() + "users\\" + aFileName;
}

void Profile::LoadUsers(SexyString* theLastName)
{
	Profile aCheck;
	bool aDropped = false;
	mNames.clear();
	theLastName->clear();
	Buffer aBuffer;
	std::string aPath = GetAppDataFolder() + "users\\users.dat";
	if (!gApp->ReadBufferFromFile(aPath, &aBuffer, true))
		return;
	long aMagic = aBuffer.ReadLong();
	short aVersion = aBuffer.ReadShort();
	if (aVersion == kUsersVersion && aMagic == kMagic) {
		*theLastName = WStringToSexyString(aBuffer.ReadUTF8String());
		SexyString aName = WStringToSexyString(aBuffer.ReadUTF8String());
		if (aName.empty())
			return;
		do {
			if (!aCheck.Load(aName)) {
				aDropped = true;
				Delete(aName);
			} else {
				AddName(aName);
			}
			aName = WStringToSexyString(aBuffer.ReadUTF8String());
		} while (!aName.empty());
		if (!aDropped)
			return;
	}
	SaveUsers();
}

void Profile::SaveUsers()
{
	if (!gApp->mLoaded)
		return;
	if (mName == GT(0x7da070))  // the tutorial's player is never written
		return;
	Buffer aBuffer;
	aBuffer.WriteLong(kMagic);
	aBuffer.WriteShort(kUsersVersion);
	aBuffer.WriteUTF8String(SexyStringToWString(mName));
	for (int i = 0; i < (int)mNames.size(); i++)
		aBuffer.WriteUTF8String(SexyStringToWString(mNames[i]));
	gApp->WriteBufferToFile(GetAppDataFolder() + "users\\users.dat", &aBuffer);
}

bool Profile::Load(const SexyString& theName)
{
	Buffer aBuffer;
	std::string aPath = GetUserFolder(theName) + "\\profile.dat";
	if (!gApp->ReadBufferFromFile(aPath, &aBuffer, false) || aBuffer.ReadLong() != kMagic)
		return false;
	int aVersion = aBuffer.ReadLong();
	if (aVersion < 0x33)
		return false;

	mLastChallengeCategory = -1;
	mName = theName;
	ClearMap();
	mIsNew = false;
	mHighestScore = aBuffer.ReadLong();
	mTotalLevelsDelta = aBuffer.ReadLong();
	mTotalPowerGems = aBuffer.ReadLong();
	if (aVersion < 0x34)
		aBuffer.ReadLong();
	mBestChain = aBuffer.ReadLong();
	if (aVersion < 0x34) {
		aBuffer.ReadLong();
		aBuffer.ReadLong();
	}
	mTotalFlames = aBuffer.ReadLong();
	mTotalElectros = aBuffer.ReadLong();
	mAchievementBits = aBuffer.ReadLong();
	mPuzzlesSolved = aBuffer.ReadLong();
	mTotalPoints = aBuffer.ReadLong();
	mFirstGameTime = aBuffer.ReadLong();
	for (int i = 0; i < 6; i++) {
		mBestLevels[i] = aBuffer.ReadLong();
		mBestScores[i] = aBuffer.ReadLong();
	}
	mTotalPoints = aBuffer.ReadLong();
	mGemsCleared = aBuffer.ReadLong();
	ReadFlags(aBuffer, aVersion, &mGemTypesUnlocked);
	aBuffer.ReadBytes(mSavedBits, 0x40);
	mGamesPlayed = aBuffer.ReadLong();
	mStartingRank = aBuffer.ReadLong();
	mShowReminder = aBuffer.ReadByte() != 0;
	ReadFlags(aBuffer, aVersion, &mTipsSeen);
	for (int i = 0; i < 10; i++)
		mSavedLongs[i] = aBuffer.ReadLong();
	mLastModePlayed = aBuffer.ReadLong();
	mBeforeFirstMove = aBuffer.ReadByte() != 0;
	ReadFlags(aBuffer, aVersion, &mModesUnlocked);
	mRank = aBuffer.ReadLong();
	mSavedBools.clear();
	int aCount = aBuffer.ReadShort();
	for (int i = aCount; i > 0; i--)
		mSavedBools.push_back(aBuffer.ReadBoolean());
	mRankPoints = aBuffer.ReadLong();
	mReplayCount = aBuffer.ReadLong();
	mCustomCursors = aBuffer.ReadByte() != 0;
	gApp->EnableCustomCursors(mCustomCursors);
	int aSlots = aBuffer.ReadShort();
	mChallengeStages.clear();
	for (int i = aSlots; i > 0; i--)
		mChallengeStages.push_back(aBuffer.ReadShort());
	if (aVersion < 0x2c)
		aBuffer.ReadByte();
	mChallengeBests.clear();
	for (int i = aSlots; i > 0; i--)
		mChallengeBests.push_back(aBuffer.ReadLong());
	mModesSeenBits = aVersion < 0x2a ? 0 : aBuffer.ReadLong();
	for (int i = 0; i < 6; i++)
		mGamesByMode[i] = aVersion < 0x2b ? 0 : aBuffer.ReadLong();
	if (0x2e < aVersion) {
		for (SavedGame& aGame : mSavedGames)
			aGame.Read(aBuffer);
	}
	// With the app's list at +0xf5c (comes with the challenges) both vectors are padded with zeros to its length
	AddName(mName);
	return aBuffer.AtEnd() && !aBuffer.PastEnd();
}

void Profile::Save()
{
	Buffer aBuffer;
	std::string aPath;
	if (!GetSaveData(&aPath, &aBuffer))
		return;
	MkDir(GetFileDir(aPath, false));
	gApp->WriteBufferToFile(aPath, &aBuffer);
}

// port (web): Save's file and its contents, for WinTwistApp::GetShutdownSaves as well
bool Profile::GetSaveData(std::string* thePath, Buffer* theBuffer)
{
	if (mName.empty() || mName == GT(0x7da070))
		return false;
	Buffer& aBuffer = *theBuffer;
	*thePath = GetUserFolder(mName) + "\\profile.dat";
	aBuffer.WriteLong(kMagic);
	aBuffer.WriteLong(kProfileVersion);
	aBuffer.WriteLong(mHighestScore);
	aBuffer.WriteLong(mTotalLevelsDelta);
	aBuffer.WriteLong(mTotalPowerGems);
	aBuffer.WriteLong(mBestChain);
	aBuffer.WriteLong(mTotalFlames);
	aBuffer.WriteLong(mTotalElectros);
	aBuffer.WriteLong(mAchievementBits);
	aBuffer.WriteLong(mPuzzlesSolved);
	aBuffer.WriteLong(mTotalPoints);
	aBuffer.WriteLong(mFirstGameTime);
	for (int i = 0; i < 6; i++) {
		aBuffer.WriteLong(mBestLevels[i]);
		aBuffer.WriteLong(mBestScores[i]);
	}
	aBuffer.WriteLong(mTotalPoints);
	aBuffer.WriteLong(mGemsCleared);
	WriteFlags(aBuffer, mGemTypesUnlocked);
	aBuffer.WriteBytes(mSavedBits, 0x40);
	aBuffer.WriteLong(mGamesPlayed);
	aBuffer.WriteLong(mStartingRank);
	aBuffer.WriteByte(mShowReminder);
	WriteFlags(aBuffer, mTipsSeen);
	for (int i = 0; i < 10; i++)
		aBuffer.WriteLong(mSavedLongs[i]);
	aBuffer.WriteLong(mLastModePlayed);
	aBuffer.WriteByte(mBeforeFirstMove);
	WriteFlags(aBuffer, mModesUnlocked);
	aBuffer.WriteLong(mRank);
	aBuffer.WriteShort((short)mSavedBools.size());
	for (unsigned int i = 0; i < mSavedBools.size(); i++)
		aBuffer.WriteBoolean(mSavedBools[i]);
	aBuffer.WriteLong(mRankPoints);
	aBuffer.WriteLong(mReplayCount);
	aBuffer.WriteByte(mCustomCursors);
	int aSlots = (int)mChallengeStages.size();
	aBuffer.WriteShort((short)aSlots);
	for (int i = 0; i < aSlots; i++)
		aBuffer.WriteShort((short)mChallengeStages[i]);
	for (int i = 0; i < aSlots; i++)
		aBuffer.WriteLong(i < (int)mChallengeBests.size() ? mChallengeBests[i] : 0);
	aBuffer.WriteLong(mModesSeenBits);
	for (int i = 0; i < 6; i++)
		aBuffer.WriteLong(mGamesByMode[i]);
	for (const SavedGame& aGame : mSavedGames)
		aGame.Write(aBuffer);
	return true;
}

bool Profile::Select(const SexyString& theName)
{
	Save();
	bool aLoaded = Load(theName);
	if (!aLoaded)
		RemoveName(theName);
	return aLoaded;
}

bool Profile::SelectAny()
{
	for (int i = 0; i < (int)mNames.size(); i++) {
		if (Select(mNames[i]))
			return true;
	}
	return false;
}

bool Profile::Create(const SexyString& theName)
{
	if (!AddName(theName)) {
		SexyString aTaken = theName;
		for (int i = 0; i < (int)mNames.size(); i++) {
			if (CompareNoCase(mNames[i], aTaken) == 0) {
				aTaken = mNames[i];
				break;
			}
		}
		gApp->DoDialog(0x21, true, GT(0x7e9d14), GT(0x7e9d28) + aTaken, _S(""), 1);
		return false;
	}
	Deltree(GetUserFolder(theName));
	Save();
	mChallengeBests.clear();
	mChallengeStages.clear();
	// 0x734a14(0): slot 0 is there and set
	while ((int)mChallengeStages.size() <= 0)
		mChallengeStages.push_back(0);
	if (mChallengeStages[0] == 0)
		mChallengeStages[0] = 1;
	// With the app's list at +0xf5c (comes with the challenges) a zero for each of its entries goes on both vectors
	mReplayCount = 0;
	mIsNew = true;
	mName = theName;
	ClearMap();
	mHighestScore = 0;
	mTotalLevelsDelta = 0;
	mSavedBools.clear();
	mTotalPowerGems = 0;
	mBestChain = 0;
	mTotalFlames = 0;
	mTotalElectros = 0;
	mCustomCursors = gApp->mCustomCursorsEnabled;
	for (int i = 0; i < 6; i++) {
		mBestLevels[i] = 0;
		mBestScores[i] = 0;
		mGamesByMode[i] = 0;
	}
	mTotalPoints = 0;
	mGemsCleared = 0;
	mRankPoints = 0;
	mPuzzlesSolved = 0;
	mAchievementBits = 0;
	mFirstGameTime = 0;
	mGemTypesUnlocked = 0;
	mGamesPlayed = 0;
	memset(mSavedBits, 0, sizeof(mSavedBits));
	mStartingRank = 2;
	mShowReminder = true;
	mBeforeFirstMove = true;
	mTipsSeen = 0;
	for (int i = 0; i < 10; i++)
		mSavedLongs[i] = 0;
	mRank = -1;
	mLastModePlayed = 0;
	mModesUnlocked = 0;
	mModesSeenBits = 0;
	for (SavedGame& aGame : mSavedGames) {
		aGame.mReplayStart = -1;
		aGame.mReplayEnd = -1;
		aGame.mBest = 0;
		aGame.mData.Clear();
	}
	mModesUnlocked |= 1u << 1;
	mModesUnlocked |= 1u << 4;
	return true;
}

bool Profile::AddName(const SexyString& theName)
{
	for (int i = 0; i < (int)mNames.size(); i++) {
		if (CompareNoCase(mNames[i], theName) == 0)
			return false;
	}
	mNames.push_back(theName);
	return true;
}

bool Profile::RemoveName(const SexyString& theName)
{
	bool aRemoved = false;
	for (int i = 0; i < (int)mNames.size(); i++) {
		if (CompareNoCase(mNames[i], theName) == 0) {
			// The original erases while it walks on, so it skips the name after each one it erases
			mNames.erase(mNames.begin() + i);
			aRemoved = true;
		}
	}
	return aRemoved;
}

void Profile::Delete(const SexyString& theName)
{
	if (CompareNoCase(theName, mName) == 0)
		mName = _S("");
	if (!gApp->mPlayingDemoBuffer) {
		Deltree(GetUserFolder(theName));
		RemoveName(theName);
	}
}

void Profile::ClearMap()
{
}

void Profile::SetGemTypeUnlocked(int theBit, bool on)
{
	unsigned int aBit = 1u << (theBit & 31);
	mGemTypesUnlocked = on ? (mGemTypesUnlocked | aBit) : (mGemTypesUnlocked & ~aBit);
}

void Profile::SetTipSeen(int theBit, bool on)
{
	unsigned int aBit = 1u << (theBit & 31);
	mTipsSeen = on ? (mTipsSeen | aBit) : (mTipsSeen & ~aBit);
}

bool Profile::IsModeUnlocked(int theMode) const
{
	return (mModesUnlocked & (1u << (theMode & 31))) != 0;
}

SexyString Profile::GetName(int theIdx) const
{
	if (theIdx > -1 && theIdx < (int)mNames.size())
		return mNames[theIdx];
	return _S("");
}

void Profile::MarkTutorialDone()
{
	SetTipSeen(14, true);
	SetGemTypeUnlocked(4, true);
	SetGemTypeUnlocked(3, true);
	mRank = 0;
	Save();
}

// part of GetChallengeStage (0x73497f) and OpenChallengeCategory (0x734a14): the lists grow to reach a category
void Profile::GrowChallengeLists(int theCategory)
{
	while ((int)mChallengeStages.size() <= theCategory)
		mChallengeStages.push_back(0);
	while ((int)mChallengeBests.size() <= theCategory)
		mChallengeBests.push_back(0);
}

int Profile::GetChallengeStage(int theCategory)
{
	GrowChallengeLists(theCategory);
	return mChallengeStages[theCategory];
}

bool Profile::IsChallengeCategoryOpen(int theCategory)
{
	return GetChallengeStage(theCategory) > 0;
}

void Profile::SetChallengeDone(int theCategory, int theNumber)
{
	if (theCategory < (int)mChallengeStages.size()) {
		mChallengeStages[theCategory] = theNumber + 1;
		mLastChallengeCategory = theCategory;
	}
}

void Profile::OpenChallengeCategory(int theCategory)
{
	GrowChallengeLists(theCategory);
	if (mChallengeStages[theCategory] == 0)
		mChallengeStages[theCategory] = 1;
}

