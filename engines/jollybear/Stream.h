#ifndef __JB_STREAM_H__
#define __JB_STREAM_H__

// The engine's save stream (vtable 0x51e02c): it writes to a file or to a growing buffer, or reads from a file or
// from memory, with big-endian numbers and a running CRC-32 (the standard table at 0x527bc4) that Checksum writes
// or checks. Its constructor seeds the C runtime's generator with the tick count, as the original does.

#include "JBBase.h"

#include <cstdio>

namespace jb
{

class Stream;

// What Serialize hands the stream to (its first virtual)
class Streamable
{
public:
	virtual ~Streamable() {}
	virtual void Serialize(Stream& theStream) = 0;
};

class Stream
{
public:
	FILE* mFile = nullptr;      // +0x04
	uint32_t mCrc = 0xffffffff; // +0x08
	uint8_t* mBuffer = nullptr; // +0x0c
	int mUsed = 0;              // +0x10
	int mCapacity = 0;          // +0x14
	int mMode = 0;              // +0x18 1 writing, 2 reading
	const uint8_t* mData = nullptr;  // +0x1c
	int mSize = 0;              // +0x20
	int mPos = 0;               // +0x24
	bool mChecksumOk = false;   // +0x28

	// 0x424d30
	Stream();
	// 0x424d90
	virtual ~Stream();

	// 0x424da0, 0x424db0
	bool IsReading() const { return mMode == 2; }
	bool ChecksumOk() const { return mChecksumOk; }
	// 0x424dc0
	void Close();
	// 0x424e00
	bool OpenWrite(const char* thePath);
	// 0x424e40
	void OpenReadMemory(const uint8_t* theData, int theSize);
	// 0x424e70
	bool OpenRead(const char* thePath);
	// The mode a caller sets for writing into the buffer (mMode 1 without a file)
	void OpenWriteMemory() { Close(); mMode = 1; mCrc = 0xffffffff; }
	// 0x424eb0
	void Serialize(Streamable* theObject) { theObject->Serialize(*this); }
	// 0x424ec0
	void PutByte(uint8_t theByte);
	// 0x424f60: 0 at the end
	char GetByte();
	// 0x424fd0, 0x425050: the characters and a 0; read up to the 0 (the two copies do the same)
	void String(std::string& theText);
	// 0x4250d0: 8 bits, sign-extended when read
	void Char(int& theValue);
	// 0x425100: 16 bits, sign-extended when read
	void Short(int& theValue);
	// 0x425160: 16 bits, read without the sign
	void Word(int& theValue);
	// 0x4251b0
	void Int(int& theValue);
	// 0x425240: read true only for 1
	void Bool(bool& theValue);
	// 0x425280: the CRC's complement, written; or read and compared into mChecksumOk
	void Checksum();
	// 0x4252d0: a block added to the CRC (it is not written), then the checksum
	void ChecksumBlock(const uint8_t* theData, int theSize);
};

// 0x42f5e0
void Crc32Byte(uint8_t theByte, uint32_t& theCrc);
// 0x42f610
void Crc32Block(uint32_t& theCrc, const uint8_t* theData, int theSize);

} // namespace jb

#endif
