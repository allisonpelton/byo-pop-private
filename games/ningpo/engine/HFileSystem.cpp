// The archive file system (see HFileSystem.h).

#include "HFileSystem.h"
#include "Pg.h"

#include <cctype>
#include <cstring>
#include <stb_image.h>

namespace pg
{

static uint16_t Le16(const uint8_t* p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t Le32(const uint8_t* p)
{
	return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

static std::string Lower(const char* theName)
{
	std::string aName = theName;
	for (char& c : aName)
		c = (char)tolower((unsigned char)c);
	return aName;
}

bool HFileSystem::mount(const char* theArchiveName)
{
	mEntries.clear();
	if (!GetHost()->ReadGameFile(theArchiveName, mArchive))
		return false;
	// the end of central directory record: the archive has no comment, but look back over one anyway
	const uint8_t* aData = mArchive.data();
	size_t aSize = mArchive.size();
	if (aSize < 22)
		return false;
	size_t anEnd = aSize - 22;
	size_t aLimit = (anEnd > 0xffff) ? anEnd - 0xffff : 0;
	while (Le32(aData + anEnd) != 0x06054b50)
	{
		if (anEnd == aLimit)
			return false;
		anEnd--;
	}
	int aCount = Le16(aData + anEnd + 10);
	uint32_t aDirOffset = Le32(aData + anEnd + 16);
	size_t aPos = aDirOffset;
	for (int i = 0; i < aCount; i++)
	{
		if (aPos + 46 > aSize || Le32(aData + aPos) != 0x02014b50)
			return false;
		Entry anEntry;
		anEntry.mMethod = Le16(aData + aPos + 10);
		anEntry.mCompressedSize = Le32(aData + aPos + 20);
		anEntry.mSize = Le32(aData + aPos + 24);
		int aNameLen = Le16(aData + aPos + 28);
		int anExtraLen = Le16(aData + aPos + 30);
		int aCommentLen = Le16(aData + aPos + 32);
		anEntry.mOffset = Le32(aData + aPos + 42);
		if (aPos + 46 + aNameLen > aSize)
			return false;
		anEntry.mName = Lower(std::string((const char*)aData + aPos + 46, aNameLen).c_str());
		mEntries.push_back(anEntry);
		aPos += 46 + aNameLen + anExtraLen + aCommentLen;
	}
	return true;
}

const HFileSystem::Entry* HFileSystem::find(const char* theName) const
{
	std::string aName = Lower(theName);
	for (const Entry& anEntry : mEntries)
		if (anEntry.mName == aName)
			return &anEntry;
	return nullptr;
}

bool HFileSystem::exists(const char* theName) const
{
	return find(theName) != nullptr;
}

bool HFileSystem::readFile(const char* theName, std::vector<uint8_t>& theData) const
{
	const Entry* anEntry = find(theName);
	if (anEntry == nullptr)
		return false;
	const uint8_t* aData = mArchive.data();
	size_t aPos = anEntry->mOffset;
	if (aPos + 30 > mArchive.size() || Le32(aData + aPos) != 0x04034b50)
		return false;
	aPos += 30 + Le16(aData + aPos + 26) + Le16(aData + aPos + 28);
	if (aPos + anEntry->mCompressedSize > mArchive.size())
		return false;
	theData.resize(anEntry->mSize);
	if (anEntry->mMethod == 0)
	{
		if (anEntry->mSize != 0)
			memcpy(theData.data(), aData + aPos, anEntry->mSize);
		return true;
	}
	if (anEntry->mMethod != 8)
		return false;
	int aLen = stbi_zlib_decode_noheader_buffer((char*)theData.data(), (int)anEntry->mSize,
		(const char*)aData + aPos, (int)anEntry->mCompressedSize);
	return aLen == (int)anEntry->mSize;
}

} // namespace pg
