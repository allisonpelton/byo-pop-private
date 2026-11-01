// The engine's files and its pak (see File.h).

#include "File.h"
#include "Host.h"
#include "Platform.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

namespace bonnies
{

Pak* gPak = nullptr;

namespace
{
// A file the C runtime had open: its bytes, the position, and where it goes when closed
struct OpenFile
{
	std::vector<uint8_t> mData;
	size_t mPos = 0;
	bool mWrite = false;
	Str mPath;
};

std::map<int, OpenFile> gFiles;
int gNextHandle = 3;  // _open's first free handle after stdin, stdout and stderr

// _open(thePath, _O_RDONLY | _O_BINARY)
int crtOpenRead(const Str& thePath)
{
	OpenFile aFile;
	if (host()->userFileExists(thePath))
	{
		if (!host()->readUserFile(thePath, aFile.mData))
			return -1;
	}
	else if (!host()->readGameFile(thePath, aFile.mData))
		return -1;
	int aHandle = gNextHandle++;
	gFiles[aHandle] = std::move(aFile);
	return aHandle;
}

// _open(thePath, _O_RDWR | _O_BINARY, plus _O_CREAT | _O_TRUNC unless theAppend)
int crtOpenWrite(const Str& thePath, bool theAppend)
{
	OpenFile aFile;
	aFile.mWrite = true;
	aFile.mPath = thePath;
	if (theAppend)
	{
		if (host()->userFileExists(thePath))
		{
			if (!host()->readUserFile(thePath, aFile.mData))
				return -1;
		}
		else if (!host()->readGameFile(thePath, aFile.mData))
			return -1;
	}
	int aHandle = gNextHandle++;
	gFiles[aHandle] = std::move(aFile);
	return aHandle;
}

int crtRead(int theHandle, void* theBuffer, unsigned int theCount)
{
	std::map<int, OpenFile>::iterator anIt = gFiles.find(theHandle);
	if (anIt == gFiles.end())
		return -1;
	OpenFile& aFile = anIt->second;
	size_t aLeft = aFile.mPos < aFile.mData.size() ? aFile.mData.size() - aFile.mPos : 0;
	size_t aCount = std::min((size_t)theCount, aLeft);
	if (aCount > 0)
		std::memcpy(theBuffer, aFile.mData.data() + aFile.mPos, aCount);
	aFile.mPos += aCount;
	return (int)aCount;
}

int crtWrite(int theHandle, const void* theBuffer, unsigned int theCount)
{
	std::map<int, OpenFile>::iterator anIt = gFiles.find(theHandle);
	if (anIt == gFiles.end())
		return -1;
	OpenFile& aFile = anIt->second;
	if (aFile.mPos + theCount > aFile.mData.size())
		aFile.mData.resize(aFile.mPos + theCount);
	if (theCount > 0)
		std::memcpy(aFile.mData.data() + aFile.mPos, theBuffer, theCount);
	aFile.mPos += theCount;
	return (int)theCount;
}

void crtSeek(int theHandle, long theOffset, int theOrigin)
{
	std::map<int, OpenFile>::iterator anIt = gFiles.find(theHandle);
	if (anIt == gFiles.end())
		return;
	OpenFile& aFile = anIt->second;
	if (theOrigin == 2)
		aFile.mPos = aFile.mData.size() + theOffset;
	else
		aFile.mPos = theOffset;
}

void crtClose(int theHandle)
{
	std::map<int, OpenFile>::iterator anIt = gFiles.find(theHandle);
	if (anIt == gFiles.end())
		return;
	if (anIt->second.mWrite)
		host()->writeUserFile(anIt->second.mPath, anIt->second.mData);
	gFiles.erase(anIt);
}
}

// 0x429a70
int fileRead(int theHandle, void* theBuffer, unsigned int theCount)
{
	if (theHandle == PAK_HANDLE && gPak != nullptr)
		return gPak->read(PAK_HANDLE, theBuffer, theCount);
	return crtRead(theHandle, theBuffer, theCount);
}

// 0x429ab0
void fileClose(int theHandle)
{
	if (theHandle != -1)
	{
		if (theHandle != PAK_HANDLE || gPak == nullptr)
		{
			crtClose(theHandle);
			return;
		}
		gPak->closeEntry();
	}
}

// 0x429ae0
Str fileReadText(int theHandle, unsigned int theMax)
{
	std::vector<char> aBuffer(theMax + 100);
	int aCount;
	if (theHandle == PAK_HANDLE && gPak != nullptr)
		aCount = gPak->read(PAK_HANDLE, aBuffer.data(), theMax);
	else
		aCount = crtRead(theHandle, aBuffer.data(), theMax);
	aBuffer[aCount < 0 ? 0 : aCount] = '\0';
	return Str(aBuffer.data());
}

// 0x429ba0 (_unlink)
bool fileRemove(const Str& thePath)
{
	return host()->removeUserFile(thePath);
}

// 0x429c00
Str baseName(const Str& thePath)
{
	int i = (int)thePath.size();
	int j;
	do
	{
		j = i;
		i = j - 1;
		if (i < 0)
			return thePath;
	} while ((int)thePath.size() <= i || (thePath[i] != '/' && thePath[i] != '\\'));
	return substr(thePath, j, (int)thePath.size());
}

// 0x429cb0, 0x42a320
void fileWriteInt(int theHandle, const void* theValue)
{
	crtWrite(theHandle, theValue, 4);
}

// 0x429cd0
void fileWriteByte(int theHandle, const void* theValue)
{
	crtWrite(theHandle, theValue, 1);
}

// 0x429cf0
void fileWriteString(int theHandle, const Str* theStr)
{
	Str aStr = "";
	if (theStr != nullptr && 0 < (int)theStr->size())
		aStr = *theStr;
	int aLen = (int)aStr.size();
	crtWrite(theHandle, &aLen, 4);
	if (0 < aLen)
		crtWrite(theHandle, aStr.data(), aLen);
}

// 0x429db0, 0x42a340
void fileReadInt(int theHandle, void* theValue)
{
	if (theHandle == PAK_HANDLE && gPak != nullptr)
	{
		gPak->read(PAK_HANDLE, theValue, 4);
		return;
	}
	crtRead(theHandle, theValue, 4);
}

// 0x429df0
void fileReadByte(int theHandle, void* theValue)
{
	if (theHandle == PAK_HANDLE && gPak != nullptr)
	{
		gPak->read(PAK_HANDLE, theValue, 1);
		return;
	}
	crtRead(theHandle, theValue, 1);
}

// 0x429e30. As the original: the string ends at a zero byte among the bytes read.
int fileReadString(int theHandle, Str& theStr)
{
	int aLen = 0;
	int aRead;
	if (theHandle == PAK_HANDLE && gPak != nullptr)
		aRead = gPak->read(PAK_HANDLE, &aLen, 4);
	else
		aRead = crtRead(theHandle, &aLen, 4);
	if (0 < aLen)
	{
		std::vector<char> aBuffer(aLen + 1, '\0');
		int aRead2;
		if (theHandle == PAK_HANDLE && gPak != nullptr)
			aRead2 = gPak->read(PAK_HANDLE, aBuffer.data(), aLen);
		else
			aRead2 = crtRead(theHandle, aBuffer.data(), aLen);
		aBuffer[aLen] = '\0';
		theStr = aBuffer.data();
		return aRead2 + aRead;
	}
	theStr = "";
	return aRead;
}

// 0x429ee0
int fileOpen(const Str& thePath, bool theWrite, bool theAppend)
{
	if (!theWrite && gPak != nullptr)
	{
		int aHandle = gPak->openEntry(baseName(thePath));
		if (aHandle != -1)
			return aHandle;
	}
	int aHandle = theWrite ? crtOpenWrite(thePath, theAppend) : crtOpenRead(thePath);
	if (theWrite && theAppend)
		crtSeek(aHandle, 0, 2);
	return aHandle;
}

// 0x429ff0 (_findfirst/_findnext over theDir/*.*, in the order of the folder's names)
void listFiles(const Str& theDir, const Str& theFilter, int* theCount, Str** theNames)
{
	std::vector<Str> aFound;
	std::vector<std::string> aNames = host()->listFiles(theDir);
	for (size_t n = 0; n < aNames.size(); n++)
	{
		if (containsNoCase(aNames[n], theFilter))
		{
			char aName[260];
			std::snprintf(aName, sizeof(aName), "%s", aNames[n].c_str());
			char* p = aName + std::strlen(aName) - 1;
			if (aName < p)
			{
				do
				{
					if (*p == '.')
					{
						*p = '\0';
						break;
					}
					p = p - 1;
				} while (aName < p);
			}
			if (aName[0] != '\0' && (int)aFound.size() < 999)
				aFound.push_back(aName);
		}
	}
	int aCount = (int)aFound.size();
	Str* aResult = new Str[aCount < 1 ? 1 : aCount];
	for (int i = 0; i < aCount; i++)
		aResult[i] = aFound[i];
	*theNames = aResult;
	*theCount = aCount;
}

// 0x42a390
bool fileExists(const Str& thePath)
{
	if (gPak != nullptr && gPak->contains(baseName(thePath)))
		return true;
	int aHandle = crtOpenRead(thePath);
	if (aHandle == -1)
		return false;
	crtClose(aHandle);
	return true;
}

// 0x42f090
Pak::Pak()
{
	mOpen = 0;
	mCount = 0;
	mHandle = -1;
	mEntries = nullptr;
	mInUse = 0;
}

// 0x42f0a0
Pak::~Pak()
{
	mOpen = 0;
}

// 0x42f0b0
void Pak::open()
{
	mInUse = 0;
	mCount = 0;
	Str aPath = dataDir(true) + "Bonnie.pak";
	mHandle = fileOpen(aPath, false, false);
	if (mHandle != -1)
	{
		int aVersion;
		unsigned char aHeader[52];
		fileReadInt(mHandle, &aVersion);
		int i = 0;
		do
		{
			fileReadByte(mHandle, aHeader + i);
			i = i + 1;
		} while (i < 0x32);
		fileReadInt(mHandle, &mCount);
		if (mCount == 0)
		{
			fileClose(mHandle);
			mHandle = -1;
		}
		else
		{
			mEntries = new Entry[mCount];
			i = 0;
			if (0 < mCount)
			{
				do
				{
					fileReadInt(mHandle, &mEntries[i].mHash);
					fileReadInt(mHandle, &mEntries[i].mOffset);
					fileReadInt(mHandle, &mEntries[i].mSize);
					i = i + 1;
				} while (i < mCount);
			}
			mOpen = 1;
		}
	}
}

// 0x42f230
void Pak::close()
{
	if (mOpen != 0)
	{
		if (mHandle != 0)
			fileClose(mHandle);
		delete[] mEntries;
		mEntries = nullptr;
		mOpen = 0;
	}
}

// 0x42f270
int Pak::find(const Str& theName)
{
	uint32_t aHash = hashName(theName);
	int i = 0;
	if (0 < mCount)
	{
		Entry* anEntry = mEntries;
		do
		{
			if (aHash == anEntry->mHash)
				return i;
			i = i + 1;
			anEntry = anEntry + 1;
		} while (i < mCount);
	}
	return -1;
}

// 0x42f310
bool Pak::contains(const Str& theName)
{
	if (mOpen == 0)
		return false;
	return find(theName) != -1;
}

// 0x42f3b0
int Pak::openEntry(const Str& theName)
{
	if (mOpen != 0)
	{
		int anIndex = find(theName);
		if (anIndex != -1)
		{
			crtSeek(mHandle, mEntries[anIndex].mOffset, 0);
			mInUse = 1;
			return PAK_HANDLE;
		}
	}
	return -1;
}

// 0x42f470
void Pak::closeEntry()
{
	mInUse = 0;
}

// 0x42f480
int Pak::read(int theHandle, void* theBuffer, unsigned int theCount)
{
	(void)theHandle;
	return crtRead(mHandle, theBuffer, theCount);
}

// 0x42f4a0
int Pak::size(const Str& theName)
{
	int anIndex = find(theName);
	if (anIndex == -1)
		return 0;
	return mEntries[anIndex].mSize;
}

} // namespace bonnies
