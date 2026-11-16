#include "Archive.h"
#include "Engine.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstring>

namespace engine
{

namespace
{

// 0x499d30 (feedingfrenzy2) and 0x499e00 (feedingfrenzy2): little-endian values read from the stream
uint32_t readU32(InputStream* theStream)
{
	uint8_t aBytes[4] = {};
	theStream->read(aBytes, 4);
	return aBytes[0] | (aBytes[1] << 8) | (aBytes[2] << 16) | ((uint32_t)aBytes[3] << 24);
}

uint16_t readU16(InputStream* theStream)
{
	uint8_t aBytes[2] = {};
	theStream->read(aBytes, 2);
	return (uint16_t)(aBytes[0] | (aBytes[1] << 8));
}

std::string toLower(const std::string& theName)
{
	std::string aName = theName;
	std::transform(aName.begin(), aName.end(), aName.begin(), [](char c) { return (char)tolower((unsigned char)c); });
	return aName;
}

} // namespace

// 0x450800 (feedingfrenzy2), with 0x499e30 (feedingfrenzy2) for the version and digest
bool ArchiveToc::read(RandomAccessInputStream* theStream)
{
	uint16_t aMajor = readU16(theStream);
	uint16_t aMinor = readU16(theStream);
	if (aMajor != 1 || aMinor != 0)
	{
		getEngine()->log("Archive TOC has wrong version number\n");
		return false;
	}
	uint8_t aDigest[16];
	theStream->read(aDigest, 16);

	uint32_t aCount = readU32(theStream);
	if (aCount < 1)
	{
		getEngine()->log("Archive TOC contains no entries.");
		return false;
	}
	for (uint32_t i = 0; i < aCount; ++i)
	{
		ArchiveEntry anEntry;
		anEntry.mOffset = readU32(theStream);
		anEntry.mSize = readU32(theStream);
		theStream->read(anEntry.mDigest, 16);
		uint16_t aLength = readU16(theStream);
		// The original reads the name into a buffer of its own (0x5ae3f0 (feedingfrenzy2)) and takes it up to its zero
		std::string aName(aLength, '\0');
		theStream->read(&aName[0], aLength);
		aName.resize(strlen(aName.c_str()));
		mEntries[aName] = anEntry;
	}
	return true;
}

const ArchiveEntry* ArchiveToc::find(const std::string& theName) const
{
	std::map<std::string, ArchiveEntry>::const_iterator it = mEntries.find(toLower(theName));
	return it == mEntries.end() ? nullptr : &it->second;
}

bool ArchiveLoader::open(const std::string& thePath)
{
	mStream = getEngine()->openDiskFile(thePath);
	if (!mStream)
		return false;
	uint32_t aMagic = readU32(mStream);
	uint16_t aMajor = readU16(mStream);
	uint16_t aMinor = readU16(mStream);
	if (aMagic != 0x53414646)  // "FFAS"
	{
		getEngine()->log("Archive file has invalid magic number.  %s\n", thePath.c_str());
		mStream = nullptr;
		return false;
	}
	if (aMajor != 1 || aMinor != 0)
	{
		getEngine()->log("Archive file has invalid version number. %s\n", thePath.c_str());
		mStream = nullptr;
		return false;
	}
	mTocOffset = readU32(mStream);
	return readToc();
}

bool ArchiveLoader::readToc()
{
	if (!mStream)
	{
		getEngine()->log("Archive file not open to read TOC.\n");
		return false;
	}
	if (!mStream->seek((int)mTocOffset, RandomAccessInputStream::ORIGIN_SET))
	{
		getEngine()->log("Unable to access Archive TOC.\n");
		return false;
	}
	mToc = new ArchiveToc();
	if (!mToc->read(mStream))
	{
		getEngine()->log("Unable to read Archive TOC.\n");
		mToc = nullptr;
		return false;
	}
	return true;
}

bool ArchiveLoader::contains(const std::string& theName) const
{
	return mToc && mToc->find(theName) != nullptr;
}

InputStream* ArchiveLoader::getInputStream(const std::string& theName, bool theCheckDigest)
{
	const ArchiveEntry* anEntry = mToc->find(theName);
	if (!anEntry)
	{
		getEngine()->log("%s not found in Archive.\n", theName.c_str());
		return nullptr;
	}
	(void)theCheckDigest;  // port: see Archive.h
	return new ArchiveInputStream(this, *anEntry);
}

int ArchiveInputStream::read(void* theBuffer, int theSize)
{
	if (!mArchive || mPosition >= mEnd)
		return 0;
	if ((uint32_t)theSize + mPosition > mEnd)
		theSize = (int)(mEnd - mPosition);
	RandomAccessInputStream* aSource = mArchive->getStream();
	if ((uint32_t)aSource->tell() != mPosition)
		aSource->seek((int)mPosition, ORIGIN_SET);
	int aRead = aSource->read(theBuffer, theSize);
	mPosition += aRead;
	return aRead;
}

bool ArchiveInputStream::close()
{
	mArchive = nullptr;
	return true;
}

bool ArchiveInputStream::seek(int theOffset, int theOrigin)
{
	uint32_t aBase;
	switch (theOrigin)
	{
	case ORIGIN_SET:
		aBase = mStart;
		break;
	case ORIGIN_CURRENT:
		aBase = mPosition;
		break;
	case ORIGIN_END:
		aBase = mEnd;
		break;
	default:
		return false;
	}
	uint32_t aPosition = aBase + theOffset;
	if (aPosition < mStart)
		return false;
	mPosition = aPosition;
	return true;
}

} // namespace engine
