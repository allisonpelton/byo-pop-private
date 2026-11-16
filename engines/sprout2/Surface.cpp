#include "Surface.h"

#include "Engine.h"
#include "Stream.h"

#include "platform/ImageLoader.h"

#include <cstring>

namespace engine
{

namespace
{

// The rest of a stream read into memory: the port decodes whole files where the original's libjpeg and libpng read
// the stream as they go
std::vector<uint8_t> readAll(InputStream* theStream)
{
	std::vector<uint8_t> aData;
	uint8_t aBuffer[16384];
	for (;;)
	{
		int aCount = theStream->read(aBuffer, sizeof(aBuffer));
		if (aCount <= 0)
			break;
		aData.insert(aData.end(), aBuffer, aBuffer + aCount);
	}
	return aData;
}

// 0x46e320 (feedingfrenzy2): the string ends with the other, compared as written
bool endsWith(const std::string& theString, const char* theEnd)
{
	size_t aLength = strlen(theEnd);
	return theString.size() >= aLength && theString.compare(theString.size() - aLength, aLength, theEnd) == 0;
}

// What libpng's info says of a PNG: IHDR's bit depth and colour type (0 grey, 2 RGB, 3 palette, 4 grey and alpha,
// 6 RGBA), and whether a tRNS chunk comes before the image data
struct PngHeader
{
	int mBitDepth = 0;
	int mColorType = 0;
	bool mHasTransparency = false;
};

PngHeader readPngHeader(const std::vector<uint8_t>& theData)
{
	PngHeader aHeader;
	if (theData.size() < 33)
		return aHeader;
	aHeader.mBitDepth = theData[24];
	aHeader.mColorType = theData[25];
	for (size_t aPos = 8; aPos + 8 <= theData.size();)
	{
		size_t aLength = ((size_t)theData[aPos] << 24) | ((size_t)theData[aPos + 1] << 16) | ((size_t)theData[aPos + 2] << 8) |
			theData[aPos + 3];
		const char* aType = (const char*)&theData[aPos + 4];
		if (memcmp(aType, "IDAT", 4) == 0)
			break;
		if (memcmp(aType, "tRNS", 4) == 0)
			aHeader.mHasTransparency = true;
		aPos += 12 + aLength;
	}
	return aHeader;
}

// _splitpath's directory (with its last slash, without a drive) and name (without the extension)
void splitPath(const std::string& thePath, std::string& theDirectory, std::string& theName)
{
	size_t aSlash = thePath.find_last_of("/\\");
	size_t aNameStart = aSlash == std::string::npos ? 0 : aSlash + 1;
	size_t aDirectoryStart = thePath.size() >= 2 && thePath[1] == ':' ? 2 : 0;
	if (aDirectoryStart > aNameStart)
		aDirectoryStart = aNameStart;
	theDirectory = thePath.substr(aDirectoryStart, aNameStart - aDirectoryStart);
	size_t aDot = thePath.find_last_of('.');
	if (aDot == std::string::npos || aDot < aNameStart)
		aDot = thePath.size();
	theName = thePath.substr(aNameStart, aDot - aNameStart);
}

} // namespace

Surface::Surface() : mBlendMode(0), mBlendFlag(false), mAnchor(0, 0), mWidth(0), mHeight(0)
{
}

void Surface::allocate(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;
	mPixels.assign((size_t)theWidth * theHeight, 0);
}

Surface::~Surface()
{
}

void Surface::load(const std::string& thePath, bool theFromDisk)
{
	setName(thePath);
	std::string aDirectory;
	std::string aName;
	splitPath(thePath, aDirectory, aName);
	std::string aMaskPath = aDirectory + "_" + aName + ".png";
	Ref<InputStream> aStream;
	bool aHasMask;
	if (!theFromDisk)
	{
		aStream = getEngine()->openFile(thePath);
		aHasMask = getEngine()->fileExists(aMaskPath);
	}
	else
	{
		aStream = getEngine()->openDiskFile(thePath);
		aHasMask = getEngine()->diskFileExists(aMaskPath);
	}
	if (!aStream)
		return;
	std::vector<uint8_t> aData = readAll(aStream);
	if (endsWith(thePath, ".jpg"))
	{
		// 0x4791f0 and 0x479dc0 (feedingfrenzy2): libjpeg 6b with its defaults, read as RGB
		int aWidth = 0;
		int aHeight = 0;
		int aComponents = 0;
		std::vector<uint8_t> aPixels;
		Platform::DecodeJPEG(aData.data(), aData.size(), aWidth, aHeight, aComponents, aPixels);
		allocate(aWidth, aHeight);
		setBlendMode(BLEND_OPAQUE, false);
		for (size_t anIndex = 0; anIndex < mPixels.size(); anIndex++)
		{
			const uint8_t* aSource = &aPixels[anIndex * aComponents];
			uint32_t aRed = aSource[0];
			uint32_t aGreen = aComponents == 3 ? aSource[1] : aSource[0];
			uint32_t aBlue = aComponents == 3 ? aSource[2] : aSource[0];
			mPixels[anIndex] = 0xff000000 | (aRed << 16) | (aGreen << 8) | aBlue;
		}
	}
	else if (endsWith(thePath, ".png"))
	{
		// 0x463410, 0x466160 and 0x537c57 (feedingfrenzy2): libpng with the palette and low depths expanded, 16 bits
		// cut to 8 and grey made RGB, which is how stb_image reads the game's images
		int aWidth = 0;
		int aHeight = 0;
		std::vector<uint32_t> aPixels;
		Platform::DecodeImage(aData, aWidth, aHeight, aPixels, nullptr);
		allocate(aWidth, aHeight);
		if (!aHasMask)
		{
			// 0x41a690 (feedingfrenzy2): blended when the colour type has an alpha channel
			setBlendMode((readPngHeader(aData).mColorType & 4) ? BLEND_ALPHA : BLEND_OPAQUE, false);
			mPixels = aPixels;
		}
		else
		{
			// The colours alone (0x4ff1e4 (feedingfrenzy2)): the mask gives the alpha
			for (size_t anIndex = 0; anIndex < mPixels.size(); anIndex++)
				mPixels[anIndex] = (mPixels[anIndex] & 0xff000000) | (aPixels[anIndex] & 0x00ffffff);
		}
	}
	aStream = nullptr;
	if (aHasMask)
		loadMask(aMaskPath, theFromDisk, true);
	else if (getBlendMode() == BLEND_ALPHA)
		premultiplyAlpha();
}

void Surface::loadMask(const std::string& thePath, bool theFromDisk, bool thePremultiply)
{
	Ref<InputStream> aStream;
	if (!theFromDisk)
		aStream = getEngine()->openFile(thePath);
	else
		aStream = getEngine()->openDiskFile(thePath);
	// The original reads the mask without checking that it opened; its callers have checked that it is there
	std::vector<uint8_t> aData;
	if (aStream)
		aData = readAll(aStream);
	// 0x464550 (feedingfrenzy2): libpng makes the mask grey, expanding a palette and the low depths (and with them a
	// tRNS chunk into alpha), cutting 16 bits to 8 and turning RGB into grey by libpng's default weights; the alpha is
	// the last channel of what is left. (The game's colour masks are grey, so the weights give their value whatever
	// libpng's gamma tables would do to other colours.)
	PngHeader aHeader = readPngHeader(aData);
	bool anExpands = aHeader.mColorType == 3 || aHeader.mBitDepth < 8;
	bool aHasAlphaChannel = (aHeader.mColorType & 4) != 0 || (anExpands && aHeader.mHasTransparency);
	int aWidth = 0;
	int aHeight = 0;
	std::vector<uint32_t> aPixels;
	Platform::DecodeImage(aData, aWidth, aHeight, aPixels, nullptr);
	// The original reads a mask row for each of the surface's rows and an entry for each of its pixels, whatever the
	// mask's size; the port stops at the smaller of the two
	int aCopyWidth = minOf(mWidth, aWidth);
	int aCopyHeight = minOf(mHeight, aHeight);
	for (int aY = 0; aY < aCopyHeight; aY++)
	{
		for (int aX = 0; aX < aCopyWidth; aX++)
		{
			uint32_t aMaskPixel = aPixels[(size_t)aY * aWidth + aX];
			uint32_t anAlpha;
			if (aHasAlphaChannel)
				anAlpha = aMaskPixel >> 24;
			else
			{
				uint32_t aRed = (aMaskPixel >> 16) & 0xff;
				uint32_t aGreen = (aMaskPixel >> 8) & 0xff;
				uint32_t aBlue = aMaskPixel & 0xff;
				anAlpha = (6968 * aRed + 23434 * aGreen + 2366 * aBlue) >> 15;
			}
			uint32_t& aPixel = mPixels[(size_t)aY * mWidth + aX];
			aPixel = (aPixel & 0x00ffffff) | (anAlpha << 24);
		}
	}
	if (thePremultiply)
		premultiplyAlpha();
	setBlendMode(BLEND_ALPHA, false);
}

void Surface::premultiplyAlpha()
{
	for (uint32_t& aPixel : mPixels)
	{
		uint32_t anAlpha = aPixel >> 24;
		uint32_t aResult = aPixel & 0xff000000;
		for (int aShift = 0; aShift < 24; aShift += 8)
		{
			uint32_t aProduct = ((aPixel >> aShift) & 0xff) * anAlpha + 0x80;
			aResult |= (((aProduct + (aProduct >> 8)) >> 8) & 0xff) << aShift;
		}
		aPixel = aResult;
	}
}

void Surface::fill(const Color& theColor)
{
	// The x87 multiplies at double precision and truncates; a channel above 1 keeps its low byte
	uint32_t aRed = (uint32_t)(int)((double)theColor.r * 255.0) & 0xff;
	uint32_t aGreen = (uint32_t)(int)((double)theColor.g * 255.0) & 0xff;
	uint32_t aBlue = (uint32_t)(int)((double)theColor.b * 255.0) & 0xff;
	uint32_t anAlpha = (uint32_t)(int)((double)theColor.a * 255.0) & 0xff;
	uint32_t aPixel = anAlpha << 24 | aRed << 16 | aGreen << 8 | aBlue;
	for (uint32_t& aTarget : mPixels)
		aTarget = aPixel;
}

} // namespace engine
