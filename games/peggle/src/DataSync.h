// DataSync.h: the binary files of the game (profiles, statistics, high scores, saved games, replays) are written and read
// by one function each: it calls the same Sync call for every value, and the DataSync either writes the value out or reads
// it in. The original's Sexy::DataSync with its DataReader and DataWriter. Values are little endian; a string is a
// 16-bit length and the characters; the "bit" values of a run are packed into bytes, eight to a byte, lowest bit first.

#ifndef __DATASYNC_H__
#define __DATASYNC_H__

#include "SharedPtr.h"

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
	size_t GetBytesRead() const { return mReadPos; } // how far reading has got (for a test that checks the whole file was used)

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

	// The objects that are written once and afterwards named by a number (0x4bb330, 0x4baac0, 0x406860): a code of 0 stands for
	// no object, 1 for an object that follows, and a larger one for an object that came before. An object gets its number
	// as it is first met, when writing and when reading alike, and the numbers start at 3 (the original counts the entries of
	// its table after adding the object, and adds 2).
	int RegisterObject(const void* theObject); // 0x4bb330: 0 when there is none, 1 when it is new (now numbered), else its number
	void* FindObject(int theNumber);		   // 0x4baac0: throws when no object has that number
	void ForgetObjects();					   // 0x4bb1f0: the numbers are given out again from the start

	// 0x406860: a shared object of the class T, which creates itself with its default constructor and syncs with
	// SyncState(DataSync&). On reading the new object is numbered before its contents are read.
	template <typename T>
	void SyncSharedObject(SharedPtr<T>& theObject)
	{
		if (mReading)
		{
			int aCode = 0;
			SyncLong(aCode);
			if (aCode == 0)
				theObject = nullptr;
			else if (aCode == 1)
			{
				T* aNew = new T();
				theObject = aNew;
				RegisterObject(aNew);
				aNew->SyncState(*this);
			}
			else
				theObject = (T*)FindObject(aCode);
		}
		else
		{
			int aCode = RegisterObject(theObject);
			SyncLong(aCode);
			if (aCode == 1)
				theObject->SyncState(*this);
		}
	}

	// 0x4067c0: the same for a class that has subclasses: after the code 1 comes the number of the object's class (its GetClass),
	// from which theFactory makes the object when it is read. Whatever theFactory makes counts as numbered before it is read.
	template <typename T, typename FactoryType>
	void SyncSharedPolymorphic(SharedPtr<T>& theObject, FactoryType theFactory)
	{
		if (mReading)
		{
			int aCode = 0;
			SyncLong(aCode);
			if (aCode == 0)
				theObject = nullptr;
			else if (aCode == 1)
			{
				int aClass = 0;
				SyncLong(aClass);
				T* aNew = theFactory(aClass);
				theObject = aNew;
				RegisterObject(aNew);
				aNew->SyncState(*this);
			}
			else
				theObject = (T*)FindObject(aCode);
		}
		else
		{
			int aCode = RegisterObject(theObject);
			SyncLong(aCode);
			if (aCode == 1)
			{
				int aClass = theObject->GetClass();
				SyncLong(aClass);
				theObject->SyncState(*this);
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
	std::map<const void*, int> mObjectNumbers; // what each registered object was numbered (the original's +0x68 table)
	std::map<int, void*> mObjectsByNumber;		// and the object of each number (+0x74)
	int mWriteBitCount;		// how many bits of the last written byte a bit run has used
};

} // namespace Sexy

#endif
