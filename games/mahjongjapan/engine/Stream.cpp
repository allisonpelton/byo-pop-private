#include "Stream.h"

#include <cstring>

namespace jb
{

static uint32_t gCrcTable[256];
static bool gCrcTableMade = false;

// The table at 0x527bc4 is the standard one of the reflected polynomial 0xedb88320
static void MakeCrcTable()
{
	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t c = i;
		for (int k = 0; k < 8; k++)
			c = (c & 1) ? (c >> 1) ^ 0xedb88320u : c >> 1;
		gCrcTable[i] = c;
	}
	gCrcTableMade = true;
}

// 0x42f5e0
void Crc32Byte(uint8_t theByte, uint32_t& theCrc)
{
	if (!gCrcTableMade)
		MakeCrcTable();
	theCrc = gCrcTable[(theCrc & 0xff) ^ theByte] ^ (theCrc >> 8);
}

// 0x42f610
void Crc32Block(uint32_t& theCrc, const uint8_t* theData, int theSize)
{
	int i = 0;
	theCrc = ~theCrc;
	if (0 < theSize)
	{
		do
		{
			Crc32Byte(theData[i], theCrc);
			i = i + 1;
		} while (i < theSize);
	}
	theCrc = ~theCrc;
}

// 0x424d30
Stream::Stream()
{
	mCrc = 0xffffffff;
	CrtSrand(TickCount());
}

// 0x424d90
Stream::~Stream()
{
	Close();
}

// 0x424dc0
void Stream::Close()
{
	if (mBuffer != nullptr)
	{
		delete[] mBuffer;
		mBuffer = nullptr;
	}
	mUsed = 0;
	mCapacity = 0;
	mPos = 0;
	mChecksumOk = false;
	if (mFile != nullptr)
	{
		fclose(mFile);
		mFile = nullptr;
	}
}

// 0x424e00
bool Stream::OpenWrite(const char* thePath)
{
	Close();
	mMode = 1;
	mCrc = 0xffffffff;
	mFile = fopen(thePath, "wb");
	return mFile != nullptr;
}

// 0x424e40
void Stream::OpenReadMemory(const uint8_t* theData, int theSize)
{
	Close();
	mData = theData;
	mSize = theSize;
	mMode = 2;
	mCrc = 0xffffffff;
}

// 0x424e70
bool Stream::OpenRead(const char* thePath)
{
	Close();
	mMode = 2;
	mCrc = 0xffffffff;
	mFile = fopen(thePath, "rb");
	return mFile != nullptr;
}

// 0x424ec0
void Stream::PutByte(uint8_t theByte)
{
	Crc32Byte(theByte, mCrc);
	if (mFile == nullptr)
	{
		if (mCapacity <= mUsed)
		{
			uint8_t* anOld = mBuffer;
			mBuffer = new uint8_t[mCapacity + 0x400];
			if (anOld != nullptr)
				memcpy(mBuffer, anOld, mCapacity);
			mCapacity = mCapacity + 0x400;
			delete[] anOld;
		}
		mBuffer[mUsed] = theByte;
		mUsed = mUsed + 1;
		return;
	}
	fputc(theByte, mFile);
}

// 0x424f60
char Stream::GetByte()
{
	if (mFile == nullptr)
	{
		if (mSize <= mPos)
			return 0;
		uint8_t aByte = mData[mPos];
		Crc32Byte(aByte, mCrc);
		mPos = mPos + 1;
		return (char)aByte;
	}
	int aByte = fgetc(mFile);
	if (aByte == EOF)
		return 0;
	Crc32Byte((uint8_t)aByte, mCrc);
	return (char)aByte;
}

// 0x424fd0, 0x425050
void Stream::String(std::string& theText)
{
	if (mMode == 1)
	{
		int i = 0;
		char aChar = theText.c_str()[0];
		while (aChar != '\0')
		{
			PutByte((uint8_t)aChar);
			i = i + 1;
			aChar = theText.c_str()[i];
		}
		PutByte(0);
		return;
	}
	if (mMode == 2)
	{
		theText.clear();
		char aChar = GetByte();
		while (aChar != '\0')
		{
			theText += aChar;
			aChar = GetByte();
		}
	}
}

// 0x4250d0
void Stream::Char(char& theValue)
{
	if (mMode == 1)
	{
		PutByte((uint8_t)theValue);
		return;
	}
	if (mMode == 2)
		theValue = GetByte();
}

// 0x425100
void Stream::Short(int& theValue)
{
	if (mMode == 1)
	{
		PutByte((uint8_t)((theValue & 0xffff) >> 8));
		PutByte((uint8_t)theValue);
		return;
	}
	if (mMode == 2)
	{
		uint8_t aHigh = (uint8_t)GetByte();
		uint8_t aLow = (uint8_t)GetByte();
		theValue = (int16_t)((aHigh << 8) | aLow);
	}
}

// 0x425160
void Stream::Word(int& theValue)
{
	if (mMode == 1)
	{
		PutByte((uint8_t)(theValue >> 8));
		PutByte((uint8_t)theValue);
		return;
	}
	if (mMode == 2)
	{
		uint8_t aHigh = (uint8_t)GetByte();
		uint8_t aLow = (uint8_t)GetByte();
		theValue = (aHigh << 8) | aLow;
	}
}

// 0x4251b0
void Stream::Int(int& theValue)
{
	if (mMode == 1)
	{
		PutByte((uint8_t)(theValue >> 0x18));
		PutByte((uint8_t)(theValue >> 0x10));
		PutByte((uint8_t)(theValue >> 8));
		PutByte((uint8_t)theValue);
		return;
	}
	if (mMode == 2)
	{
		uint32_t a = (uint8_t)GetByte();
		uint32_t b = (uint8_t)GetByte();
		uint32_t c = (uint8_t)GetByte();
		uint32_t d = (uint8_t)GetByte();
		theValue = (int)((((a << 8) | b) << 8 | c) << 8 | d);
	}
}

// 0x425240
void Stream::Bool(bool& theValue)
{
	if (mMode == 1)
	{
		PutByte(theValue ? 1 : 0);
		return;
	}
	if (mMode == 2)
		theValue = GetByte() == 1;
}

// 0x425280
void Stream::Checksum()
{
	if (mMode == 1)
	{
		int aValue = (int)~mCrc;
		Int(aValue);
		return;
	}
	if (mMode == 2)
	{
		uint32_t anExpected = ~mCrc;
		int aValue;
		Int(aValue);
		mChecksumOk = anExpected == (uint32_t)aValue;
	}
}

// 0x4252d0
void Stream::ChecksumBlock(const uint8_t* theData, int theSize)
{
	Crc32Block(mCrc, theData, theSize);
	Checksum();
}

} // namespace jb
