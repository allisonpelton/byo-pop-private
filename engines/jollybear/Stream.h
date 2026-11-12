#ifndef __JB_STREAM_H__
#define __JB_STREAM_H__

// The engine's save stream (vtable 0x51cf64 (mahjongchina), 0x51e02c (mahjongjapan)): it writes to a file or to a
// growing buffer, or reads from a file or from memory, with big-endian numbers and a running CRC-32 (the standard table
// at 0x528698 (mahjongchina), 0x527bc4 (mahjongjapan)) that Checksum writes or checks. Its constructor seeds the C
// runtime's generator with the tick count, as the original does.

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

	// 0x42a500 (mahjongchina), 0x424d30 (mahjongjapan)
	Stream();
	// 0x424d90 (mahjongjapan)
	virtual ~Stream();

	// 0x424da0 (mahjongjapan), 0x424db0 (mahjongjapan)
	bool IsReading() const { return mMode == 2; }
	bool ChecksumOk() const { return mChecksumOk; }
	// 0x42a570 (mahjongchina), 0x424dc0 (mahjongjapan)
	void Close();
	// 0x42a5b0 (mahjongchina), 0x424e00 (mahjongjapan)
	bool OpenWrite(const char* thePath);
	// 0x42a5f0 (mahjongchina), 0x424e40 (mahjongjapan)
	void OpenReadMemory(const uint8_t* theData, int theSize);
	// 0x42a620 (mahjongchina), 0x424e70 (mahjongjapan)
	bool OpenRead(const char* thePath);
	// The mode a caller sets for writing into the buffer (mMode 1 without a file)
	void OpenWriteMemory() { Close(); mMode = 1; mCrc = 0xffffffff; }
	// 0x42a660 (mahjongchina), 0x424eb0 (mahjongjapan)
	void Serialize(Streamable* theObject) { theObject->Serialize(*this); }
	// 0x42a670 (mahjongchina), 0x424ec0 (mahjongjapan)
	void PutByte(uint8_t theByte);
	// 0x42a710 (mahjongchina), 0x424f60 (mahjongjapan): 0 at the end
	char GetByte();
	// 0x42a780 (mahjongchina), 0x424fd0 (mahjongjapan), 0x42a800 (mahjongchina), 0x425050 (mahjongjapan): the
	// characters and a 0; read up to the 0 (the two copies do the same)
	void String(std::string& theText);
	// 0x42a880 (mahjongchina), 0x4250d0 (mahjongjapan): 8 bits, sign-extended when read
	void Char(int& theValue);
	// 0x42a8b0 (mahjongchina), 0x425100 (mahjongjapan): 16 bits, sign-extended when read
	void Short(int& theValue);
	// 0x42a910 (mahjongchina), 0x425160 (mahjongjapan): 16 bits, read without the sign
	void Word(int& theValue);
	// 0x42a960 (mahjongchina), 0x4251b0 (mahjongjapan)
	void Int(int& theValue);
	// 0x42a9f0 (mahjongchina), 0x425240 (mahjongjapan): read true only for 1
	void Bool(bool& theValue);
	// 0x42aa30 (mahjongchina), 0x425280 (mahjongjapan): the CRC's complement, written; or read and compared into
	// mChecksumOk
	void Checksum();
	// 0x42aa80 (mahjongchina), 0x4252d0 (mahjongjapan): a block added to the CRC (it is not written), then the checksum
	void ChecksumBlock(const uint8_t* theData, int theSize);
};

// 0x4440b0 (mahjongchina), 0x42f5e0 (mahjongjapan)
void Crc32Byte(uint8_t theByte, uint32_t& theCrc);
// 0x4440e0 (mahjongchina), 0x42f610 (mahjongjapan)
void Crc32Block(uint32_t& theCrc, const uint8_t* theData, int theSize);

} // namespace jb

#endif
