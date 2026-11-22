#include "TheoraReader.h"

#include "Engine.h"
#include "Stream.h"
#include "Surface.h"

#include <cstdio>
#include <cstring>

namespace engine
{

namespace
{

// 0x45f130 (feedingfrenzy2): a little-endian number of the stream, read byte by byte
uint32_t readUint32(InputStream* theStream)
{
	unsigned char aBytes[4] = {0, 0, 0, 0};
	theStream->read(aBytes, 4);
	uint32_t aValue = 0;
	for (int i = 4; i > 0; i--)
		aValue = (aValue << 8) | aBytes[i - 1];
	return aValue;
}

// 0x45f240 (feedingfrenzy2)
int64_t readInt64(InputStream* theStream)
{
	unsigned char aBytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	theStream->read(aBytes, 8);
	uint64_t aValue = 0;
	for (int i = 8; i > 0; i--)
		aValue = (aValue << 8) | aBytes[i - 1];
	return (int64_t)aValue;
}

int clampByte(int theValue)
{
	if (theValue < 0)
		return 0;
	if (theValue > 0xff)
		return 0xff;
	return theValue;
}

} // namespace

TheoraReader::TheoraReader() : mSetup(nullptr), mDecoder(nullptr), mFrameIndex(0)
{
	th_info_init(&mInfo);
	th_comment_init(&mComment);
	memset(&mPacket, 0, sizeof(mPacket));
	memset(mFrame, 0, sizeof(mFrame));
}

TheoraReader::~TheoraReader()
{
	// 0x418e70 (feedingfrenzy2): the decoder, its headers and the file let go
	if (mDecoder)
		th_decode_free(mDecoder);
	if (mSetup)
		th_setup_free(mSetup);
	th_comment_clear(&mComment);
	th_info_clear(&mInfo);
}

void TheoraReader::open(const std::string& thePath)
{
	// The engine's 0x5c and 0x60 around the opening: the archive's digests left unchecked for it (the port never
	// checks them)
	mStream = getEngine()->openFile(thePath);
	// The original reads on from a file that did not open; the port takes it as one with no frames
	if (!mStream)
	{
		mPacket.e_o_s = 1;
		return;
	}
	// 0x47ba10 (feedingfrenzy2), the old API's theora_decode_header, for each header; its result is not checked
	for (int i = 0; i < 3; i++)
	{
		readPacket();
		th_decode_headerin(&mInfo, &mComment, &mSetup, &mPacket);
	}
	// 0x4873f0 (feedingfrenzy2), theora_decode_init
	mDecoder = th_decode_alloc(&mInfo, mSetup);
	mFrameIndex = 0;
	// _splitpath without a drive: the folder of the path, its last separator kept
	size_t aSeparator = thePath.find_last_of("\\/");
	mFolder = aSeparator == std::string::npos ? std::string() : thePath.substr(0, aSeparator + 1);
}

void TheoraReader::readPacket()
{
	mPacket.bytes = (long)readUint32(mStream);
	mPacket.b_o_s = (long)readUint32(mStream);
	mPacket.e_o_s = (long)readUint32(mStream);
	mPacket.granulepos = readInt64(mStream);
	mPacket.packetno = readInt64(mStream);
	// The buffer is made larger when a packet does not fit, and kept
	if (mBytes.size() < (size_t)(uint32_t)mPacket.bytes)
		mBytes.resize((size_t)(uint32_t)mPacket.bytes);
	mPacket.packet = mBytes.empty() ? nullptr : mBytes.data();
	mStream->read(mPacket.packet, (int)mPacket.bytes);
}

Surface* TheoraReader::readFrame()
{
	if (atEnd())
		return nullptr;
	readPacket();
	// A frame's own image kept by the engine under "<folder><frame>.png" takes the decoded one's place, and the packet
	// is then not decoded
	char aName[512];
	snprintf(aName, sizeof(aName), "%s%d.png", mFolder.c_str(), (int)mFrameIndex);
	Surface* aFrame = getEngine()->findImage(aName);
	if (!aFrame)
	{
		// 0x46baf0 and 0x4727e0 (feedingfrenzy2), theora_decode_packetin and theora_decode_YUVout
		th_decode_packetin(mDecoder, &mPacket, nullptr);
		th_decode_ycbcr_out(mDecoder, mFrame);
		aFrame = convertFrame();
		aFrame->setName(aName);
		// Its alpha from "<folder>_alpha.<frame>.png" where there is one
		char anAlphaName[512];
		snprintf(anAlphaName, sizeof(anAlphaName), "%s_alpha.%d.png", mFolder.c_str(), (int)mFrameIndex);
		if (getEngine()->fileExists(anAlphaName))
			aFrame->loadMask(anAlphaName, false, false);
	}
	mFrameIndex++;
	return aFrame;
}

void TheoraReader::skipFrame()
{
	if (atEnd())
		return;
	readPacket();
	th_decode_packetin(mDecoder, &mPacket, nullptr);
	th_decode_ycbcr_out(mDecoder, mFrame);
	mFrameIndex++;
}

Surface* TheoraReader::convertFrame()
{
	// The engine's 0x9c (0x4673d0 (feedingfrenzy2)): a new surface of the size
	Surface* aSurface = new Surface();
	aSurface->allocate((int)mInfo.pic_width, (int)mInfo.pic_height);
	aSurface->setName("[TheoraFrame]");
	int aWidth = aSurface->getWidth();
	int aHeight = aSurface->getHeight();
	// The planes from the coded frame's top left corner, whatever the picture's offset
	const unsigned char* aLuma = mFrame[0].data;
	int aLumaStride = mFrame[0].stride;
	int aChromaWidth = mFrame[1].width;
	int aChromaHeight = mFrame[1].height;
	int aChromaStride = mFrame[1].stride;
	const unsigned char* aBlueRow = mFrame[1].data;
	const unsigned char* aRedRow = mFrame[2].data;
	// Each pixel's colour difference interpolated between the four nearest chroma samples, in 256ths: the step, per
	// pixel and per row, moves across the chroma plane a little less than half a sample
	int aStepX = (aChromaWidth * 0x100 - 0x100) / (aChromaWidth * 2);
	int aStepY = (aChromaHeight * 0x100 - 0x100) / (aChromaHeight * 2);
	uint32_t* aPixel = aSurface->getPixels();
	int aPositionY = 0;
	for (int aY = 0; aY < aHeight; aY++)
	{
		int aRowOffset = (aPositionY >> 8) * aChromaStride;
		aBlueRow += aRowOffset;
		aRedRow += aRowOffset;
		aPositionY &= 0xff;
		const unsigned char* aBlue = aBlueRow;
		const unsigned char* aRed = aRedRow;
		int aPositionX = 0;
		for (int aX = 0; aX < aWidth; aX++)
		{
			int aFractionX = aPositionX & 0xff;
			aRed += aPositionX >> 8;
			aBlue += aPositionX >> 8;
			int aLowerRight = (aFractionX * aPositionY) >> 8;
			int anUpperRight = ((0x100 - aPositionY) * aFractionX) >> 8;
			int aLowerLeft = ((0x100 - aFractionX) * aPositionY) >> 8;
			int anUpperLeft = 0x101 - anUpperRight - aLowerLeft - aLowerRight;
			int aBlueDifference = (((int)aBlue[aChromaStride + 1] * aLowerRight + (int)aBlue[1] * anUpperRight +
			                           (int)aBlue[0] * anUpperLeft + (int)aBlue[aChromaStride] * aLowerLeft) >>
			                              8 &
			                          0xff) -
			                      0x80;
			int aLumaTerm = ((int)*aLuma - 0x10) * 0x12a;
			int aRedDifference = (((int)aRed[aChromaStride + 1] * aLowerRight + (int)aRed[1] * anUpperRight +
			                          (int)aRed[0] * anUpperLeft + (int)aRed[aChromaStride] * aLowerLeft) >>
			                             8 &
			                         0xff) -
			                     0x80;
			int aRedValue = clampByte((aRedDifference * 0x199 + 0x80 + aLumaTerm) >> 8);
			int aGreenValue = clampByte((aBlueDifference * -100 + aRedDifference * -0xd0 + 0x80 + aLumaTerm) >> 8);
			int aBlueValue = clampByte((aBlueDifference * 0x204 + 0x80 + aLumaTerm) >> 8);
			// The pixel's alpha byte is left as it was
			*aPixel = (*aPixel & 0xff000000u) | ((uint32_t)aRedValue << 16) | ((uint32_t)aGreenValue << 8) |
			          (uint32_t)aBlueValue;
			aPositionX = aFractionX + aStepX;
			aLuma++;
			aPixel++;
		}
		aPositionY += aStepY;
		aLuma += aLumaStride - aWidth;
	}
	return aSurface;
}

} // namespace engine
