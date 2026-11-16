// DataSync.h: the binary files of the game (profiles, statistics, high scores, saved games, replays) are written and read
// by one function each: it calls the same Sync call for every value, and the DataSync either writes the value out or reads
// it in. The original's Sexy::DataSync with its DataReader and DataWriter. Values are little endian; a string is a
// 16-bit length and the characters; the "bit" values of a run are packed into bytes, eight to a byte, lowest bit first.

#ifndef __DATASYNC_H__
#define __DATASYNC_H__

#include <cstdint>
#include <cstring>
#include <exception>
#include <set>
#include <map>
#include <string>
#include <vector>

namespace Sexy
{

// Thrown when a file is read past its end (the original throws a plain exception there).
class DataSyncException : public std::exception
{
public:
	const char* what() const noexcept override { return "data sync: read past the end of the data"; }
};

class DataSync
{
public:
	DataSync();

	// Reading: the data must stay alive while the DataSync reads it (0x4bb6a0).
	void StartReadMemory(const void* theData, size_t theSize);
	// Writing: the written bytes are in GetWrittenData() (0x4bb6d0).
	void StartWriteMemory(size_t theInitialSize = 0x20);
	const std::vector<uint8_t>& GetWrittenData() const { return mWritten; }

	bool IsReading() const { return mReading; } // the original's mReader pointer being set (0x4)

	// 0x4ba810 - 0x4ba9b0: each reads into or writes out its value, whichever way the DataSync runs
	void SyncLong(int& theValue);
	void SyncLong(unsigned int& theValue);
	void SyncShort(int& theValue);		  // 16 bits, not sign extended when read
	void SyncSignedShort(int& theValue);  // 0x4ba870
	void SyncByte(int& theValue);		  // 8 bits, not sign extended when read
	void SyncSignedByte(int& theValue);
	void SyncBool(bool& theValue);
	void SyncBoolBit(bool& theValue);
	void EndBit();
	void SyncFloat(float& theValue);
	void SyncString(std::string& theValue);

	template <typename T>
	void SyncByte(T& theValue)
	{
		int aValue = (int)theValue;
		SyncByte(aValue);
		theValue = (T)aValue;
	}

	template <typename T>
	void SyncShort(T& theValue)
	{
		int aValue = (int)theValue;
		SyncShort(aValue);
		theValue = (T)aValue;
	}

	// 0x420600, 0x422390: a set of longs is its count and then its members
	template <typename SetType>
	void SyncLongSet(SetType& theSet)
	{
		if (mReading)
		{
			theSet.clear();
			int aCount = 0;
			SyncLong(aCount);
			for (int i = 0; i < aCount; i++)
			{
				int aValue = 0;
				SyncLong(aValue);
				theSet.insert(aValue);
			}
		}
		else
		{
			int aCount = (int)theSet.size();
			SyncLong(aCount);
			for (int aValue : theSet)
				SyncLong(aValue);
		}
	}

	// 0x41e330: a map from long to long is its count and then each key with its value
	template <typename MapType>
	void SyncLongMap(MapType& theMap)
	{
		if (mReading)
		{
			theMap.clear();
			int aCount = 0;
			SyncLong(aCount);
			for (int i = 0; i < aCount; i++)
			{
				int aKey = 0;
				int aValue = 0;
				SyncLong(aKey);
				SyncLong(aValue);
				theMap[aKey] = aValue;
			}
		}
		else
		{
			int aCount = (int)theMap.size();
			SyncLong(aCount);
			for (auto& aPair : theMap)
			{
				int aKey = aPair.first;
				int aValue = aPair.second;
				SyncLong(aKey);
				SyncLong(aValue);
			}
		}
	}

	// 0x4224c0: a set of strings is its count and then its members
	template <typename SetType>
	void SyncStringSet(SetType& theSet)
	{
		if (mReading)
		{
			theSet.clear();
			int aCount = 0;
			SyncLong(aCount);
			for (int i = 0; i < aCount; i++)
			{
				std::string aValue;
				SyncString(aValue);
				theSet.insert(aValue);
			}
		}
		else
		{
			int aCount = (int)theSet.size();
			SyncLong(aCount);
			for (const std::string& aValue : theSet)
			{
				std::string aCopy = aValue;
				SyncString(aCopy);
			}
		}
	}

	int mVersion; // the file format version a Sync function may test (0x48)

private:
	void WriteBytes(const void* theData, size_t theSize);
	void ReadBytes(void* theData, size_t theSize);
	uint8_t ReadByte();

	bool mReading;
	const uint8_t* mReadData;
	size_t mReadSize;
	size_t mReadPos;
	int mReadBitPos;		// the next bit of mReadBits to give out; 7 until a bit run has started
	uint8_t mReadBits;
	std::vector<uint8_t> mWritten;
	int mWriteBitCount;		// how many bits of the last written byte a bit run has used
};

} // namespace Sexy

#endif
