#ifndef __SPROUT2_STREAM_H__
#define __SPROUT2_STREAM_H__

// The engine's input streams (a file of the game's folder is the host's FileInputStream, a file in an archive the
// archive's), and the reader that reads a stream one character at a time (the XML parser's input).

#include "Object.h"

#include <cstdint>
#include <string>
#include <vector>

namespace engine
{

class InputStream : public Object
{
public:
	// The bytes read, at most theSize; 0 at the end
	virtual int read(void* theBuffer, int theSize) = 0;
	// True when the stream closed cleanly
	virtual bool close() = 0;
};

class RandomAccessInputStream : public InputStream
{
public:
	enum Origin
	{
		ORIGIN_SET = 0,
		ORIGIN_CURRENT = 1,
		ORIGIN_END = 2
	};

	virtual bool seek(int theOffset, int theOrigin) = 0;
	virtual int tell() = 0;
};

class Reader : public Object
{
public:
	virtual bool close() = 0;
	virtual int read(void* theBuffer, int theSize) = 0;
	// The next character, -1 at the end
	virtual int read() = 0;
};

// 0x41dbb0 (feedingfrenzy2) makes one over a stream (the resource manager's 0x41dc70 (feedingfrenzy2))
class InputStreamReader : public Reader
{
public:
	explicit InputStreamReader(InputStream* theStream) : mStream(theStream) {}

	// 0x51406f (feedingfrenzy2)
	bool close() override { return mStream->close(); }
	// 0x514067 (feedingfrenzy2)
	int read(void* theBuffer, int theSize) override { return mStream->read(theBuffer, theSize); }
	// 0x44ab50 (feedingfrenzy2): a character read as a signed char
	int read() override
	{
		char aChar;
		if (mStream->read(&aChar, 1) != 1)
			return -1;
		return aChar;
	}

private:
	Ref<InputStream> mStream;
};

} // namespace engine

#endif
