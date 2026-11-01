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
	mUnknown20 = theBuffer.ReadShort();
	mUnknown24 = theBuffer.ReadShort();
	mUnknown28 = theBuffer.ReadLong();
	ByteVector aData;
	theBuffer.ReadBuffer(&aData);
	mData.SetData(aData);
}

void Profile::SavedGame::Write(Buffer& theBuffer) const
{
	theBuffer.WriteShort((short)mUnknown20);
	theBuffer.WriteShort((short)mUnknown24);
	theBuffer.WriteLong(mUnknown28);
	theBuffer.WriteBuffer(mData.mData);
}

Profile::Profile()
{
	for (SavedGame& aGame : mSavedGames) {
		aGame.mUnknown20 = 0;
		aGame.mUnknown24 = 0;
		aGame.mUnknown28 = 0;
	}
}

Profile::~Profile() = default;

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

	mUnknown214 = -1;
	mName = theName;
	ClearMap();
	mIsNew = false;
	mUnknown74 = aBuffer.ReadLong();
	mUnknown78 = aBuffer.ReadLong();
	mUnknown8C = aBuffer.ReadLong();
	if (aVersion < 0x34)
		aBuffer.ReadLong();
	mUnknown90 = aBuffer.ReadLong();
	if (aVersion < 0x34) {
		aBuffer.ReadLong();
		aBuffer.ReadLong();
	}
	mUnknown94 = aBuffer.ReadLong();
	mUnknown98 = aBuffer.ReadLong();
	mUnknown164 = aBuffer.ReadLong();
	mUnknown124 = aBuffer.ReadLong();
	mUnknown158 = aBuffer.ReadLong();
	mUnknown58 = aBuffer.ReadLong();
	for (int i = 0; i < 6; i++) {
		mUnknown128[i] = aBuffer.ReadLong();
		mUnknown5C[i] = aBuffer.ReadLong();
	}
	mUnknown158 = aBuffer.ReadLong();
	mUnknown15C = aBuffer.ReadLong();
	ReadFlags(aBuffer, aVersion, &mFlags1E0);
	aBuffer.ReadBytes(mBits170, 0x40);
	mUnknown168 = aBuffer.ReadLong();
	mUnknown16C = aBuffer.ReadLong();
	mUnknown1B0 = aBuffer.ReadByte() != 0;
	ReadFlags(aBuffer, aVersion, &mFlags1E4);
	for (int i = 0; i < 10; i++)
		mUnknown1B4[i] = aBuffer.ReadLong();
	mUnknown1DC = aBuffer.ReadLong();
	mUnknown1B1 = aBuffer.ReadByte() != 0;
	ReadFlags(aBuffer, aVersion, &mFlags1E8);
	mRank = aBuffer.ReadLong();
	mBools80.clear();
	int aCount = aBuffer.ReadShort();
	for (int i = aCount; i > 0; i--)
		mBools80.push_back(aBuffer.ReadBoolean());
	mUnknown160 = aBuffer.ReadLong();
	mUnknown1F0 = aBuffer.ReadLong();
	mCustomCursors = aBuffer.ReadByte() != 0;
	gApp->EnableCustomCursors(mCustomCursors);
	int aSlots = aBuffer.ReadShort();
	mInts1F4.clear();
	for (int i = aSlots; i > 0; i--)
		mInts1F4.push_back(aBuffer.ReadShort());
	if (aVersion < 0x2c)
		aBuffer.ReadByte();
	mInts204.clear();
	for (int i = aSlots; i > 0; i--)
		mInts204.push_back(aBuffer.ReadLong());
	mUnknown218 = aVersion < 0x2a ? 0 : aBuffer.ReadLong();
	for (int i = 0; i < 6; i++)
		mUnknown140[i] = aVersion < 0x2b ? 0 : aBuffer.ReadLong();
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
	if (mName.empty() || mName == GT(0x7da070))
		return;
	Buffer aBuffer;
	std::string aPath = GetUserFolder(mName) + "\\profile.dat";
	MkDir(GetFileDir(aPath, false));
	aBuffer.WriteLong(kMagic);
	aBuffer.WriteLong(kProfileVersion);
	aBuffer.WriteLong(mUnknown74);
	aBuffer.WriteLong(mUnknown78);
	aBuffer.WriteLong(mUnknown8C);
	aBuffer.WriteLong(mUnknown90);
	aBuffer.WriteLong(mUnknown94);
	aBuffer.WriteLong(mUnknown98);
	aBuffer.WriteLong(mUnknown164);
	aBuffer.WriteLong(mUnknown124);
	aBuffer.WriteLong(mUnknown158);
	aBuffer.WriteLong(mUnknown58);
	for (int i = 0; i < 6; i++) {
		aBuffer.WriteLong(mUnknown128[i]);
		aBuffer.WriteLong(mUnknown5C[i]);
	}
	aBuffer.WriteLong(mUnknown158);
	aBuffer.WriteLong(mUnknown15C);
	WriteFlags(aBuffer, mFlags1E0);
	aBuffer.WriteBytes(mBits170, 0x40);
	aBuffer.WriteLong(mUnknown168);
	aBuffer.WriteLong(mUnknown16C);
	aBuffer.WriteByte(mUnknown1B0);
	WriteFlags(aBuffer, mFlags1E4);
	for (int i = 0; i < 10; i++)
		aBuffer.WriteLong(mUnknown1B4[i]);
	aBuffer.WriteLong(mUnknown1DC);
	aBuffer.WriteByte(mUnknown1B1);
	WriteFlags(aBuffer, mFlags1E8);
	aBuffer.WriteLong(mRank);
	aBuffer.WriteShort((short)mBools80.size());
	for (unsigned int i = 0; i < mBools80.size(); i++)
		aBuffer.WriteBoolean(mBools80[i]);
	aBuffer.WriteLong(mUnknown160);
	aBuffer.WriteLong(mUnknown1F0);
	aBuffer.WriteByte(mCustomCursors);
	int aSlots = (int)mInts1F4.size();
	aBuffer.WriteShort((short)aSlots);
	for (int i = 0; i < aSlots; i++)
		aBuffer.WriteShort((short)mInts1F4[i]);
	for (int i = 0; i < aSlots; i++)
		aBuffer.WriteLong(i < (int)mInts204.size() ? mInts204[i] : 0);
	aBuffer.WriteLong(mUnknown218);
	for (int i = 0; i < 6; i++)
		aBuffer.WriteLong(mUnknown140[i]);
	for (const SavedGame& aGame : mSavedGames)
		aGame.Write(aBuffer);
	gApp->WriteBufferToFile(aPath, &aBuffer);
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
	mInts204.clear();
	mInts1F4.clear();
	// 0x734a14(0): slot 0 is there and set
	while ((int)mInts1F4.size() <= 0)
		mInts1F4.push_back(0);
	if (mInts1F4[0] == 0)
		mInts1F4[0] = 1;
	// With the app's list at +0xf5c (comes with the challenges) a zero for each of its entries goes on both vectors
	mUnknown1F0 = 0;
	mIsNew = true;
	mName = theName;
	ClearMap();
	mUnknown74 = 0;
	mUnknown78 = 0;
	mBools80.clear();
	mUnknown8C = 0;
	mUnknown90 = 0;
	mUnknown94 = 0;
	mUnknown98 = 0;
	mCustomCursors = gApp->mCustomCursorsEnabled;
	for (int i = 0; i < 6; i++) {
		mUnknown128[i] = 0;
		mUnknown5C[i] = 0;
		mUnknown140[i] = 0;
	}
	mUnknown158 = 0;
	mUnknown15C = 0;
	mUnknown160 = 0;
	mUnknown124 = 0;
	mUnknown164 = 0;
	mUnknown58 = 0;
	mFlags1E0 = 0;
	mUnknown168 = 0;
	memset(mBits170, 0, sizeof(mBits170));
	mUnknown16C = 2;
	mUnknown1B0 = true;
	mUnknown1B1 = true;
	mFlags1E4 = 0;
	for (int i = 0; i < 10; i++)
		mUnknown1B4[i] = 0;
	mRank = -1;
	mUnknown1DC = 0;
	mFlags1E8 = 0;
	mUnknown218 = 0;
	for (SavedGame& aGame : mSavedGames) {
		aGame.mUnknown20 = -1;
		aGame.mUnknown24 = -1;
		aGame.mUnknown28 = 0;
		aGame.mData.Clear();
	}
	mFlags1E8 |= 1u << 1;
	mFlags1E8 |= 1u << 4;
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
	mMap2C.clear();
}

void Profile::SetFlag1E0(int theBit, bool on)
{
	unsigned int aBit = 1u << (theBit & 31);
	mFlags1E0 = on ? (mFlags1E0 | aBit) : (mFlags1E0 & ~aBit);
}

void Profile::SetFlag1E4(int theBit, bool on)
{
	unsigned int aBit = 1u << (theBit & 31);
	mFlags1E4 = on ? (mFlags1E4 | aBit) : (mFlags1E4 & ~aBit);
}

bool Profile::GetBit170(int theBit) const
{
	return (mBits170[theBit / 8] & (0x80 >> (theBit & 7))) != 0;
}

void Profile::SetBit170(int theBit)
{
	mBits170[theBit / 8] |= (unsigned char)(0x80 >> (theBit & 7));
}

SexyString Profile::GetName(int theIdx) const
{
	if (theIdx > -1 && theIdx < (int)mNames.size())
		return mNames[theIdx];
	return _S("");
}

void Profile::MarkTutorialDone()
{
	SetFlag1E4(14, true);
	SetFlag1E0(4, true);
	SetFlag1E0(3, true);
	mRank = 0;
	Save();
}
