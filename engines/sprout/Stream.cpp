#include "Stream.h"
#include <cstring>

using namespace engine;

MemoryStream::MemoryStream(const SharedData& theData, size_t theStart, size_t theSize)
	: mData(theData), mStart(theStart), mSize(theSize), mPos(0)
{
}

MemoryStream::MemoryStream(std::vector<uint8_t>&& theData)
	: mData(std::make_shared<const std::vector<uint8_t>>(std::move(theData))), mStart(0), mPos(0)
{
	mSize = mData->size();
}

int MemoryStream::read(void* theBuffer, int theOffset, int theSize)
{
	size_t aCount = (size_t)theSize;
	if (aCount > mSize - mPos)
		aCount = mSize - mPos;
	memcpy((uint8_t*)theBuffer + theOffset, mData->data() + mStart + mPos, aCount);
	mPos += aCount;
	return (int)aCount;
}

int MemoryStream::read()
{
	if (mPos >= mSize)
		return -1;
	return (*mData)[mStart + mPos++];
}

void MemoryStream::seek(long theOffset, int theOrigin)
{
	long aPos = theOffset;
	if (theOrigin == 1)
		aPos += (long)mPos;
	else if (theOrigin == 2)
		aPos += (long)mSize;
	if (aPos < 0)
		aPos = 0;
	if ((size_t)aPos > mSize)
		aPos = (long)mSize;
	mPos = (size_t)aPos;
}

void engine::readAll(InputStream* theStream, std::vector<uint8_t>& theData)
{
	long aSize = theStream->getSize() - theStream->tell();
	theData.resize(aSize > 0 ? (size_t)aSize : 0);
	if (!theData.empty())
		theStream->read(theData.data(), (int)theData.size());
}
