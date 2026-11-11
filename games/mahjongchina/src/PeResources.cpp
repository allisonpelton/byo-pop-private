// port: the reader for the resources of the game's exe and Resource.dll (PeResources.h); the original asked Windows.

#include "PeResources.h"

#include <cctype>
#include <cstdio>
#include <cstring>

using namespace mahjongchina;

namespace
{

uint16_t U16(const std::vector<uint8_t>& theData, size_t theOffset) { return theOffset + 2 <= theData.size() ? uint16_t(theData[theOffset] | theData[theOffset + 1] << 8) : 0; }
uint32_t U32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	return theOffset + 4 <= theData.size() ? uint32_t(theData[theOffset] | theData[theOffset + 1] << 8 | theData[theOffset + 2] << 16 | uint32_t(theData[theOffset + 3]) << 24) : 0;
}

// UTF-16 to Windows-1252: the game's texts are English, so anything outside the code page becomes '?'
void AppendCp1252(std::string& theOut, uint16_t theChar)
{
	static const uint16_t CP1252_HIGH[32] = {0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030,
		0x0160, 0x2039, 0x0152, 0, 0x017d, 0, 0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc,
		0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178};
	if (theChar < 0x80 || (theChar >= 0xa0 && theChar < 0x100))
	{
		theOut += char(theChar);
		return;
	}
	for (int i = 0; i < 32; i++)
		if (CP1252_HIGH[i] == theChar)
		{
			theOut += char(0x80 + i);
			return;
		}
	theOut += '?';
}

} // namespace

PeResources::Key::Key(const char* theName) : mIsName(true), mId(0)
{
	for (const char* aChar = theName; *aChar; aChar++)
		mName += char(toupper((unsigned char)*aChar));
}

bool PeResources::Key::operator<(const Key& theOther) const
{
	if (mIsName != theOther.mIsName)
		return mIsName;  // names come before ids, as in the directory
	return mIsName ? mName < theOther.mName : mId < theOther.mId;
}

uint32_t PeResources::RvaToOffset(uint32_t theRva) const
{
	uint32_t aPeHeader = U32(mData, 0x3c);
	int aSectionCount = U16(mData, aPeHeader + 6);
	uint32_t aSection = aPeHeader + 24 + U16(mData, aPeHeader + 20);
	for (int i = 0; i < aSectionCount; i++, aSection += 40)
	{
		uint32_t aVirtualSize = U32(mData, aSection + 8), aVirtualAddress = U32(mData, aSection + 12), aRawSize = U32(mData, aSection + 16), aRawOffset = U32(mData, aSection + 20);
		uint32_t aSpan = aVirtualSize > aRawSize ? aVirtualSize : aRawSize;
		if (theRva >= aVirtualAddress && theRva < aVirtualAddress + aSpan)
			return aRawOffset + theRva - aVirtualAddress;
	}
	return 0;
}

bool PeResources::Open(const std::string& thePath)
{
	mData.clear();
	mDir.clear();
	mOrder.clear();
	FILE* aFile = fopen(thePath.c_str(), "rb");
	if (!aFile)
		return false;
	fseek(aFile, 0, SEEK_END);
	long aLength = ftell(aFile);
	fseek(aFile, 0, SEEK_SET);
	mData.resize(aLength > 0 ? size_t(aLength) : 0);
	if (aLength <= 0 || fread(mData.data(), 1, mData.size(), aFile) != mData.size())
		mData.clear();
	fclose(aFile);
	if (mData.size() < 0x40 || mData[0] != 'M' || mData[1] != 'Z')
	{
		mData.clear();
		return false;
	}
	uint32_t aPeHeader = U32(mData, 0x3c);
	if (U32(mData, aPeHeader) != 0x4550)
	{
		mData.clear();
		return false;
	}
	uint32_t anOptionalHeader = aPeHeader + 24;
	uint32_t aResourceRva = U32(mData, anOptionalHeader + 96 + 8 * 2);
	uint32_t aBase = aResourceRva ? RvaToOffset(aResourceRva) : 0;
	if (!aBase)
		return true;

	auto aKeyAt = [&](uint32_t theName) -> Key {
		if (!(theName & 0x80000000))
			return Key(theName);
		uint32_t anOffset = aBase + (theName & 0x7fffffff);
		int aLength = U16(mData, anOffset);
		std::string aName;
		for (int i = 0; i < aLength; i++)
			AppendCp1252(aName, U16(mData, anOffset + 2 + 2 * i));
		return Key(aName.c_str());
	};
	auto aForEach = [&](uint32_t theDir, auto&& theFn) {
		int aCount = U16(mData, aBase + theDir + 12) + U16(mData, aBase + theDir + 14);
		for (int i = 0; i < aCount; i++)
			theFn(U32(mData, aBase + theDir + 16 + 8 * i), U32(mData, aBase + theDir + 20 + 8 * i));
	};
	aForEach(0, [&](uint32_t theType, uint32_t theTypeDir) {
		if (!(theTypeDir & 0x80000000))
			return;
		Key aType = aKeyAt(theType);
		mOrder.push_back({aType, {}});
		aForEach(theTypeDir & 0x7fffffff, [&](uint32_t theName, uint32_t theNameDir) {
			if (!(theNameDir & 0x80000000))
				return;
			Key aName = aKeyAt(theName);
			bool aFirst = true;
			aForEach(theNameDir & 0x7fffffff, [&](uint32_t, uint32_t theLeaf) {
				if (!aFirst || (theLeaf & 0x80000000))
					return;
				aFirst = false;
				uint32_t aRva = U32(mData, aBase + theLeaf), aSize = U32(mData, aBase + theLeaf + 4);
				uint32_t anOffset = RvaToOffset(aRva);
				if (anOffset && anOffset + aSize <= mData.size())
				{
					mDir[aType][aName] = {anOffset, aSize};
					mOrder.back().second.push_back(aName);
				}
			});
		});
	});
	return true;
}

const uint8_t* PeResources::Find(const Key& theType, const Key& theName, uint32_t* theSize) const
{
	auto aType = mDir.find(theType);
	if (aType == mDir.end())
		return nullptr;
	auto anEntry = aType->second.find(theName);
	if (anEntry == aType->second.end())
		return nullptr;
	if (theSize)
		*theSize = anEntry->second.mSize;
	return mData.data() + anEntry->second.mOffset;
}

bool PeResources::Load(const Key& theType, const Key& theName, std::vector<uint8_t>& theOut) const
{
	uint32_t aSize = 0;
	const uint8_t* aData = Find(theType, theName, &aSize);
	if (!aData)
		return false;
	theOut.assign(aData, aData + aSize);
	return true;
}

bool PeResources::LoadString(uint32_t theId, std::string& theOut) const
{
	theOut.clear();
	uint32_t aSize = 0;
	const uint8_t* aBlock = Find(Key(6u), Key((theId >> 4) + 1), &aSize);  // RT_STRING: blocks of 16
	if (!aBlock)
		return false;
	uint32_t anOffset = 0;
	for (uint32_t i = 0; i < (theId & 15); i++)
	{
		if (anOffset + 2 > aSize)
			return false;
		anOffset += 2 + 2 * uint32_t(aBlock[anOffset] | aBlock[anOffset + 1] << 8);
	}
	if (anOffset + 2 > aSize)
		return false;
	uint32_t aLength = aBlock[anOffset] | aBlock[anOffset + 1] << 8;
	for (uint32_t i = 0; i < aLength && anOffset + 4 + 2 * i <= aSize; i++)
		AppendCp1252(theOut, uint16_t(aBlock[anOffset + 2 + 2 * i] | aBlock[anOffset + 3 + 2 * i] << 8));
	return aLength != 0;
}

std::vector<PeResources::Key> PeResources::Names(const Key& theType) const
{
	for (auto& aType : mOrder)
		if (!(aType.first < theType) && !(theType < aType.first))
			return aType.second;
	return {};
}
