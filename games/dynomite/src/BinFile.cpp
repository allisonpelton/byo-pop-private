#include "BinFile.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

std::map<std::string, std::vector<uint8_t> > BinFile::sMemoryFiles;

// 0x414a00
BinFile::BinFile()
{
	mPos = 0;
	mOpen = false;
}

// 0x414ce0
BinFile::~BinFile()
{
	Close();
}

// port: a name UserPath made
static bool IsUserPath(const std::string& theName)
{
	std::string aPrefix = Platform::VFS::Get().GetUserDir() + "/";
	return theName.compare(0, aPrefix.size(), aPrefix) == 0;
}

// port: a host file read whole
static bool ReadHostFile(const std::string& thePath, std::vector<uint8_t>& theData)
{
	FILE* aFile = fopen(thePath.c_str(), "rb");
	if (aFile == NULL)
		return false;
	fseek(aFile, 0, SEEK_END);
	long aSize = ftell(aFile);
	fseek(aFile, 0, SEEK_SET);
	theData.resize(aSize > 0 ? (size_t) aSize : 0);
	if (aSize > 0)
		theData.resize(fread(&theData[0], 1, (size_t) aSize, aFile));
	fclose(aFile);
	return true;
}

// 0x414a10: _creat, then the file opened read-write
void BinFile::Create(const char* theName)
{
	mData.clear();
	mPos = 0;
	mOpen = true;
	mWritePath = theName;
}

// 0x414a50: the original opens read-only and then, either way, read-write (the handle it keeps). Port: the saved
// games are read from the user folder, the game's files through the VFS; writes after Open stay in memory
void BinFile::Open(const char* theName, bool readOnly)
{
	mData.clear();
	mPos = 0;
	mWritePath.clear();
	if (IsUserPath(theName))
	{
		mOpen = ReadHostFile(theName, mData);
		return;
	}
	std::map<std::string, std::vector<uint8_t> >::iterator aFile = sMemoryFiles.find(theName);
	if (aFile != sMemoryFiles.end())
	{
		mData = aFile->second;
		mOpen = true;
		return;
	}
	mOpen = Platform::VFS::Get().ReadAll(theName, mData);
}

// 0x414a90. port: a file made by Create is written out now, into the user folder (or kept in memory, see the header)
void BinFile::Close()
{
	if (mOpen && !mWritePath.empty())
	{
		if (IsUserPath(mWritePath))
		{
			Platform::MakeDirs(Platform::VFS::Get().GetUserDir());
			FILE* aFile = fopen(mWritePath.c_str(), "wb");
			if (aFile != NULL)
			{
				if (!mData.empty())
					fwrite(&mData[0], 1, mData.size(), aFile);
				fclose(aFile);
				// port (web): the saved game reaches the browser's storage
				Platform::UserDataChanged();
			}
		}
		else
			SetMemoryFile(mWritePath.c_str(), mData);
	}
	mWritePath.clear();
	mData.clear();
	mPos = 0;
	mOpen = false;
}

// 0x414aa0
void BinFile::Skip(long theCount)
{
	Seek((long) mPos + theCount);
}

// 0x414ac0
void BinFile::WriteBool(bool theValue)
{
	unsigned char aByte = theValue ? 1 : 0;
	Write(&aByte, 1);
}

// 0x414ae0
void BinFile::WriteInt(int theValue)
{
	Write(&theValue, 4);
}

// port: 0x414ae0, given the float's bits
void BinFile::WriteFloat(float theValue)
{
	Write(&theValue, 4);
}

// 0x414b00
void BinFile::WriteString(const char* theString)
{
	unsigned int aLength = (unsigned int) strlen(theString) + 1;
	Write(&aLength, 4);
	Write(theString, aLength);
}

// 0x414b50: like _write, at the position, the file growing as needed
void BinFile::Write(const void* theBuffer, unsigned int theSize)
{
	if (!mOpen)
		return;
	if (mData.size() < mPos + theSize)
		mData.resize(mPos + theSize);
	if (theSize != 0)
		memcpy(&mData[mPos], theBuffer, theSize);
	mPos += theSize;
}

// 0x414b70
bool BinFile::ReadBool()
{
	unsigned char aByte = 0;
	Read(&aByte, 1);
	return aByte != 0;
}

// 0x414b90 (port: 0 past the end, where the original returns what its stack held)
int BinFile::ReadInt()
{
	int aValue = 0;
	Read(&aValue, 4);
	return aValue;
}

// 0x414bb0
float BinFile::ReadFloat()
{
	float aValue = 0;
	Read(&aValue, 4);
	return aValue;
}

// 0x414bd0
void BinFile::ReadString(void* theBuffer)
{
	unsigned int aLength = 0;
	Read(&aLength, 4);
	Read(theBuffer, aLength);
}

// 0x414c00: like _read, as much as the file still has
void BinFile::Read(void* theBuffer, unsigned int theSize)
{
	if (!mOpen || (mPos >= mData.size()))
		return;
	if (theSize > mData.size() - mPos)
		theSize = (unsigned int) (mData.size() - mPos);
	memcpy(theBuffer, &mData[mPos], theSize);
	mPos += theSize;
}

// 0x414c20
void* BinFile::ReadAlloc()
{
	unsigned int aLength = 0;
	Read(&aLength, 4);
	void* aBuffer = malloc(aLength);
	Read(aBuffer, aLength);
	return aBuffer;
}

// 0x414c70
void BinFile::CopyFrom(BinFile* theFile, unsigned int theSize)
{
	void* aBuffer = malloc(theSize);
	theFile->Read(aBuffer, theSize);
	Write(aBuffer, theSize);
	free(aBuffer);
}

// 0x414c60
int BinFile::Length()
{
	return (int) mData.size();
}

// 0x414cb0
int BinFile::Tell()
{
	return (int) mPos;
}

// 0x414cc0
void BinFile::Seek(long thePos)
{
	if (thePos < 0)
		thePos = 0;
	mPos = (size_t) thePos;
}

void BinFile::SetMemoryFile(const char* theName, const std::vector<uint8_t>& theData)
{
	sMemoryFiles[theName] = theData;
}

std::string BinFile::UserPath(const char* theName)
{
	return Platform::VFS::Get().GetUserDir() + "/" + theName;
}

bool BinFile::UserFileExists(const char* theName)
{
	return Platform::IsFile(UserPath(theName));
}
