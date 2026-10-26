#include "BinFile.h"
#include "platform/VFS.h"
#include <cstdlib>
#include <cstring>

using namespace Sexy;

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

// 0x414a50: the original opens read-only and then, either way, read-write (the handle it keeps)
void BinFile::Open(const char* theName, bool readOnly)
{
	mData.clear();
	mPos = 0;
	mOpen = Platform::VFS::Get().ReadAll(theName, mData);
}

// 0x414a90
void BinFile::Close()
{
	mData.clear();
	mPos = 0;
	mOpen = false;
}

// 0x414aa0
void BinFile::Skip(long theCount)
{
	Seek((long) mPos + theCount);
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
