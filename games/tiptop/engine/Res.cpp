// TipTop's resource manager (see Res.h). The original allocated from its own memory pools (0x43b400) and could
// load a resource on a thread (entry flag 0x40, never set in TipTop's table); the port uses malloc and loads
// on the caller's thread.

#include "Res.h"
#include "platform/VFS.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#include <stb_image.h>

namespace res
{

static Entry sTable[] = {
#include "ResTable.inc"
	{0, nullptr, 0, 0, 0},
};

static Entry* gTable = nullptr;        // 0x48a2d4
static std::string gDataFile;          // 0x48a2b4
static uint8_t* gArchiveTable = nullptr;  // 0x48a2cc

Entry* GameTable()
{
	return sTable;
}

// 0x439c20
void SetTable(Entry* theTable)
{
	gTable = theTable;
}

// 0x439c00
void SetDataFile(int theSource, const char* theName)
{
	if (theSource == 0)
		gDataFile = theName;
}

// 0x438600: the archive's bit order to the real one
static void Unscramble(uint8_t* theData, uint32_t theSize)
{
	for (; theSize != 0; theSize--)
	{
		uint8_t b = *theData;
		*theData = (uint8_t)((b & 0x20) >> 1 | (b & 0x10) << 1 | (b & 0x08) << 4 | (b & 0x80) >> 4 |
			(b >> 5 & 2) | (b & 2) << 5 | (b >> 2 & 1) | (b & 1) << 2);
		theData++;
	}
}

// 0x438670: names compare without case
static bool SameName(const char* a, const char* b)
{
	while (toupper((uint8_t)*a) == toupper((uint8_t)*b))
	{
		if (*a == 0)
			return true;
		a++;
		b++;
	}
	return false;
}

// 0x430e70: theSize bytes at theOffset of a file into a new buffer
static uint32_t ReadFileRegion(const std::string& theFile, uint32_t theOffset, uint32_t theSize, uint8_t** theOut)
{
	*theOut = nullptr;
	FILE* aFile = Platform::VFS::Get().Open(theFile, "rb");
	if (aFile == nullptr)
		return 0;
	uint8_t* aBuffer = (uint8_t*)malloc(theSize != 0 ? theSize : 1);
	uint32_t aRead = 0;
	if (aBuffer != nullptr && fseek(aFile, (long)theOffset, SEEK_SET) == 0)
		aRead = (uint32_t)fread(aBuffer, 1, theSize, aFile);
	fclose(aFile);
	if (aRead != theSize)
	{
		free(aBuffer);
		return 0;
	}
	*theOut = aBuffer;
	return aRead;
}

// 0x438830: a resource's bytes from its source (3: the data file's table, read and unscrambled once)
static uint8_t* ReadSource(const char* theName, int32_t* theSize, uint8_t theSource)
{
	if (theSource != 3)
	{
		*theSize = 0;
		return nullptr;
	}
	if (gArchiveTable == nullptr)
	{
		FILE* aFile = Platform::VFS::Get().Open(gDataFile, "rb");
		uint32_t aTableSize = 0;
		if (aFile == nullptr || fread(&aTableSize, 1, 4, aFile) != 4)
		{
			if (aFile != nullptr)
				fclose(aFile);
			*theSize = 0;
			return nullptr;
		}
		Unscramble((uint8_t*)&aTableSize, 4);
		gArchiveTable = (uint8_t*)calloc(aTableSize + 1, 1);
		if (gArchiveTable == nullptr || fread(gArchiveTable, 1, aTableSize, aFile) != aTableSize)
		{
			free(gArchiveTable);
			gArchiveTable = nullptr;
			fclose(aFile);
			*theSize = 0;
			return nullptr;
		}
		Unscramble(gArchiveTable, aTableSize);
		fclose(aFile);
	}
	const uint8_t* p = gArchiveTable;
	if (*p == 0)
	{
		*theSize = 0;
		return nullptr;
	}
	while (!SameName((const char*)p, theName))
	{
		p += strlen((const char*)p) + 1 + 8;
		if (*p == 0)
		{
			*theSize = 0;
			return nullptr;
		}
	}
	const uint8_t* anInfo = p + strlen((const char*)p) + 1;
	uint32_t anOffset, aSize;
	memcpy(&anOffset, anInfo, 4);
	memcpy(&aSize, anInfo + 4, 4);
	uint8_t* aData = nullptr;
	*theSize = (int32_t)ReadFileRegion(gDataFile, anOffset, aSize, &aData);
	if (aData != nullptr)
		Unscramble(aData, (uint32_t)*theSize);
	return aData;
}

static lowos::Image* NewImage(uint32_t theBytes)
{
	lowos::Image* anImage = (lowos::Image*)calloc(1, sizeof(lowos::Image) + theBytes);
	if (anImage != nullptr)
		anImage->mBits = (uint8_t*)(anImage + 1);
	return anImage;
}

void FreeImage(lowos::Image* theImage)
{
	free(theImage);
}

static uint16_t ReadU16(const uint8_t* p)
{
	return (uint16_t)(p[0] | p[1] << 8);
}

// 0x438ab0: a PCX file (8 bits with its palette, or 24 bits in three planes) to an 8-bit or 24-bit RGB image
static lowos::Image* DecodePcx(const uint8_t* theFile, uint32_t theSize)
{
	if (theSize < 0x80)
		return nullptr;
	uint8_t aPlanes = theFile[0x41];
	uint16_t aBytesPerLine = ReadU16(theFile + 0x42);
	uint32_t aWidth = (uint32_t)ReadU16(theFile + 8) - ReadU16(theFile + 4) + 1;
	int aPitch = (int)(aWidth * aPlanes);
	int aHeight = ReadU16(theFile + 10) - ReadU16(theFile + 6) + 1;
	int aBytes = aHeight * aPitch;
	if (aBytes == 0)
		return nullptr;
	lowos::Image* anImage = NewImage((aPlanes != 1 ? 0 : 0x300) + aBytes);
	if (anImage == nullptr)
		return nullptr;
	if (aPlanes == 1)
	{
		anImage->mBytesPerPixel = 1;
		anImage->mPalette = anImage->mBits + aBytes;
	}
	else
	{
		if (theFile[3] > 8 || aPlanes != 3)
		{
			free(anImage);
			return nullptr;
		}
		anImage->mFormat = {8, 0, 8, 8, 8, 16};
		anImage->mBytesPerPixel = 3;
	}
	anImage->mPitch = (uint32_t)aPitch;
	anImage->mHeight = (uint32_t)aHeight;
	anImage->mWidth = aWidth;
	uint32_t aLineSize = (uint32_t)aBytesPerLine * aPlanes;
	uint8_t* aLine = (uint8_t*)malloc(aLineSize != 0 ? aLineSize : 1);
	if (aLine == nullptr)
	{
		free(anImage);
		return nullptr;
	}
	uint8_t* anOut = anImage->mBits;
	const uint8_t* p = theFile + 0x80;
	const uint8_t* anEnd = theFile + theSize;
	for (; aHeight != 0; aHeight--)
	{
		uint8_t* d = aLine;
		int aLeft = (int)aLineSize;
		while (aLeft > 0 && p < anEnd)
		{
			uint8_t b = *p;
			if ((b & 0xc0) == 0xc0)
			{
				int aCount = b & 0x3f;
				aLeft -= aCount;
				// a run past the line's end (none in TipTop's files) would overrun the original's buffer
				for (; aCount != 0; aCount--)
					if (d < aLine + aLineSize)
						*d++ = p + 1 < anEnd ? p[1] : 0;
				p += 2;
			}
			else
			{
				*d++ = b;
				p++;
				aLeft--;
			}
		}
		for (uint32_t x = 0; x < anImage->mWidth; x++)
		{
			const uint8_t* s = aLine + x;
			for (int aPlane = aPlanes; aPlane != 0; aPlane--)
			{
				*anOut++ = *s;
				s += aBytesPerLine;
			}
		}
	}
	free(aLine);
	if (aPlanes == 1)
	{
		if (p >= anEnd || *p != 0xc)
		{
			free(anImage);
			return nullptr;
		}
		for (int i = 0; i < 0x300; i++)
			*anOut++ = p + 1 + i < anEnd ? p[1 + i] : 0;
	}
	return anImage;
}

// The tail of 0x438d40 and 0x4391a0: a 24-bit RGB image to the display format
static lowos::Image* ToDisplayFormat(lowos::Image* theImage)
{
	lowos::PixelFormat aFormat;
	uint8_t aBpp = lowos::GetDisplayFormat(&aFormat, nullptr, nullptr);
	if (theImage->mBytesPerPixel != 3)
		return theImage;
	const lowos::PixelFormat aRgb = {8, 0, 8, 8, 8, 16};
	if (memcmp(&aFormat, &aRgb, sizeof(aFormat)) == 0 && aBpp == 3)
		return theImage;
	lowos::Image* aConverted = NewImage(theImage->mHeight * theImage->mWidth * aBpp);
	if (aConverted == nullptr)
	{
		free(theImage);
		return nullptr;
	}
	aConverted->mFormat = aFormat;
	aConverted->mBytesPerPixel = aBpp;
	aConverted->mPitch = theImage->mWidth * aBpp;
	aConverted->mHeight = theImage->mHeight;
	aConverted->mWidth = theImage->mWidth;
	aConverted->mPalette = nullptr;
	lowos::ConvertBitmap(&theImage->mFormat, &aConverted->mFormat, aConverted->mBits, theImage->mBits,
		(int)theImage->mHeight, (int)theImage->mWidth, aConverted->mPitch, theImage->mPitch,
		theImage->mBytesPerPixel, aBpp);
	free(theImage);
	return aConverted;
}

// 0x438d40: a PCX to the display format (8-bit images through their palette)
lowos::Image* ConvertPcx(const uint8_t* theFile, uint32_t theSize)
{
	if (theSize < 0x80)
		return nullptr;
	lowos::Image* aDecoded = DecodePcx(theFile, theSize);
	if (aDecoded == nullptr)
		return nullptr;
	lowos::Image* anImage = aDecoded;
	if (aDecoded->mBytesPerPixel == 1)
	{
		anImage = NewImage(aDecoded->mHeight * aDecoded->mWidth * 3);
		if (anImage == nullptr)
		{
			free(aDecoded);
			return nullptr;
		}
		anImage->mFormat = {8, 0, 8, 8, 8, 16};
		anImage->mPitch = aDecoded->mWidth * 3;
		anImage->mHeight = aDecoded->mHeight;
		anImage->mBytesPerPixel = 3;
		anImage->mWidth = aDecoded->mWidth;
		anImage->mPalette = nullptr;
		for (uint32_t y = 0; y < aDecoded->mHeight; y++)
		{
			const uint8_t* s = aDecoded->mBits + aDecoded->mPitch * y;
			uint8_t* d = anImage->mBits + y * anImage->mPitch;
			for (uint32_t x = 0; x < aDecoded->mWidth; x++)
			{
				d[0] = aDecoded->mPalette[*s * 3];
				d[1] = aDecoded->mPalette[*s * 3 + 1];
				d[2] = aDecoded->mPalette[*s * 3 + 2];
				s++;
				d += 3;
			}
		}
		free(aDecoded);
	}
	return ToDisplayFormat(anImage);
}

// 0x4391a0: a JPEG to the display format. The original decoded with the IJG library (0x438f90) into 24-bit RGB;
// stb_image decodes the same files (its IDCT and upsampling may differ from IJG's by a unit here and there).
lowos::Image* ConvertJpeg(const uint8_t* theFile, uint32_t theSize)
{
	if (theSize < 0x80)
		return nullptr;
	int aWidth = 0, aHeight = 0, aComponents = 0;
	uint8_t* aPixels = stbi_load_from_memory(theFile, (int)theSize, &aWidth, &aHeight, &aComponents, 3);
	if (aPixels == nullptr)
		return nullptr;
	lowos::Image* anImage = NewImage((uint32_t)(aWidth * aHeight * 3));
	if (anImage == nullptr)
	{
		stbi_image_free(aPixels);
		return nullptr;
	}
	anImage->mFormat = {8, 0, 8, 8, 8, 16};
	anImage->mPitch = (uint32_t)aWidth * 3;
	anImage->mWidth = (uint32_t)aWidth;
	anImage->mHeight = (uint32_t)aHeight;
	anImage->mBytesPerPixel = 3;
	anImage->mPalette = nullptr;
	memcpy(anImage->mBits, aPixels, (size_t)aWidth * aHeight * 3);
	stbi_image_free(aPixels);
	return ToDisplayFormat(anImage);
}

// 0x438a70. port: two terminators, not one: the tips dialog (0x420ea9) goes on reading one past the end of a text
// that ends with its last word.
static char* ConvertText(const uint8_t* theData, uint32_t theSize)
{
	char* aText = (char*)malloc(theSize + 2);
	if (aText == nullptr)
		return nullptr;
	memcpy(aText, theData, theSize);
	aText[theSize] = 0;
	aText[theSize + 1] = 0;
	return aText;
}

// 0x4395a0: an Ogg Vorbis file to a WAV image of 16-bit samples. The original read it with libvorbisfile
// (ov_read, a 'fmt '/'data' pair per logical stream, its RIFF size 0xc short); TipTop's sounds are one stream each.
static uint8_t* ConvertOgg(const uint8_t* theData, uint32_t theSize)
{
	int aChannels = 0, aRate = 0;
	short* aPcm = nullptr;
	int aFrames = stb_vorbis_decode_memory(theData, (int)theSize, &aChannels, &aRate, &aPcm);
	if (aFrames <= 0 || aPcm == nullptr)
	{
		free(aPcm);
		return nullptr;
	}
	uint32_t aBytes = (uint32_t)aFrames * aChannels * 2;
	uint8_t* aWav = (uint8_t*)malloc(sizeof(WavHeader) + aBytes);
	if (aWav == nullptr)
	{
		free(aPcm);
		return nullptr;
	}
	WavHeader* aHeader = (WavHeader*)aWav;
	aHeader->mRiff = 0x46464952;
	aHeader->mRiffSize = (uint32_t)sizeof(WavHeader) + aBytes - 0xc;
	aHeader->mWave = 0x45564157;
	aHeader->mFmt = 0x20746d66;
	aHeader->mFmtSize = 0x10;
	aHeader->mFormat = 1;
	aHeader->mChannels = (uint16_t)aChannels;
	aHeader->mRate = (uint32_t)aRate;
	aHeader->mByteRate = (uint32_t)aChannels * aRate * 2;
	aHeader->mBitsPerSample = 0x10;
	aHeader->mBlockAlign = (uint16_t)(aChannels << 1);
	aHeader->mData = 0x61746164;
	aHeader->mDataSize = aBytes;
	memcpy(aWav + sizeof(WavHeader), aPcm, aBytes);
	free(aPcm);
	return aWav;
}

// 0x4386d0: the converter of a type; returns theData itself for raw resources
static void* Convert(uint8_t* theData, uint32_t theSize, uint32_t theType)
{
	switch (theType)
	{
	case TYPE_PCX:
		return ConvertPcx(theData, theSize);
	case TYPE_JPEG:
		return ConvertJpeg(theData, theSize);
	case TYPE_TEXT:
		return ConvertText(theData, theSize);
	case TYPE_OGG:
		return ConvertOgg(theData, theSize);
	default:
		return theData;
	}
}

// 0x439870: reads and converts a resource that is not loaded yet. Returns 1 on failure.
static char LoadEntry(Entry* theEntry)
{
	if (theEntry->mHandle == nullptr)
	{
		uint8_t* aData = ReadSource(theEntry->mName, &theEntry->mSize, theEntry->mFlags & 0x3f);
		theEntry->mHandle = aData;
		if (theEntry->mSize == 0)
		{
			free(aData);
			theEntry->mHandle = nullptr;
			theEntry->mRefCount--;
			return 1;
		}
		void* aHandle = Convert(aData, (uint32_t)theEntry->mSize, theEntry->mType);
		theEntry->mHandle = aHandle;
		if (aHandle == nullptr)
		{
			free(aData);
			theEntry->mRefCount--;
			return 1;
		}
		if (aHandle != aData)
			free(aData);
	}
	return 0;
}

// 0x4397d0: references a resource and loads it the first time
static char AddRef(Entry* theEntry)
{
	if (theEntry->mSize == -1)
		return 1;
	theEntry->mRefCount++;
	return LoadEntry(theEntry);
}

// 0x4399a0: drops a reference; the last one frees the resource
static void Release(Entry* theEntry)
{
	if (theEntry->mRefCount != 0 && --theEntry->mRefCount == 0)
	{
		if (theEntry->mHandle != nullptr)
		{
			free(theEntry->mHandle);
			theEntry->mHandle = nullptr;
		}
	}
}

// 0x439b30
char Load(int theGroup, int theCount, ...)
{
	va_list anArgs;
	va_start(anArgs, theCount);
	for (int i = 0; i < (theCount & 0xff); i++)
	{
		uint32_t anId = va_arg(anArgs, uint32_t);
		for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		{
			if (e->mId == anId && e->mGroup == (uint32_t)theGroup)
			{
				if (AddRef(e) != 0)
				{
					va_end(anArgs);
					return 1;
				}
				break;
			}
		}
	}
	va_end(anArgs);
	return 0;
}

// 0x439ba0
void Free(int theGroup, int theCount, ...)
{
	va_list anArgs;
	va_start(anArgs, theCount);
	for (int i = theCount & 0xff; i != 0; i--)
	{
		uint32_t anId = va_arg(anArgs, uint32_t);
		for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		{
			if (e->mId == anId && e->mGroup == (uint32_t)theGroup)
			{
				Release(e);
				break;
			}
		}
	}
	va_end(anArgs);
}

// 0x439a40: marks the group, unmarks the ids asked for, frees the marked ones and loads the others
char KeepOnly(int theGroup, int theCount, ...)
{
	char aFailed = 0;
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		if (e->mGroup == (uint32_t)theGroup)
			e->mFlags |= 0x80;
	va_list anArgs;
	va_start(anArgs, theCount);
	for (int i = theCount & 0xff; i != 0; i--)
	{
		uint32_t anId = va_arg(anArgs, uint32_t);
		for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		{
			if (e->mId == anId && e->mGroup == (uint32_t)theGroup)
			{
				e->mFlags &= 0x7f;
				break;
			}
		}
	}
	va_end(anArgs);
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
	{
		if (e->mGroup == (uint32_t)theGroup && (e->mFlags & 0x80) != 0)
		{
			e->mRefCount = 1;
			Release(e);
		}
	}
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
	{
		if (e->mGroup == (uint32_t)theGroup)
		{
			if ((e->mFlags & 0x80) == 0)
			{
				e->mRefCount = 0;
				aFailed |= AddRef(e);
			}
			else
				e->mFlags &= 0x7f;
		}
	}
	return aFailed;
}

// 0x439ca0
void* Get(int theGroup, int theId)
{
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		if (e->mId == (uint32_t)theId && e->mGroup == (uint32_t)theGroup)
			return e->mHandle;
	return nullptr;
}

// 0x439d70
int IndexOf(int theGroup, int theId)
{
	int anIndex = 0;
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++, anIndex++)
		if (e->mId == (uint32_t)theId && e->mGroup == (uint32_t)theGroup)
			return anIndex;
	return 0;
}

int32_t SizeOf(int theGroup, int theId)
{
	for (Entry* e = gTable; (e->mFlags & 0x3f) != 0; e++)
		if (e->mId == (uint32_t)theId && e->mGroup == (uint32_t)theGroup)
			return e->mSize;
	return 0;
}

} // namespace res
