#ifndef __WORDHARMONY_ENGINE_STREAM_H__
#define __WORDHARMONY_ENGINE_STREAM_H__

// Input streams. The original has a file stream over stdio (vtable 0x484e24, 0x43f1f0), a stream over a file of the
// archive (vtable 0x4850a4, 0x441540) and a buffered stream (0x18 bytes, 0x43f350) that the resource manager wraps
// around either for byte-by-byte readers such as the XML parser. In the port every stream is a memory stream over the
// file's bytes (the host reads whole files), which buffers by itself.

#include "Object.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace engine
{

class InputStream : public Object
{
public:
	virtual void close() {}  // 0x4384a0
	// 0x43fc70
	int read(void* theBuffer, int theSize) { return read(theBuffer, 0, theSize); }
	// 0x43f0c0: reads into theBuffer + theOffset; returns the count read
	virtual int read(void* theBuffer, int theOffset, int theSize) = 0;
	// 0x43f0b0: one byte, or -1 at the end
	virtual int read() = 0;
	// 0x43f0f0: origin 0 the start, 1 the position, 2 the end
	virtual void seek(long theOffset, int theOrigin) = 0;
	virtual long tell() = 0;  // 0x43f150
	virtual long getSize() = 0;
};

typedef std::shared_ptr<const std::vector<uint8_t>> SharedData;

class MemoryStream : public InputStream
{
public:
	// A stream over theSize bytes of theData from theStart
	MemoryStream(const SharedData& theData, size_t theStart, size_t theSize);
	explicit MemoryStream(std::vector<uint8_t>&& theData);

	// 0x43f0c0 (the file stream), 0x4416e0 (the archive's)
	int read(void* theBuffer, int theOffset, int theSize) override;
	// 0x43f4c0 (the buffered stream's byte)
	int read() override;
	// 0x43f0f0, 0x43f4f0 (the buffered stream's)
	void seek(long theOffset, int theOrigin) override;
	// 0x43f150
	long tell() override { return (long)mPos; }
	long getSize() override { return (long)mSize; }

private:
	SharedData mData;
	size_t mStart;
	size_t mSize;
	size_t mPos;
};

// Reads a whole stream into theData
void readAll(InputStream* theStream, std::vector<uint8_t>& theData);

} // namespace engine

#endif
