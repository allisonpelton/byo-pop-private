// DataSync.cpp: see DataSync.h.

#include "DataSync.h"

using namespace Sexy;

// 0x4bb6f0
DataSync::DataSync()
	: mVersion(0), mReading(false), mReadData(nullptr), mReadSize(0), mReadPos(0), mReadBitPos(7), mReadBits(0),
	  mWriteBitCount(0)
{
}

// 0x4bb6a0
void DataSync::StartReadMemory(const void* theData, size_t theSize)
{
	mReading = true;
	mReadData = (const uint8_t*)theData;
	mReadSize = theSize;
	mReadPos = 0;
	mReadBitPos = 7;
	mReadBits = 0;
}

// 0x4bb6d0
void DataSync::StartWriteMemory(size_t theInitialSize)
{
	mReading = false;
	mWritten.clear();
	mWritten.reserve(theInitialSize < 0x20 ? 0x20 : theInitialSize);
	mWriteBitCount = 0;
}

// 0x4bbf70: every ordinary read puts an open bit run to an end
void DataSync::ReadBytes(void* theData, size_t theSize)
{
	mReadBitPos = 0;
	if (mReadPos + theSize > mReadSize)
		throw DataSyncException();
	memcpy(theData, mReadData + mReadPos, theSize);
	mReadPos += theSize;
}

uint8_t DataSync::ReadByte()
{
	uint8_t aByte;
	ReadBytes(&aByte, 1);
	return aByte;
}

// 0x4bbb10: an open bit run is closed before an ordinary write
void DataSync::WriteBytes(const void* theData, size_t theSize)
{
	mWriteBitCount = 0;
	const uint8_t* aBytes = (const uint8_t*)theData;
	mWritten.insert(mWritten.end(), aBytes, aBytes + theSize);
}

// 0x4ba810
void DataSync::SyncLong(int& theValue)
{
	if (mReading)
		ReadBytes(&theValue, 4);
	else
		WriteBytes(&theValue, 4);
}

void DataSync::SyncLong(unsigned int& theValue)
{
	if (mReading)
		ReadBytes(&theValue, 4);
	else
		WriteBytes(&theValue, 4);
}

// 0x4ba840
void DataSync::SyncShort(int& theValue)
{
	if (mReading)
	{
		uint16_t aValue;
		ReadBytes(&aValue, 2);
		theValue = aValue;
	}
	else
	{
		uint16_t aValue = (uint16_t)theValue;
		WriteBytes(&aValue, 2);
	}
}

// 0x4ba870
void DataSync::SyncSignedShort(int& theValue)
{
	if (mReading)
	{
		int16_t aValue;
		ReadBytes(&aValue, 2);
		theValue = aValue;
	}
	else
	{
		int16_t aValue = (int16_t)theValue;
		WriteBytes(&aValue, 2);
	}
}

// 0x4ba8a0
void DataSync::SyncByte(int& theValue)
{
	if (mReading)
		theValue = ReadByte();
	else
	{
		uint8_t aValue = (uint8_t)theValue;
		WriteBytes(&aValue, 1);
	}
}

// 0x4ba8d0
void DataSync::SyncSignedByte(int& theValue)
{
	if (mReading)
		theValue = (int8_t)ReadByte();
	else
	{
		int8_t aValue = (int8_t)theValue;
		WriteBytes(&aValue, 1);
	}
}

// 0x4ba920
void DataSync::SyncBool(bool& theValue)
{
	if (mReading)
		theValue = ReadByte() != 0;
	else
	{
		uint8_t aValue = theValue ? 1 : 0;
		WriteBytes(&aValue, 1);
	}
}

// 0x4ba980
void DataSync::SyncFloat(float& theValue)
{
	if (mReading)
		ReadBytes(&theValue, 4);
	else
		WriteBytes(&theValue, 4);
}

// 0x4ba9b0: a 16-bit length, then the characters
void DataSync::SyncString(std::string& theValue)
{
	if (mReading)
	{
		uint16_t aLength;
		ReadBytes(&aLength, 2);
		theValue.assign(aLength, '\0');
		if (aLength > 0)
			ReadBytes(&theValue[0], aLength);
	}
	else
	{
		uint16_t aLength = (uint16_t)theValue.size();
		WriteBytes(&aLength, 2);
		WriteBytes(theValue.data(), aLength);
	}
}

// 0x4ba950 (reading: 0x4bc290; writing: 0x4bbc90)
void DataSync::SyncBoolBit(bool& theValue)
{
	if (mReading)
	{
		if (mReadBitPos == 0)
			mReadBits = ReadByte();
		bool aBit = (mReadBits & (1 << mReadBitPos)) != 0;
		// ReadByte put the run to an end; the bit position moves on from here
		if (++mReadBitPos > 7)
			mReadBitPos = 0;
		theValue = aBit;
	}
	else
	{
		if (mWriteBitCount == 0)
		{
			mWritten.push_back(0);
		}
		if (theValue)
			mWritten.back() |= (uint8_t)(1 << mWriteBitCount);
		if (++mWriteBitCount > 7)
			mWriteBitCount = 0;
	}
}

// 0x4ba900: the next bit run starts at a new byte
void DataSync::EndBit()
{
	if (mReading)
		mReadBitPos = 0;
	else
		mWriteBitCount = 0;
}
