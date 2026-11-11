// HDisplay, its surfaces, blend modes, fonts and image loaders (see HDisplay.h).

#include "HDisplay.h"
#include "HFileSystem.h"
#include "PGSprite.h"

#include <cstring>
#include <stb_image.h>
#include "platform/ImageLoader.h"

namespace pg
{

static HDisplay* gScreenDisplay = nullptr;

HDisplay* GetScreenDisplay()
{
	return gScreenDisplay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Images

void HImage::create(int theWidth, int theHeight, int theFormat)
{
	int aBytes = 0;
	if (theFormat == 1)
		aBytes = 1;
	else if (theFormat == 2)
		aBytes = 4;
	else if (theFormat == 3)
		aBytes = 2;
	mFormat = theFormat;
	mWidth = theWidth;
	mHeight = theHeight;
	mBits.assign((size_t)aBytes * theWidth * theHeight, 0);
}

int HImage::bytesPerPixel() const
{
	if (mFormat == 1)
		return 1;
	if (mFormat == 2)
		return 4;
	if (mFormat == 3)
		return 2;
	PGAlert("getBytesPerPixel - unsupported format '%d'", mFormat);
	return 0;
}

void HImage::flipVertical()
{
	int aPitch = pitch();
	int aHalf = mHeight / 2;
	for (int y = 0; y < aHalf; y++)
	{
		uint8_t* a = mBits.data() + y * aPitch;
		uint8_t* b = mBits.data() + (mHeight - 1 - y) * aPitch;
		if (a == b)
			return;
		for (int i = 0; i < aPitch; i++)
		{
			uint8_t t = b[i];
			b[i] = a[i];
			a[i] = t;
		}
	}
}

// 0x4282e0: uncompressed Targa (the game's are all type 2, bottom-up). 16 bits a pixel are repacked (the alpha bit
// dropped), 24 and 32 kept as blue, green, red and alpha, any other type read as 8 bits.
static bool LoadTarga(const std::vector<uint8_t>& theFile, HImage& theImage)
{
	if (theFile.size() < 18)
		return false;
	const uint8_t* a1 = theFile.data();
	uint8_t aType = a1[2];
	int aWidth = (int16_t)(a1[0xc] | (a1[0xd] << 8));
	int aHeight = (int16_t)(a1[0xe] | (a1[0xf] << 8));
	uint8_t aBits = a1[0x10];
	int aStep;      // bytes a pixel in the image
	int aSrcStep;   // bytes a pixel in the file
	if (aType == 2)
	{
		if (aBits < 0x11)
		{
			theImage.create(aWidth, aHeight, 3);
			aStep = 2;
		}
		else
		{
			theImage.create(aWidth, aHeight, 2);
			aStep = 4;
		}
		aSrcStep = aBits >> 3;
	}
	else
	{
		theImage.create(aWidth, aHeight, 1);
		aSrcStep = 1;
		aStep = 1;
	}
	// the pixels follow the 18-byte header (the game's files have no id field and no colour map)
	const uint8_t* aSrc = a1 + 0x12;
	const uint8_t* aSrcEnd = a1 + theFile.size();
	uint8_t* aDest = theImage.mBits.data();
	int aSize = theImage.pitch() * theImage.mHeight;
	for (int i = 0; i < aSize; i += aStep)
	{
		if (aSrc + aSrcStep > aSrcEnd)
			break;
		if (aSrcStep == 1)
			*aDest = *aSrc;
		else if (aSrcStep == 2)
		{
			uint16_t v = (uint16_t)(aSrc[0] | (aSrc[1] << 8));
			uint16_t p = (uint16_t)((((v & 0x3e0) | (v << 10)) * 2) | ((v & 0x7c00) >> 9));
			aDest[0] = (uint8_t)p;
			aDest[1] = (uint8_t)(p >> 8);
		}
		else if (aSrcStep == 3)
		{
			aDest[0] = aSrc[0];
			aDest[1] = aSrc[1];
			aDest[2] = aSrc[2];
			aDest[3] = 0xff;
		}
		else if (aSrcStep == 4)
		{
			aDest[0] = aSrc[0];
			aDest[1] = aSrc[1];
			aDest[2] = aSrc[2];
			aDest[3] = aSrc[3];
		}
		aDest += aStep;
		aSrc += aSrcStep;
	}
	theImage.flipVertical();
	return true;
}

// 0x428960 (libjpeg 6b) and 0x428590 (libpng 1.2.2): port: Platform::LoadImageFromMemory decodes both, a JPEG through
// libjpeg 6b as the original does, so its pixels are the original's, and a PNG through stb_image. The pixels are stored as
// 0x428070 packs them (blue, green, red, alpha in memory).
static bool LoadStb(const std::vector<uint8_t>& theFile, HImage& theImage, bool theAlpha)
{
	int aWidth, aHeight, aComp;
	stbi_uc* aPixels = Platform::LoadImageFromMemory(theFile.data(), (int)theFile.size(), &aWidth, &aHeight, &aComp,
		theAlpha ? 4 : 3);
	if (aPixels == nullptr)
		return false;
	theImage.create(aWidth, aHeight, 2);
	int aStep = theAlpha ? 4 : 3;
	const stbi_uc* s = aPixels;
	uint8_t* d = theImage.mBits.data();
	for (int i = 0; i < aWidth * aHeight; i++, s += aStep, d += 4)
	{
		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
		d[3] = theAlpha ? s[3] : 0xff;
	}
	stbi_image_free(aPixels);
	return true;
}

static bool HasExtension(const char* theName, const char* theExt)
{
	return strstr(theName, theExt) != nullptr;
}

// 0x428180
// 0x428140, 0x428170, 0x428280, 0x428290, 0x4282b0 and 0x428950 are the loader objects' constructor, getters and release
bool LoadImage(const HFileSystem* theFileSystem, const char* theName, HImage& theImage)
{
	int aKind = 0;
	if (HasExtension(theName, ".tga"))
		aKind = 1;
	if (HasExtension(theName, ".jpg") || HasExtension(theName, ".jpeg"))
		aKind = 2;
	if (HasExtension(theName, ".png"))
		aKind = 3;
	if (aKind == 0)
		return false;
	std::vector<uint8_t> aFile;
	if (!theFileSystem->readFile(theName, aFile))
		return false;
	if (aKind == 1)
		return LoadTarga(aFile, theImage);
	return LoadStb(aFile, theImage, aKind == 3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Surfaces

// 0x423e80
HSurface::~HSurface()
{
	// 0x423e80
	delete[] mPixels;
	delete[] mAlpha;
}

// 0x424810: a 32-bit image is dithered down to 15 bits with a 2x2 ordered dither (0, 2 / 3, 1 added to each
// channel's top 7 bits); a pixel that is not opaque, or of the key colour (255, 0, 255), gives the surface an alpha
// plane (the key colour's alpha 0). A 16-bit image's pixels are repacked, its key colour 0x7c1f transparent.
// 0x427fd0 and 0x428010 read a pixel of the image
HSurface* HDisplay::createSurface(const HImage& theImage)
{
	HSurface* aSurface = new HSurface();
	aSurface->mWidth = theImage.mWidth;
	aSurface->mHeight = theImage.mHeight;
	aSurface->mBpp = 0x10;
	int aCount = aSurface->mHeight * aSurface->mWidth;
	uint8_t* a1 = new uint8_t[aCount > 0 ? aCount : 1];
	memset(a1, 0xff, aCount);
	aSurface->mPitch = aSurface->mWidth * (aSurface->mBpp / 8);
	aSurface->mPixels = new uint16_t[(aCount > 0) ? aCount : 1];
	memset(aSurface->mPixels, 0, (size_t)aCount * 2);
	bool aHasAlpha = false;
	int aWidth = theImage.mWidth;
	int aHeight = theImage.mHeight;
	if (aSurface->mBpp == 0x10)
	{
		if (theImage.bytesPerPixel() == 4)
		{
			static const short kDither[4] = {0, 2, 3, 1};
			uint8_t* aAlphaRow = a1;
			for (int y = 0; y < aHeight; y++)
			{
				int aDitherRow = (y & 1) * 2;
				for (int x = 0; x < aWidth; x++)
				{
					// 0x427fd0: red, green, blue, alpha
					const uint8_t* p = theImage.mBits.data() + theImage.pitch() * y + x * 4;
					uint8_t r = p[2], g = p[1], b = p[0], a = p[3];
					aAlphaRow[x] = a;
					if (a != 0xff)
					{
						aHasAlpha = true;
						aAlphaRow[x] = a;
					}
					if (r == 0xff && g == 0 && b == 0xff)
					{
						aHasAlpha = true;
						aAlphaRow[x] = 0;
					}
					short d = kDither[(x & 1) + aDitherRow];
					unsigned short aR = (unsigned short)((r >> 1) + d);
					if (aR > 0x7f)
						aR = 0x7f;
					aR >>= 2;
					unsigned short aG = (unsigned short)((g >> 1) + d);
					if (aG > 0x7f)
						aG = 0x7f;
					aG >>= 2;
					unsigned short aB = (unsigned short)((b >> 1) + d);
					if (aB > 0x7f)
						aB = 0x7f;
					*(uint16_t*)((uint8_t*)aSurface->mPixels + aSurface->mPitch * y + x * 2) =
						(uint16_t)((((aR << 5) | aG) << 5) | (aB >> 2));
				}
				aAlphaRow += aWidth;
			}
		}
		else if (theImage.bytesPerPixel() == 2)
		{
			uint8_t* aAlphaRow = a1;
			for (int y = 0; y < aHeight; y++)
			{
				for (int x = 0; x < aWidth; x++)
				{
					// 0x428010
					const uint8_t* p = theImage.mBits.data() + (theImage.mWidth * y + x) * 2;
					uint16_t v = (uint16_t)(p[0] | (p[1] << 8));
					uint16_t aPixel = (uint16_t)((((v & 0x7c0) | (v >> 10)) >> 1) | ((v & 0x3e) << 9));
					*(uint16_t*)((uint8_t*)aSurface->mPixels + aSurface->mPitch * y + x * 2) = aPixel;
					if (aPixel == 0x7c1f)
					{
						aHasAlpha = true;
						aAlphaRow[x] = 0;
					}
				}
				aAlphaRow += aWidth;
			}
		}
	}
	else
		PGAlert("Unimplemented");
	if (aHasAlpha)
		aSurface->mAlpha = a1;
	else
		delete[] a1;
	return aSurface;
}

// 0x423e60
void HDisplay::freeSurface(HSurface* theSurface)
{
	delete theSurface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Blending (0x4238f0..0x423b80): the pixel format and the current blend mode are globals in the original

static int gShiftR, gShiftG, gShiftB;     // 0x4c51dc, 0x4c5200, 0x4c51c8
static int gMaskR, gMaskG, gMaskB;        // 0x4c51d0, 0x4c51fc, 0x4c51cc
static uint8_t gConst[4];                 // 0x4c51d4: the constants A and B, then the flags of 0x423aa0
static int gSrc1, gSrc2, gSrcInvert;      // 0x4c51f4, 0x4c51ec, 0x4c51c0
static int gDest1, gDest2, gDestInvert;   // 0x4c51f8, 0x4c51f0, 0x4c51c4
static int gOp;                           // 0x46ca88
static int gFastPath;                     // 0x4c51e0: 1 plain alpha blending, 2 alpha tested at half

// 0x4238f0
static void SetPixelFormat(int theBitsR, int theBitsG, int theBitsB, int theShiftR, int theShiftG, int theShiftB)
{
	gShiftR = theShiftR;
	gShiftG = theShiftG;
	gShiftB = theShiftB;
	// the original also keeps the bit counts (0x4c51e4, 0x4c51e8, 0x4c51d8), which nothing reads
	gMaskR = (1 << theBitsR) - 1;
	gMaskG = (1 << theBitsG) - 1;
	gMaskB = (1 << theBitsB) - 1;
}

// 0x423960
static void SetBlendState(int s1, int s2, int s3, uint8_t s4, int d1, int d2, int d3, uint8_t d4, int theOp)
{
	gConst[0] = s4;
	gConst[1] = d4;
	gSrc1 = s1;
	gSrc2 = s2;
	gSrcInvert = s3;
	gDest1 = d1;
	gDest2 = d2;
	gDestInvert = d3;
	gOp = theOp;
	gFastPath = 0;
	if (s1 == 2 && s2 == 1 && s3 == 0 && d1 == 3 && d2 == 1 && d3 == 0 && theOp == 0)
		gFastPath = (gConst[2] != 0) + 1;
}

// 0x423a00: the first factor
static unsigned int Factor1(int theKind, unsigned int theAlpha)
{
	switch (theKind)
	{
	case 0:
		return 0;
	case 1:
		break;
	case 2:
		return theAlpha & 0xff;
	case 3:
		return 0xff - (theAlpha & 0xff);
	case 4:
		return gConst[0];
	case 5:
		return 0xff - gConst[0];
	case 6:
		return gConst[1];
	case 7:
		return 0xff - gConst[1];
	default:
		break;
	}
	return 0xff;
}

// 0x423b00: the second factor
static unsigned int Factor2(int theKind)
{
	switch (theKind)
	{
	case 0:
		return 0;
	case 1:
		break;
	case 2:
		return gConst[0];
	case 3:
		return 0xff - gConst[0];
	case 4:
		return gConst[1];
	case 5:
		return 0xff - gConst[1];
	default:
		break;
	}
	return 0xff;
}

// 0x423b80: one pixel
static unsigned int BlendPixel(uint16_t theSrc, uint16_t theDest, unsigned int theAlpha)
{
	if (gFastPath == 1)
	{
		if ((uint8_t)theAlpha == 0xff)
			return theSrc;
		if ((uint8_t)theAlpha == 0)
			return theDest;
	}
	else if (gFastPath == 2)
	{
		if ((uint8_t)theAlpha > 0x7f)
			return theSrc;
		return theDest;
	}
	unsigned int sR = theSrc >> gShiftR & gMaskR;
	unsigned int sG = theSrc >> gShiftG & gMaskG;
	unsigned int sB = theSrc >> gShiftB & gMaskB;
	unsigned int dR = theDest >> gShiftR & gMaskR;
	unsigned int dG = theDest >> gShiftG & gMaskG;
	unsigned int dB = theDest >> gShiftB & gMaskB;
	unsigned int f = Factor1(gSrc1, theAlpha);
	unsigned int f2 = Factor2(gSrc2);
	int k = (gSrcInvert == 1) ? (int)(0xfe01 - f2 * f) : (int)(f2 * f);
	sB = ((unsigned int)k * sB) / 0xfe01;
	sG = ((unsigned int)k * sG) / 0xfe01;
	sR = ((unsigned int)k * sR) / 0xfe01;
	f = Factor1(gDest1, theAlpha);
	f2 = Factor2(gDest2);
	unsigned int kd = (gDestInvert == 1) ? 0xfe01 - f2 * f : f2 * f;
	dR = (kd * dR) / 0xfe01;
	dG = (kd * dG) / 0xfe01;
	dB = (kd * dB) / 0xfe01;
	int r, g, b;
	switch (gOp)
	{
	case 0:
		r = (int)(sR + dR);
		g = (int)(sG + dG);
		b = (int)(dB + sB);
		break;
	case 1:
		r = (int)((dR * sR) / 0xff);
		g = (int)((dG * sG) / 0xff);
		b = (int)((dB * sB) / 0xff);
		break;
	case 2:
		r = (int)(sR - dR);
		g = (int)(sG - dG);
		b = (int)(sB - dB);
		break;
	case 3:
		r = (int)(dR - sR);
		g = (int)(dG - sG);
		b = (int)(dB - sB);
		break;
	default:
		r = g = b = (int)kd;
		break;
	}
	if (r < 0)
		r = 0;
	else if (gMaskR < r)
		r = gMaskR;
	if (g < 0)
		g = 0;
	else if (gMaskG < g)
		g = gMaskG;
	if (b < 0)
		b = 0;
	else if (gMaskB < b)
		b = gMaskB;
	return (unsigned int)(b << gShiftB | g << gShiftG | r << gShiftR);
}

// 0x422dc0
bool HDisplay::blend(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface, int s1, int s2, int s3,
	int s4, int d1, int d2, int d3, int d4, int theOp)
{
	if (theSurface->mAlpha == nullptr && (s1 == 2 || s1 == 3 || d1 == 2 || d1 == 3))
	{
		copy(theDest, theSrc, theSurface);
		return true;
	}
	if (theDest != nullptr)
		mDirty.add(*theDest);
	int aDestX = 0, aDestY = 0;
	if (theDest != nullptr)
	{
		aDestY = theDest->top;
		aDestX = theDest->left;
	}
	int aLeft = theSrc->left;
	int aTop = theSrc->top;
	int aBottom = theSrc->bottom;
	int aWidth = theSrc->right - aLeft;
	SetBlendState(s1, s2, s3, (uint8_t)s4, d1, d2, d3, (uint8_t)d4, theOp);
	int y = aTop;
	for (int aRows = aBottom - aTop; aRows > 0; aRows--, y++)
	{
		uint16_t* d = (uint16_t*)((uint8_t*)mScreen.data() + (aDestY - aTop + y) * mScreenPitch + aDestX * 2);
		const uint16_t* s = (const uint16_t*)((const uint8_t*)theSurface->mPixels + theSurface->mPitch * y + aLeft * 2);
		if (theSurface->mAlpha == nullptr)
		{
			for (int n = aWidth; n > 0; n--, d++, s++)
				*d = (uint16_t)BlendPixel(*s, *d, 0xff);
		}
		else
		{
			const uint8_t* a = theSurface->mAlpha + aLeft + theSurface->mWidth * y;
			for (int n = aWidth; n > 0; n--, d++, s++, a++)
			{
				if (*a != 0)
					*d = (uint16_t)BlendPixel(*s, *d, *a);
			}
		}
	}
	return true;
}

// 0x422fb0, with 0x422ca0 (only the opaque pixels of a surface with an alpha plane) and 0x422be0 (the rows)
bool HDisplay::copy(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface)
{
	if (theDest != nullptr)
		mDirty.add(*theDest);
	int aDestX = 0, aDestY = 0;
	if (theDest != nullptr)
	{
		aDestY = theDest->top;
		aDestX = theDest->left;
	}
	int aLeft = theSrc->left;
	int aRight = theSrc->right;
	int aTop = theSrc->top;
	int aRows = theSrc->bottom - aTop;
	if (theSurface->mAlpha != nullptr)
	{
		int y = aTop - aDestY;
		for (int aDy = aDestY; aRows > 0; aRows--, aDy++)
		{
			uint16_t* d = (uint16_t*)((uint8_t*)mScreen.data() + aDy * mScreenPitch + aDestX * 2);
			const uint8_t* a = theSurface->mAlpha + aLeft + theSurface->mWidth * (y + aDy);
			const uint16_t* s = (const uint16_t*)((const uint8_t*)theSurface->mPixels + (y + aDy) * theSurface->mPitch +
				aLeft * 2);
			for (int n = aRight - aLeft; n > 0; n--, d++, s++, a++)
			{
				if (*a == 0xff)
					*d = *s;
			}
		}
	}
	else
	{
		int aBytes = ((aRight - aLeft) * theSurface->mBpp) / 8;
		int y = aTop - aDestY;
		for (int aDy = aDestY; aRows > 0; aRows--, aDy++)
		{
			const uint8_t* s = (const uint8_t*)theSurface->mPixels + (y + aDy) * theSurface->mPitch + aLeft * 2;
			uint8_t* d = (uint8_t*)mScreen.data() + mScreenPitch * aDy + aDestX * 2;
			memcpy(d, s, aBytes);
		}
	}
	return true;
}

// 0x4231f0, with 0x423000 and 0x423140
bool HDisplay::draw(const PGPoint* theAt, HSurface* theSurface)
{
	int aX = 0, aY = 0;
	if (theAt != nullptr)
	{
		aX = theAt->x;
		aY = theAt->y;
		// the original passes the point as a rectangle; port: the surface's rectangle at the point
		PGRect aRect = {aX, aY, aX + theSurface->mWidth, aY + theSurface->mHeight};
		mDirty.add(aRect);
	}
	for (int y = 0; y < theSurface->mHeight; y++)
	{
		uint16_t* d = (uint16_t*)((uint8_t*)mScreen.data() + (aY + y) * mScreenPitch + aX * 2);
		const uint16_t* s = (const uint16_t*)((const uint8_t*)theSurface->mPixels + theSurface->mPitch * y);
		if (theSurface->mAlpha != nullptr)
		{
			const uint8_t* a = theSurface->mAlpha + theSurface->mWidth * y;
			for (int x = 0; x < theSurface->mWidth; x++)
				if (a[x] == 0xff)
					d[x] = s[x];
		}
		else
			memcpy(d, s, theSurface->mPitch);
	}
	return true;
}

// The clipping of 0x423620 and 0x4237a0: false when nothing is left to draw
static bool ClipToScreen(int theWidth, int theHeight, const PGRect* theDest, const PGRect* theSrc, PGRect* theOutDest,
	PGRect* theOutSrc)
{
	int l = theDest->left, t = theDest->top, r = theDest->right, b = theDest->bottom;
	PGRect s = *theSrc;
	PGRect d;
	if (theWidth < l || r < 0 || theHeight < t || b < 0)
		return false;
	if (l < 0 || theWidth < r || t < 0 || theHeight < b)
	{
		d.left = l;
		if (l < 0)
		{
			d.left = 0;
			s.left = s.left - l;
		}
		d.top = t;
		if (t < 0)
		{
			d.top = 0;
			s.top = s.top - t;
		}
		d.right = r;
		if (theWidth < r)
		{
			s.right = s.right + (theWidth - r);
			d.right = theWidth;
		}
		d.bottom = b;
		if (theHeight < b)
		{
			s.bottom = s.bottom + (theHeight - b);
			d.bottom = theHeight;
		}
	}
	else
		d = *theDest;
	if (s.right == s.left)
		return false;
	if (s.top == s.bottom)
		return false;
	*theOutDest = d;
	*theOutSrc = s;
	return true;
}

// 0x423620
bool HDisplay::blitEx(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface, int s1, int s2, int s3,
	int s4, int d1, int d2, int d3, int d4, int theOp)
{
	PGRect aDest, aSrc;
	if (!ClipToScreen(mWidth, mHeight, theDest, theSrc, &aDest, &aSrc))
		return true;
	return blend(&aDest, &aSrc, theSurface, s1, s2, s3, s4, d1, d2, d3, d4, theOp);
}

// 0x4237a0
bool HDisplay::blit(const PGRect* theDest, const PGRect* theSrc, HSurface* theSurface)
{
	PGRect aDest, aSrc;
	if (!ClipToScreen(mWidth, mHeight, theDest, theSrc, &aDest, &aSrc))
		return true;
	return copy(&aDest, &aSrc, theSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Text

HFont* HDisplay::createFont(PGSprite* theSprite, int theWidth, int theHeight)
{
	HFont* aFont = new HFont();
	aFont->mSprite = theSprite;
	aFont->mWidth = theWidth;
	aFont->mHeight = theHeight;
	return aFont;
}

void HDisplay::freeFont(HFont* theFont)
{
	delete theFont;
}

void HDisplay::setBlendMode(int s1, int s2, int s3, int s4, int d1, int d2, int d3, int d4, int theOp)
{
	mTextMode[0] = s1;
	mTextMode[1] = s2;
	mTextMode[2] = s3;
	mTextMode[3] = s4;
	mTextMode[4] = d1;
	mTextMode[5] = d2;
	mTextMode[6] = d3;
	mTextMode[7] = d4;
	mTextMode[8] = theOp;
}

// 0x423280: each character is the cell (c % 16, c / 16 - 1) of the font's sprite
void HDisplay::drawText(const char* theText, int theX, int theY, const PGRect* theClip, int theAlign, HFont* theFont,
	const float* theColor)
{
	(void)theColor;
	if (theFont == nullptr)
		return;
	int aLen = (int)strlen(theText);
	int aX = theX;
	if (theAlign == 2)
		aX = theX - (int)(((unsigned int)aLen * theFont->mWidth) >> 1);
	else if (theAlign == 1)
		aX = theX - aLen * theFont->mWidth;
	for (int i = 0; i < aLen; i++)
	{
		int w = theFont->mWidth;
		int c = (signed char)theText[i];
		PGRect aSrc;
		aSrc.left = (c % 16) * w;
		aSrc.right = w + aSrc.left;
		aSrc.top = (c / 16 - 1) * theFont->mHeight;
		aSrc.bottom = theFont->mHeight + aSrc.top;
		PGRect aDest;
		aDest.left = w * i + aX;
		aDest.bottom = theFont->mHeight + theY;
		aDest.right = w + aDest.left;
		aDest.top = theY;
		if (aDest.left <= theClip->right && theClip->left <= aDest.right && theY <= theClip->bottom &&
			theClip->top <= aDest.bottom)
		{
			if (aDest.left < theClip->left || theClip->right < aDest.right || theY < theClip->top ||
				theClip->bottom < aDest.bottom)
			{
				if (aDest.left < theClip->left)
				{
					aSrc.left = (theClip->left - aDest.left) + aSrc.left;
					aDest.left = theClip->left;
				}
				if (theY < theClip->top)
				{
					aSrc.top = (theClip->top - theY) + aSrc.top;
					aDest.top = theClip->top;
				}
				if (theClip->right < aDest.right)
				{
					aSrc.right = aSrc.right + (theClip->right - aDest.right);
					aDest.right = theClip->right;
				}
				if (theClip->bottom < aDest.bottom)
				{
					aSrc.bottom = aSrc.bottom + (theClip->bottom - aDest.bottom);
					aDest.bottom = theClip->bottom;
				}
			}
			theFont->mSprite->blitEx(&aDest, &aSrc, mTextMode[0], mTextMode[1], mTextMode[2], mTextMode[3],
				mTextMode[4], mTextMode[5], mTextMode[6], mTextMode[7], mTextMode[8]);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The screen

// 0x424390: port: the host owns the window; the back buffer is ours
bool HDisplay::init(int theWidth, int theHeight, bool theFullscreen)
{
	mScreen.assign((size_t)theWidth * theHeight, 0);
	mScreenPitch = theWidth * 2;
	SetPixelFormat(5, 5, 5, 10, 5, 0);
	GetHost()->SetFullscreen(theFullscreen);
	gScreenDisplay = this;
	return setSize(theWidth, theHeight);
}

void HDisplay::shutdown()
{
	if (gScreenDisplay == this)
		gScreenDisplay = nullptr;
	mScreen.clear();
}

bool HDisplay::setSize(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;
	setBlendMode(BLEND_ALPHA);
	return true;
}

void HDisplay::clear()
{
	for (uint16_t& p : mScreen)
		p = 0;
}

// 0x424710: port: the host shows the whole back buffer
void HDisplay::present(const PGRect* theRects, int theCount)
{
	(void)theRects;
	if (theCount > 0)
		GetHost()->ScreenChanged();
}

// 0x423500
void HDisplay::flip()
{
	static PyTime sLast = 0;
	PyTime aNow = HTimeGetTime();
	if (sLast == 0)
		sLast = aNow;
	int aDelta = PyTimeDelta(aNow, sLast);
	sLast = aNow;
	memmove(&mFrameTimes[0], &mFrameTimes[1], 15 * sizeof(int));
	mFrameTimes[15] = aDelta;
	int aCount = mDirty.count();
	if (aCount > 0)
	{
		present(mDirty.mRects, aCount);
		mDirty.clear();
	}
}

} // namespace pg
