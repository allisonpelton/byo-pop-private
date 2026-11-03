#ifndef __BONNIES_FILE_H__
#define __BONNIES_FILE_H__

// The engine's files (0x429a70-0x42a390) and its pak, Data/Bonnie.pak (0x42f090-0x42f530, the object 0x481abc).
// The original used the C runtime's _open/_read/_write handles; here a handle is an index into a table of files held in
// memory (File.cpp), read from the user's folder when the file is there and from the game folder otherwise, and written
// back to the user's folder when closed. A file in the pak is opened by its base name and read through the one handle
// 0x3039, which reads on from the pak's position with no end of its own, as the original's did.

#include "Str.h"

namespace bonnies
{

enum
{
	PAK_HANDLE = 0x3039
};

// 0x429ee0: opens thePath (in the pak first when reading); theWrite creates or truncates it unless theAppend, which
// opens an existing file at its end. -1 when it cannot be opened.
int fileOpen(const Str& thePath, bool theWrite, bool theAppend);
// 0x429ab0
void fileClose(int theHandle);
// 0x429a70: _read; the number of bytes read
int fileRead(int theHandle, void* theBuffer, unsigned int theCount);
// _write, which the game also calls itself (zeros in its saves)
int crtWrite(int theHandle, const void* theBuffer, unsigned int theCount);
// 0x429ae0: up to theMax bytes as a string (to the first zero byte)
Str fileReadText(int theHandle, unsigned int theMax);
// 0x429db0, 0x429df0, 0x42a340: an int and a byte
void fileReadInt(int theHandle, void* theValue);
void fileReadByte(int theHandle, void* theValue);
// 0x429e30: a string as its length and its bytes; the bytes read
int fileReadString(int theHandle, Str& theStr);
// 0x429cb0, 0x42a320, 0x429cd0, 0x429cf0: the same written
void fileWriteInt(int theHandle, const void* theValue);
void fileWriteByte(int theHandle, const void* theValue);
void fileWriteString(int theHandle, const Str* theStr);
// 0x429ba0: removes a file (in the user's folder); whether it was removed
bool fileRemove(const Str& thePath);
// 0x429c00: the name after the last / or backslash
Str baseName(const Str& thePath);
// 0x42a390: whether thePath is in the pak (by its base name) or can be opened
bool fileExists(const Str& thePath);
// 0x429ff0: the names (without extension) of the files in theDir whose names contain theFilter, at most 999
void listFiles(const Str& theDir, const Str& theFilter, int* theCount, Str** theNames);

class Pak
{
public:
	// 0x42f090, 0x42f0a0
	Pak();
	~Pak();
	// 0x42f0b0: reads Data/Bonnie.pak's table
	void open();
	// 0x42f230
	void close();
	// 0x42f270: the entry of a name (by hashName), -1 when it is not in the pak
	int find(const Str& theName);
	// 0x42f310
	bool contains(const Str& theName);
	// 0x42f3b0: seeks to the named entry; PAK_HANDLE, or -1
	int openEntry(const Str& theName);
	// 0x42f470
	void closeEntry();
	// 0x42f480: reads on from the pak's position
	int read(int theHandle, void* theBuffer, unsigned int theCount);
	// 0x42f4a0: the named entry's size, 0 when it is not in the pak
	int size(const Str& theName);

	struct Entry
	{
		uint32_t mHash;
		int32_t mOffset;
		int32_t mSize;
	};

	int mOpen;        // +0x00
	int mCount;       // +0x04
	int mHandle;      // +0x08
	Entry* mEntries;  // +0x0c
	int mInUse;       // +0x10
};

extern Pak* gPak;  // 0x481abc

} // namespace bonnies

#endif
