#include "ResourceManager.h"
#include "Exception.h"
#include "Host.h"
#include <algorithm>
#include <cctype>

using namespace engine;

static uint16_t le16(const uint8_t* p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t le32(const uint8_t* p)
{
	return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

// The names are kept with '/' (the resource manager turns '\' into '/') and looked up in lower case (0x4404f0)
static std::string lower(const std::string& theName)
{
	std::string aName = theName;
	std::transform(aName.begin(), aName.end(), aName.begin(), [](char c) { return (char)tolower((unsigned char)c); });
	return aName;
}

static std::string slashes(const char* theName)
{
	std::string aName = theName;
	for (char& c : aName)
	{
		if (c == '\\')
			c = '/';
	}
	return aName;
}

// 0x43f520
ArchiveLoader::ArchiveLoader()
{
}

// 0x43fa00
void ArchiveLoader::openArchive(const std::string& thePath)
{
	std::vector<uint8_t> aData;
	if (!getHost()->readGameFile(thePath, aData))
		throw Exception("ArchiveLoader::openArchive: unable to open file '" + thePath + "' \n");
	mData = std::make_shared<const std::vector<uint8_t>>(std::move(aData));
	const std::vector<uint8_t>& d = *mData;
	if (d.size() < 12 || le32(&d[0]) != 0x53414646)
		throw Exception("ArchiveLoader::openReader: invalid magic number\n");
	if (!(le16(&d[4]) == 1 && le16(&d[6]) == 0))
		throw Exception("ArchiveLoader::openReader: invalid version number");
	readToc();
}

// 0x43f710, 0x440090 (the table's header) and 0x441120 (its entries)
void ArchiveLoader::readToc()
{
	const std::vector<uint8_t>& d = *mData;
	size_t aPos = le32(&d[8]);
	if (aPos + 24 > d.size())
		throw Exception("ArchiveLoader::readToc - unable to read TOC\n");
	if (le32(&d[aPos]) != 1)
		throw Exception("ArchiveToc::readTocHeader - bad version number\n");
	uint32_t aCount = le32(&d[aPos + 20]);
	aPos += 24;
	if (aCount == 0)
		throw Exception("No entries in TOC");
	for (uint32_t i = 0; i < aCount; i++)
	{
		if (aPos + 26 > d.size())
			throw Exception("Archive TOC - validation failed\n");
		Entry anEntry;
		anEntry.mOffset = le32(&d[aPos]);
		anEntry.mSize = le32(&d[aPos + 4]);
		size_t aNameLen = le16(&d[aPos + 24]);
		aPos += 26;
		if (aPos + aNameLen > d.size() || (size_t)anEntry.mOffset + anEntry.mSize > d.size())
			throw Exception("Archive TOC - validation failed\n");
		std::string aName((const char*)&d[aPos], aNameLen);
		aName = aName.c_str();  // the length counts the terminating zero
		aPos += aNameLen;
		mEntries[lower(aName)] = anEntry;
	}
}

// 0x43f690
bool ArchiveLoader::contains(const std::string& theName) const
{
	return mEntries.find(lower(theName)) != mEntries.end();
}

std::vector<std::string> ArchiveLoader::getNames() const
{
	std::vector<std::string> aNames;
	for (std::map<std::string, Entry>::const_iterator anIt = mEntries.begin(); anIt != mEntries.end(); ++anIt)
		aNames.push_back(anIt->first);
	return aNames;
}

// 0x43f900
InputStream* ArchiveLoader::getInputStream(const std::string& theName)
{
	std::map<std::string, Entry>::const_iterator anIt = mEntries.find(lower(theName));
	if (anIt == mEntries.end())
		throw Exception("ArchiveLoader::getInputStream - no stream found\n");
	return new MemoryStream(mData, anIt->second.mOffset, anIt->second.mSize);
}

// 0x43eae0
ResourceManager::ResourceManager()
	: mUseFiles(false)
{
}

// 0x43ebb0
void ResourceManager::addArchive(const char* thePath)
{
	Ref<ArchiveLoader> anArchive = new ArchiveLoader();
	anArchive->openArchive(thePath);
	mArchives.push_back(anArchive);
}

// 0x43e4f0
bool ResourceManager::exists(const char* theName)
{
	std::string aName = slashes(theName);
	if (!mUseFiles)
	{
		for (size_t i = 0; i < mArchives.size(); i++)
		{
			if (mArchives[i]->contains(aName))
				return true;
		}
		return false;
	}
	return fileExists(theName);
}

// 0x43e400: _findfirst() of the name
bool ResourceManager::fileExists(const char* theName)
{
	return getHost()->gameFileExists(slashes(theName));
}

// 0x43e860
InputStream* ResourceManager::openRawStream(const char* theName)
{
	if (mUseFiles)
		return openFile(theName);
	return getArchiveStream(theName);
}

// 0x43e880
InputStream* ResourceManager::openStream(const char* theName)
{
	return openRawStream(theName);
}

// 0x43e320 (the file stream's constructor 0x43f1f0 throws)
InputStream* ResourceManager::openFile(const char* theName)
{
	std::vector<uint8_t> aData;
	if (!getHost()->readGameFile(slashes(theName), aData))
		throw Exception(std::string("Unable to open file: ") + theName);
	return new MemoryStream(std::move(aData));
}

// 0x43e390
InputStream* ResourceManager::openBufferedFile(const char* theName)
{
	return openFile(theName);
}

// 0x43e640
InputStream* ResourceManager::getArchiveStream(const char* theName)
{
	std::string aName = slashes(theName);
	for (size_t i = 0; i < mArchives.size(); i++)
	{
		if (mArchives[i]->contains(aName))
			return mArchives[i]->getInputStream(aName);
	}
	throw Exception(std::string("ResourceManager::getArchiveStream - unable to find file in archive: ") + theName);
}
