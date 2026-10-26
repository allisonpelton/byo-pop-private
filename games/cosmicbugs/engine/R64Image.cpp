// Retro64's images (0x4234a0..0x425880), pixel formats, palettes, the sine tables and the pixel helpers
// 0x42d060..0x42e1c0. The 24- and 32-bit destinations the blit wrappers also handle are not translated
// (R64Blit.h says why): a wrapper whose destination is not 16-bit draws nothing here.

#include "R64.h"
#include "R64Blit.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

int gCosTable[361]; // 0x709988
int gSinTable[361]; // 0x709f30

void InitComponent(uint32_t theMask, uint32_t& theMaskOut, int& theBits, int& theShift, int& theLoss)
{
	theMaskOut = theMask;
	theBits = 0;
	theShift = 0;
	uint32_t v = theMask;
	if ((theMask & 1) == 0)
	{
		do
		{
			v = 0;
			if (theMask == 0)
				break;
			theShift++;
			theMask >>= 1;
			v = theMask;
		} while ((theMask & 1) == 0);
	}
	if ((v & 1) != 0)
	{
		int n = 0;
		do
		{
			v >>= 1;
			n++;
		} while ((v & 1) != 0);
		theBits = n;
	}
	theLoss = 8 - theBits;
}

inline bool Is16(const PixelFormat* theFmt)
{
	return theFmt->bpp == 16;
}

inline int DegIndex(int theAngle)
{
	if (theAngle < 0)
		return 360 - (-theAngle) % 360;
	return theAngle % 360;
}

} // namespace

// 0x401230 (and 0x42e2b0). An 8-bit format gets the masks 0xff0000, 0xff00 and 0xff.
void PixelFormat_Init(PixelFormat* f, int bpp, uint32_t r, uint32_t g, uint32_t b)
{
	if (bpp == 8)
	{
		r = 0xff0000;
		g = 0xff00;
		b = 0xff;
	}
	f->bpp = bpp;
	InitComponent(r, f->rMask, f->rBits, f->rShift, f->rLoss);
	InitComponent(g, f->gMask, f->gBits, f->gShift, f->gLoss);
	InitComponent(b, f->bMask, f->bBits, f->bShift, f->bLoss);
}

// 0x401330
Palette::Palette()
{
	std::memset(rgb, 0, sizeof(rgb));
	p16 = (uint16_t*)conv;
	p32 = (uint32_t*)conv;
	std::memset(conv, 0, sizeof(conv));
}

// 0x42b9a0
void InitSinCos()
{
	int i = 0;
	do
	{
		double a = ((double)i * 3.14159) / 180.0;
		gCosTable[i] = (int)(std::cos(a) * 1024.0);
		gSinTable[i] = (int)(std::sin(a) * 1024.0);
		i++;
	} while (i < 0x169);
}

// 0x42ba10
float SinDeg(int theAngle)
{
	return (float)(gSinTable[DegIndex(theAngle)] * 0.0009765625);
}

// 0x42ba70
float CosDeg(int theAngle)
{
	return (float)(gCosTable[DegIndex(theAngle)] * 0.0009765625);
}

// 0x42bad0
int ClipBlit(int clipL, int clipT, int clipR, int clipB, int x, int y, int w, int h, int* outX, int* outY,
	int* outSx, int* outSy, int* outW, int* outH)
{
	int aRight = x + -1 + w;
	int aBottom = y + -1 + h;
	if (x <= clipR && clipL <= aRight && y <= clipB && clipT <= aBottom)
	{
		*outSx = 0;
		*outSy = 0;
		*outW = w;
		*outH = h;
		if (x < clipL)
		{
			*outSx = *outSx + (clipL - x);
			*outW = *outW - *outSx;
			x = clipL;
		}
		if (y < clipT)
		{
			*outSy = *outSy + (clipT - y);
			*outH = *outH - *outSy;
			y = clipT;
		}
		if (clipR < aRight)
			*outW = *outW + (clipR - aRight);
		if (clipB < aBottom)
			*outH = *outH + (clipB - aBottom);
		*outX = x;
		*outY = y;
		if (0 < *outW && 0 < *outH)
			return 1;
	}
	return 0;
}

// 0x42b8d0: whether two rectangles {l, t, r, b} (inclusive) overlap, by the distance of their centres as the
// original works it out (halves rounded toward 0)
int Rect_Overlap(int l1, int t1, int r1, int b1, int l2, int t2, int r2, int b2)
{
	int w1 = (r1 - l1) + 1;
	int w2 = (r2 - l2) + 1;
	int aHalfW1 = w1 / 2;
	int dx = std::abs(((aHalfW1 - w2 / 2) - l2) + l1);
	int h1 = (b1 - t1) + 1;
	int h2 = (b2 - t2) + 1;
	if (((w2 + w1) / 2 < dx || (h2 + h1) / 2 < std::abs(((h1 / 2 - h2 / 2) - t2) + t1)) &&
		(aHalfW1 <= dx || h1 / 2 <= std::abs(((h1 / 2 - h2 / 2) - t2) + t1)))
		return 0;
	return 1;
}

// 0x42bbc0
void Rect_Intersect(int* theOut, int l1, int t1, int r1, int b1, int l2, int t2, int r2, int b2)
{
	if (l1 <= l2)
		l1 = l2;
	if (t1 <= t2)
		t1 = t2;
	if (r2 <= r1)
		r1 = r2;
	if (b2 <= b1)
		b1 = b2;
	theOut[0] = l1;
	theOut[1] = t1;
	theOut[2] = r1;
	theOut[3] = b1;
}

// 0x42bc10 (rounded to the nearest, as the original's fistp)
void Palette_Gradient(Palette* thePalette, int theFrom, int theTo, uint8_t r0, uint8_t g0, uint8_t b0, uint8_t r1,
	uint8_t g1, uint8_t b1)
{
	double aSteps = (double)std::abs(theTo - theFrom) + 1.0;
	double r = r0;
	double aDr = (double)r1 - r;
	double g = g0;
	double aDg = (double)g1 - g;
	bool anUp = theFrom < theTo;
	double b = b0;
	double aDb = (double)b1 - b;
	for (int i = 0; i < aSteps; i++)
	{
		thePalette->rgb[theFrom * 4] = (uint8_t)(int)std::nearbyint(r);
		thePalette->rgb[theFrom * 4 + 1] = (uint8_t)(int)std::nearbyint(g);
		thePalette->rgb[theFrom * 4 + 2] = (uint8_t)(int)std::nearbyint(b);
		theFrom = theFrom + (anUp ? 1 : -1);
		r = r + aDr / aSteps;
		g = g + aDg / aSteps;
		b = b + aDb / aSteps;
	}
}

// 0x4234a0. The original leaves +0x3c and +0x40 alone; the port's are 0.
void Image_Init(Image* img)
{
	img->bits = nullptr;
	img->w = 0;
	img->h = 0;
	img->clip[0] = 0;
	img->clip[1] = 0;
	img->clip[2] = 0;
	img->clip[3] = 0;
	img->pitch = 0;
	img->origW = 0;
	img->origH = 0;
	img->xOff = 0;
	img->yOff = 0;
	img->dataSize = 0;
	img->handleX = 0;
	img->handleY = 0;
	img->prev = nullptr;
	img->next = nullptr;
	PixelFormat_Init(&img->fmt, 8, 0, 0, 0);
	img->palette = nullptr;
	img->rawPalette = nullptr;
	img->defPalette = nullptr;
	std::memset(img->name, 0, sizeof(img->name));
	std::memcpy(img->name, "None", 4);
	img->hasMask = 0;
	img->mask = nullptr;
	img->maskX = 0;
	img->maskY = 0;
	img->defMask = nullptr;
	img->rle = nullptr;
	img->fileName = nullptr;
	img->notOwned = 0;
	img->unkd8 = 0;
	img->paletteNumber = 0;
	img->isMask = 0;
	img->unke4 = 0;
	img->autoMask = nullptr;
	img->bankDataSize = 0;
	img->bankPitch = 0;
	PixelFormat_Init(&img->bankFmt, 8, 0, 0, 0);
}

// 0x423570
void Image_SetMidHandle(Image* img)
{
	img->handleX = img->origW / 2;
	img->handleY = img->origH / 2;
}

// 0x423590
int Image_DrawX(const Image* img)
{
	return img->xOff - img->handleX;
}

// 0x4235a0
int Image_DrawY(const Image* img)
{
	return img->yOff - img->handleY;
}

// 0x4235b0: no mask falls back to the image's default one
void Image_SetMask(Image* img, Image* mask, int mx, int my)
{
	if (mask != nullptr)
	{
		img->mask = mask;
		img->maskX = mx;
		img->maskY = my;
		img->hasMask = 1;
		return;
	}
	img->maskX = 0;
	img->maskY = 0;
	if (img->defMask != nullptr)
	{
		img->mask = img->defMask;
		img->hasMask = 1;
		return;
	}
	img->hasMask = 0;
	img->mask = nullptr;
}

// 0x423620
void Image_SetPalette(Image* img, Palette* pal)
{
	if (pal == nullptr)
		pal = img->defPalette;
	img->palette = pal;
}

// 0x423640. The old bits are freed whoever owns them, as in the original.
int Image_ConvertTo(Image* img, const PixelFormat* fmt, const uint8_t* rgb)
{
	PixelFormat aFmt = *fmt;
	int aPitch = (aFmt.bpp >> 3) * img->w;
	int aRem = aPitch % 4;
	if (aRem != 0)
		aPitch = aPitch + (4 - aRem);
	uint8_t* aBits = (uint8_t*)std::malloc((size_t)img->h * aPitch);
	if (aBits != nullptr)
	{
		if (ConvertPixels(&aFmt, aBits, aPitch, 0, 0, &img->fmt, rgb, img->bits, img->pitch, 0, 0, img->w, img->h) !=
			0)
		{
			std::free(img->bits);
			img->pitch = aPitch;
			img->dataSize = img->h * aPitch;
			img->fmt = aFmt;
			img->bits = aBits;
			return 1;
		}
		std::free(aBits);
	}
	return 0;
}

// 0x423710
void Image_FreeRle(Image* img)
{
	if (img->rle != nullptr)
	{
		std::free(img->rle);
		img->rle = nullptr;
	}
}

// 0x424f90: whether column x of the mask has a pixel that is not 0
int Image_MaskColumnSet(const Image* img, int x)
{
	const Image* aMask = img->mask;
	if (aMask == nullptr)
		return 0;
	int i = 0;
	if (0 < aMask->h)
	{
		const uint8_t* p = aMask->bits + x;
		do
		{
			if (*p != 0)
				return 1;
			i++;
			p += aMask->pitch;
		} while (i < aMask->h);
	}
	return 0;
}

// 0x424fe0: an image whose bits belong to a bank keeps everything
void Image_Free(Image* img)
{
	if (img->notOwned == 0)
	{
		if (img->rle != nullptr)
		{
			std::free(img->rle);
			img->rle = nullptr;
		}
		if (img->bits != nullptr)
			std::free(img->bits);
		if (img->fileName != nullptr)
			std::free(img->fileName);
		Image_Init(img);
	}
}

// 0x425040. Image_Free resets the format to 8-bit first, so a null fmt makes an 8-bit image.
void Image_Create(Image* img, int w, int h, const PixelFormat* fmt)
{
	if (img->notOwned == 0)
	{
		Image_Free(img);
		if (fmt != nullptr)
			img->fmt = *fmt;
		img->clip[2] = w + -1;
		int aPitch = (img->fmt.bpp >> 3) * w;
		img->clip[3] = h + -1;
		int aRem = aPitch % 4;
		img->bits = nullptr;
		img->w = w;
		img->h = h;
		img->clip[0] = 0;
		img->clip[1] = 0;
		img->pitch = aPitch;
		if (aRem != 0)
			img->pitch = (aPitch - aRem) + 4;
		img->origW = w;
		img->origH = h;
		img->xOff = 0;
		img->yOff = 0;
		img->bits = (uint8_t*)std::malloc((size_t)img->pitch * h);
		std::memset(img->bits, 0, (size_t)img->pitch * img->h);
		img->dataSize = img->pitch * img->h;
	}
}

// 0x4250f0
void Image_CreateCopy(Image* img, const uint8_t* src, int spitch, int w, int h, const PixelFormat* fmt)
{
	if (img->notOwned == 0)
	{
		Image_Create(img, w, h, fmt);
		Copy8(img->bits, 0, 0, img->pitch, 0, src, 0, 0, spitch, 0, (img->fmt.bpp >> 3) * w, h);
	}
}

// 0x425150
void Image_BuildRle(Image* img)
{
	if (img->rle != nullptr)
	{
		std::free(img->rle);
		img->rle = nullptr;
	}
	if (img->fmt.bpp == 8)
		img->rle = Rle_Build(img->bits, img->pitch, img->h);
}

// 0x425820
void Image_Destroy(Image* img)
{
	Image* anAutoMask = img->autoMask;
	if (anAutoMask != nullptr)
	{
		Image_Destroy(anAutoMask);
		delete anAutoMask;
		img->autoMask = nullptr;
	}
	if (img->rle != nullptr)
	{
		std::free(img->rle);
		img->rle = nullptr;
	}
	if (img->notOwned == 0)
		Image_Free(img);
}

// 0x425880
void Image_Copy(Image* img, const Image* src)
{
	if (img->notOwned == 0)
		Image_CreateCopy(img, src->bits, src->pitch, src->w, src->h, &src->fmt);
}

// 0x423740
void Image_Blit(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (0 < w && 0 < h)
	{
		if (fmt->bpp == 8)
		{
			if (img->fmt.bpp == 8)
				Copy8(dst, ox, oy, dpitch, 0, img->bits, sx, sy, img->pitch, 0, w, h); // 0x429110
		}
		else if (img->fmt.bpp == 8)
		{
			if (img->palette != nullptr && Is16(fmt)) // 0x4297d0
				Pal16(dst, dpitch, ox, oy, img->bits, img->pitch, img->palette, sx, sy, w, h);
		}
		else if (Is16(fmt)) // 0x4291a0
			Copy16(dst, dpitch, ox, oy, img->bits, img->pitch, sx, sy, w, h);
	}
}

// 0x4238a0. A 16-bit image without a mask draws nothing; an 8-bit one is its own mask.
void Image_BlitUnclipped(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int x, int y)
{
	int ax = x + (img->xOff - img->handleX);
	int ay = y + (img->yOff - img->handleY);
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (fmt->bpp == 8)
	{
		if (img->fmt.bpp != 8)
			return;
		Copy8Key(dst, ax, ay, dpitch, 0, img->bits, 0, 0, img->pitch, 0, img->w, img->h); // 0x429150
		return;
	}
	const uint8_t* aMask;
	int aMaskPitch, aMaskX, aMaskY;
	if (img->hasMask == 1)
	{
		const uint8_t* aRle = img->mask->rle;
		if (aRle != nullptr && img->maskX == 0 && img->maskY == 0)
		{
			if (img->fmt.bpp != 8)
			{
				if (Is16(fmt)) // 0x429610
					Rle16Copy(dst, dpitch, fmt, ax, ay, img->bits, img->pitch, aRle);
				return;
			}
			if (img->palette == nullptr)
				return;
			if (Is16(fmt)) // 0x429bf0
				RlePal16Copy(dst, dpitch, fmt, ax, ay, img->bits, img->pitch, img->palette, aRle);
			return;
		}
		if (img->fmt.bpp != 8)
		{
			if (Is16(fmt)) // 0x429270
				Copy16Mask(dst, dpitch, ax, ay, img->bits, img->pitch, 0, 0, img->mask->bits, img->mask->pitch,
					img->maskX, img->maskY, img->w, img->h);
			return;
		}
		if (img->palette == nullptr)
			return;
		aMask = img->mask->bits;
		aMaskPitch = img->mask->pitch;
		aMaskX = img->maskX;
		aMaskY = img->maskY;
	}
	else
	{
		if (img->fmt.bpp != 8)
			return;
		if (img->palette == nullptr)
			return;
		aMask = img->bits;
		aMaskPitch = img->pitch;
		aMaskX = 0;
		aMaskY = 0;
	}
	if (Is16(fmt)) // 0x4298b0
		Pal16Mask(dst, dpitch, ax, ay, img->bits, img->pitch, img->palette, 0, 0, aMask, aMaskPitch, aMaskX, aMaskY,
			img->w, img->h);
}

// 0x423a70. The runs are used only when the whole image is drawn.
void Image_BlitMasked(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (!(0 < w && 0 < h))
		return;
	if (fmt->bpp == 8)
	{
		if (img->fmt.bpp == 8)
			Copy8Key(dst, ox, oy, dpitch, 0, img->bits, sx, sy, img->pitch, 0, w, h); // 0x429150
	}
	else if (img->hasMask == 1)
	{
		const uint8_t* aRle = img->mask->rle;
		if (aRle == nullptr || img->maskX != 0 || img->maskY != 0 || w != img->w || h != img->h)
		{
			if (img->fmt.bpp != 8)
			{
				if (Is16(fmt)) // 0x429270
					Copy16Mask(dst, dpitch, ox, oy, img->bits, img->pitch, sx, sy, img->mask->bits, img->mask->pitch,
						img->maskX, img->maskY, w, h);
				return;
			}
			if (img->palette != nullptr)
			{
				if (Is16(fmt)) // 0x4298b0
					Pal16Mask(dst, dpitch, ox, oy, img->bits, img->pitch, img->palette, sx, sy, img->mask->bits,
						img->mask->pitch, img->maskX, img->maskY, w, h);
				return;
			}
		}
		else
		{
			if (img->fmt.bpp != 8)
			{
				if (Is16(fmt)) // 0x429610
					Rle16Copy(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, aRle);
				return;
			}
			if (img->palette != nullptr)
			{
				if (Is16(fmt)) // 0x429bf0
					RlePal16Copy(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, aRle);
				return;
			}
		}
	}
	else if (img->fmt.bpp == 8 && img->palette != nullptr)
	{
		if (Is16(fmt)) // 0x4298b0
			Pal16Mask(dst, dpitch, ox, oy, img->bits, img->pitch, img->palette, sx, sy, img->bits, img->pitch, 0, 0,
				w, h);
	}
}

// 0x423d10: the colour through the image's mask (an 8-bit image without one is its own)
void Image_BlitFill(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t colour)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (0 < w && 0 < h && fmt->bpp != 8)
	{
		const uint8_t* aMask;
		int aMaskPitch, aMaskX, aMaskY;
		if (img->hasMask == 1)
		{
			const uint8_t* aRle = img->mask->rle;
			if (aRle != nullptr && img->maskX == 0 && img->maskY == 0 && w == img->w && h == img->h)
			{
				if (Is16(fmt)) // 0x429650
					Rle16Fill(dst, dpitch, fmt, ox, oy, aRle, (uint16_t)colour);
				return;
			}
			aMaskY = img->maskY;
			aMaskX = img->maskX;
			aMaskPitch = img->mask->pitch;
			aMask = img->mask->bits;
		}
		else
		{
			if (img->fmt.bpp != 8)
				return;
			aMask = img->bits;
			aMaskPitch = img->pitch;
			aMaskY = 0;
			aMaskX = 0;
		}
		if (Is16(fmt)) // 0x429380
			Fill16Mask(dst, dpitch, fmt, ox, oy, sx, sy, aMask, aMaskPitch, aMaskX, aMaskY, w, h, (uint16_t)colour);
	}
}

// 0x423e80
void Image_BlitFillAlpha(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t colour, uint32_t alpha)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (0 < w && 0 < h && fmt->bpp != 8)
	{
		const uint8_t* aMask;
		int aMaskPitch, aMaskX, aMaskY;
		if (img->hasMask == 1)
		{
			const uint8_t* aRle = img->mask->rle;
			if (aRle != nullptr && img->maskX == 0 && img->maskY == 0 && w == img->w && h == img->h)
			{
				if (Is16(fmt)) // 0x429690
					Rle16FillAlpha(dst, dpitch, fmt, ox, oy, aRle, (uint16_t)colour, alpha & 0xff);
				return;
			}
			aMaskY = img->maskY;
			aMaskX = img->maskX;
			aMaskPitch = img->mask->pitch;
			aMask = img->mask->bits;
		}
		else
		{
			if (img->fmt.bpp != 8)
				return;
			aMask = img->bits;
			aMaskPitch = img->pitch;
			aMaskY = 0;
			aMaskX = 0;
		}
		if (Is16(fmt)) // 0x4293c0
			Fill16MaskAlpha(dst, dpitch, fmt, ox, oy, sx, sy, aMask, aMaskPitch, aMaskX, aMaskY, w, h, (uint16_t)colour,
				(uint8_t)alpha);
	}
}

// 0x424010, 0x424220, 0x4247e0 share one shape: only images with a mask draw
namespace
{

enum MaskedKind
{
	kBlend,  // 0x424010
	kAlpha,  // 0x424220
	kAdd     // 0x4247e0
};

void BlitMaskedKind(MaskedKind theKind, Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip,
	int x, int y)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (!(0 < w && 0 < h && fmt->bpp != 8 && img->hasMask == 1))
		return;
	const uint8_t* aRle = img->mask->rle;
	const uint8_t* m = img->mask->bits;
	int mp = img->mask->pitch;
	if (aRle == nullptr || img->maskX != 0 || img->maskY != 0 || w != img->w || h != img->h)
	{
		if (img->fmt.bpp == 8)
		{
			if (img->palette != nullptr && Is16(fmt))
			{
				if (theKind == kBlend) // 0x4299d0
					PalBlend16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, sx, sy, m, mp,
						img->maskX, img->maskY, w, h);
				else if (theKind == kAlpha) // 0x429a10
					PalAlpha16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, sx, sy, m, mp,
						img->maskX, img->maskY, w, h);
				else // 0x429a90
					PalAdd16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, sx, sy, m, mp,
						img->maskX, img->maskY, w, h);
			}
		}
		else if (Is16(fmt))
		{
			if (theKind == kBlend) // 0x429400
				Blend16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, sx, sy, m, mp, img->maskX, img->maskY, w,
					h);
			else if (theKind == kAlpha) // 0x429440
				Alpha16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, sx, sy, m, mp, img->maskX, img->maskY, w,
					h);
			else // 0x4294c0
				Add16Mask(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, sx, sy, m, mp, img->maskX, img->maskY, w, h);
		}
	}
	else if (img->fmt.bpp != 8)
	{
		if (Is16(fmt))
		{
			if (theKind == kBlend) // 0x4296d0
				Rle16Blend(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, aRle);
			else if (theKind == kAlpha) // 0x429710
				Rle16Alpha(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, aRle);
			else // 0x429790
				Rle16Add(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, aRle);
		}
	}
	else if (img->palette != nullptr && Is16(fmt))
	{
		if (theKind == kBlend) // 0x429c30
			RlePal16Blend(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, aRle);
		else if (theKind == kAlpha) // 0x429c70
			RlePal16Alpha(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, aRle);
		else // 0x429cf0
			RlePal16Add(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, aRle);
	}
}

// 0x424430 and 0x4245b0: the faded alpha blend, unclipped (whole image) or clipped
void BlitAlphaFadeAt(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int ox, int oy, int sx, int sy,
	int w, int h, bool theWhole, uint32_t alpha)
{
	const uint8_t* aRle = img->mask->rle;
	const uint8_t* m = img->mask->bits;
	int mp = img->mask->pitch;
	if (aRle == nullptr || img->maskX != 0 || img->maskY != 0 || !theWhole)
	{
		if (img->fmt.bpp == 8)
		{
			if (img->palette != nullptr && Is16(fmt)) // 0x429a50
				PalAlpha16MaskFade(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, sx, sy, m, mp,
					img->maskX, img->maskY, w, h, (int)alpha);
		}
		else if (Is16(fmt)) // 0x429480
			Alpha16MaskFade(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, sx, sy, m, mp, img->maskX, img->maskY, w,
				h, (int)alpha);
	}
	else if (img->fmt.bpp != 8)
	{
		if (Is16(fmt)) // 0x429750
			Rle16AlphaFade(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, aRle, alpha);
	}
	else if (img->palette != nullptr && Is16(fmt)) // 0x429cb0
		RlePal16AlphaFade(dst, dpitch, fmt, ox, oy, img->bits, img->pitch, img->palette, aRle, alpha);
}

} // namespace

void Image_BlitBlend(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	BlitMaskedKind(kBlend, img, dst, dpitch, fmt, clip, x, y);
}

void Image_BlitAlpha(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	BlitMaskedKind(kAlpha, img, dst, dpitch, fmt, clip, x, y);
}

void Image_BlitAdd(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	BlitMaskedKind(kAdd, img, dst, dpitch, fmt, clip, x, y);
}

// 0x424430: the runs whenever the mask has them and no offset
void Image_BlitAlphaFadeUnclipped(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int x, int y,
	uint32_t alpha)
{
	int ax = x + (img->xOff - img->handleX);
	int ay = y + (img->yOff - img->handleY);
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (fmt->bpp != 8 && img->hasMask == 1)
		BlitAlphaFadeAt(img, dst, dpitch, fmt, ax, ay, 0, 0, img->w, img->h, true, alpha);
}

// 0x4245b0
void Image_BlitAlphaFade(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t alpha)
{
	int ox, oy, sx, sy, w, h;
	if (ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y,
			img->w, img->h, &ox, &oy, &sx, &sy, &w, &h) == 0)
		return;
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (0 < w && 0 < h && fmt->bpp != 8 && img->hasMask == 1)
		BlitAlphaFadeAt(img, dst, dpitch, fmt, ox, oy, sx, sy, w, h, w == img->w && h == img->h, alpha);
}

// 0x4249f0: adds the mask onto an 8-bit destination (fmt is not checked for null here)
void Image_AddMaskUnclipped(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int x, int y)
{
	if (fmt->bpp == 8 && img->hasMask == 1)
		AddMask8(dst, dpitch, (img->xOff - img->handleX) + x, (img->yOff - img->handleY) + y, img->mask->bits,
			img->mask->pitch, img->maskX, img->maskY, img->w, img->h);
}

// 0x424a50. Unlike 0x4249f0 this adds the image's own bits, at the mask's offset.
void Image_AddMask(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y)
{
	if (fmt->bpp == 8)
	{
		int ox, oy, sx, sy, w, h;
		int aClipped = ClipBlit(clip[0], clip[1], clip[2], clip[3], (img->xOff - img->handleX) + x,
			(img->yOff - img->handleY) + y, img->w, img->h, &ox, &oy, &sx, &sy, &w, &h);
		if (aClipped != 0 && 0 < w && 0 < h && img->hasMask == 1)
			AddMask8(dst, dpitch, ox, oy, img->bits, img->pitch, img->maskX + sx, img->maskY + sy, w, h);
	}
}

// 0x424b30: a part of the image, opaque and unclipped
void Image_BlitRect(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	int w, int h)
{
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (fmt->bpp == 8)
	{
		if (img->fmt.bpp == 8)
			Copy8(dst, dx, dy, dpitch, 0, img->bits, sx, sy, img->pitch, 0, w, h); // 0x429110
	}
	else if (img->fmt.bpp == 8)
	{
		if (img->palette != nullptr && Is16(fmt)) // 0x4297d0
			Pal16(dst, dpitch, dx, dy, img->bits, img->pitch, img->palette, sx, sy, w, h);
	}
	else if (Is16(fmt)) // 0x4291a0
		Copy16(dst, dpitch, dx, dy, img->bits, img->pitch, sx, sy, w, h);
}

// 0x424c10: a part of the image through its mask, unclipped
void Image_BlitSubRect(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	int w, int h)
{
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (fmt->bpp == 8)
	{
		if (img->fmt.bpp == 8)
			Copy8Key(dst, dx, dy, dpitch, 0, img->bits, sx, sy, img->pitch, 0, w, h); // 0x429150
	}
	else if (img->hasMask == 1)
	{
		if (img->fmt.bpp != 8)
		{
			if (Is16(fmt)) // 0x429270
				Copy16Mask(dst, dpitch, dx, dy, img->bits, img->pitch, sx, sy, img->mask->bits, img->mask->pitch,
					img->maskX, img->maskY, w, h);
			return;
		}
		if (img->palette != nullptr && Is16(fmt)) // 0x4298b0
			Pal16Mask(dst, dpitch, dx, dy, img->bits, img->pitch, img->palette, sx, sy, img->mask->bits,
				img->mask->pitch, img->maskX, img->maskY, w, h);
	}
	else if (img->fmt.bpp == 8 && img->palette != nullptr && Is16(fmt)) // 0x4298b0
		Pal16Mask(dst, dpitch, dx, dy, img->bits, img->pitch, img->palette, sx, sy, img->bits, img->pitch, 0, 0, w, h);
}

// 0x424d90: the colour through a part of the mask, unclipped. The mask's offset gets sx and sy added, and the
// blitter adds them again, as in the original. fmt is not checked for null.
void Image_FillAlphaRect(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	int w, int h, uint32_t colour, uint32_t alpha)
{
	if (fmt->bpp != 8 && img->hasMask == 1 && Is16(fmt)) // 0x4293c0
		Fill16MaskAlpha(dst, dpitch, fmt, dx, dy, sx, sy, img->mask->bits, img->mask->pitch, img->maskX + sx,
			img->maskY + sy, w, h, (uint16_t)colour, (uint8_t)alpha);
}

// 0x424e10: rotated by angle degrees about (x, y), the image's middle
void Image_BlitRotated(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int x, int y, int angle,
	const int* clip)
{
	if (fmt == nullptr)
		fmt = &img->fmt;
	if (fmt->bpp == 8)
	{
		if (img->fmt.bpp == 8)
			Rotate8(dst, dpitch, img->bits, img->pitch, img->w, img->h, x, y, angle, clip);
	}
	else if (img->hasMask == 1)
	{
		if (img->fmt.bpp != 8)
		{
			if (Is16(fmt)) // 0x429500
				Rotate16(dst, dpitch, img->bits, img->pitch, img->w, img->h, img->mask->bits, img->mask->pitch,
					img->maskX, img->maskY, x, y, angle, clip);
			return;
		}
		if (img->palette != nullptr && Is16(fmt)) // 0x429ad0
			PalRotate16(dst, dpitch, img->bits, img->pitch, img->palette, img->w, img->h, img->mask->bits,
				img->mask->pitch, img->maskX, img->maskY, x, y, angle, clip);
	}
	else if (img->fmt.bpp == 8 && img->palette != nullptr && Is16(fmt)) // 0x429ad0
		PalRotate16(dst, dpitch, img->bits, img->pitch, img->palette, img->w, img->h, img->bits, img->pitch, 0, 0, x, y,
			angle, clip);
}

// 0x42d060
void Fill8(uint8_t* dst, int dpitch, int l, int t, int r, int b, int value)
{
	uint8_t* d = dst + dpitch * t + l;
	if (t <= b)
	{
		int n = (b - t) + 1;
		do
		{
			std::memset(d, value, (size_t)((r - l) + 1));
			d += dpitch;
			n--;
		} while (n != 0);
	}
}

// 0x42db40
void ExtractAlpha(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, int sx, int sy, int w,
	int h)
{
	const uint8_t* s = src + spitch * sy + sx * 4;
	uint8_t* d = dst + dpitch * dy + dx;
	if (0 < h)
	{
		int aRows = h;
		do
		{
			const uint8_t* ps = s;
			uint8_t* pd = d;
			int n = w;
			if (0 < w)
			{
				do
				{
					*pd = ps[3];
					n--;
					ps += 4;
					pd++;
				} while (n != 0);
			}
			s += spitch;
			d += dpitch;
			aRows--;
		} while (aRows != 0);
	}
}

// 0x42dd60. The original adds four pixels at a time between aligned edges, skipping words that are all 0; per pixel
// that is the same as this.
void AddMask8(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, int sx, int sy, int w, int h)
{
	uint8_t* d = dst + dpitch * dy + dx;
	const uint8_t* s = src + spitch * sy + sx;
	if (0 < h)
	{
		do
		{
			uint8_t* pd = d;
			const uint8_t* ps = s;
			int n = w;
			if (0 < w)
			{
				do
				{
					if (*ps != 0)
					{
						uint32_t v = (uint32_t)*ps + (uint32_t)*pd;
						if (0xff < v)
							v = 0xff;
						*pd = (uint8_t)v;
					}
					n--;
					pd++;
					ps++;
				} while (n != 0);
			}
			s += spitch;
			d += dpitch;
			h--;
		} while (h != 0);
	}
}

// 0x42dbc0. The original passes the pixel through globals between a reader per source depth (0x42d920 8-bit,
// 0x42d960 16-bit, 0x42d9c0 24/32-bit) and a writer per destination depth (0x42da10, 0x42da70, 0x42daf0).
int ConvertPixels(const PixelFormat* dstFmt, uint8_t* dst, int dpitch, int dx, int dy, const PixelFormat* srcFmt,
	const uint8_t* rgb, const uint8_t* src, int spitch, int sx, int sy, int w, int h)
{
	uint32_t aSrcBpp = (uint32_t)srcFmt->bpp;
	uint32_t aDstBpp = (uint32_t)dstFmt->bpp;
	if (aSrcBpp != 8 && aSrcBpp != 0x10 && aSrcBpp != 0x18 && aSrcBpp != 0x20)
		return 0;
	if (aDstBpp != 0x10 && aDstBpp != 0x18 && aDstBpp != 0x20)
		return 0;
	uint8_t r = 0, g = 0, b = 0;
	auto aRead = [&](const uint8_t* p)
	{
		if (aSrcBpp == 8)
		{
			r = rgb[*p * 4];
			g = rgb[*p * 4 + 1];
			b = rgb[*p * 4 + 2];
			return;
		}
		uint32_t v;
		if (aSrcBpp == 0x10)
			v = *(const uint16_t*)p;
		else
			std::memcpy(&v, p, 4); // 24-bit reads a whole word too
		r = (uint8_t)(((srcFmt->rMask & v) >> (srcFmt->rShift & 0x1f)) << (srcFmt->rLoss & 0x1f));
		g = (uint8_t)(((srcFmt->gMask & v) >> (srcFmt->gShift & 0x1f)) << (srcFmt->gLoss & 0x1f));
		b = (uint8_t)(((srcFmt->bMask & v) >> (srcFmt->bShift & 0x1f)) << (srcFmt->bLoss & 0x1f));
	};
	auto aWrite = [&](uint8_t* p)
	{
		uint32_t v = (((uint32_t)b >> (dstFmt->bLoss & 0x1f)) << (dstFmt->bShift & 0x1f)) |
			(((uint32_t)g >> (dstFmt->gLoss & 0x1f)) << (dstFmt->gShift & 0x1f)) |
			(((uint32_t)r >> (dstFmt->rLoss & 0x1f)) << (dstFmt->rShift & 0x1f));
		if (aDstBpp == 0x10)
			*(uint16_t*)p = (uint16_t)v;
		else if (aDstBpp == 0x18)
		{
			p[0] = (uint8_t)v;
			p[1] = (uint8_t)(v >> 8);
			p[2] = (uint8_t)(v >> 0x10);
		}
		else
			std::memcpy(p, &v, 4);
	};
	uint8_t* d = dst + dpitch * dy + (aDstBpp >> 3) * dx;
	const uint8_t* s = src + spitch * sy + (aSrcBpp >> 3) * sx;
	if (0 < h)
	{
		int aRows = h;
		do
		{
			const uint8_t* ps = s;
			uint8_t* pd = d;
			if (0 < w)
			{
				int n = w;
				do
				{
					aRead(ps);
					ps += aSrcBpp >> 3;
					aWrite(pd);
					pd += aDstBpp >> 3;
					n--;
				} while (n != 0);
			}
			s += spitch;
			d += dpitch;
			aRows--;
		} while (aRows != 0);
	}
	return 1;
}

// 0x42dfa0. A first pass sizes the buffer, a second writes it; each row is a state machine over the pixels: 0
// counts skipped (0) pixels, 1 blended ones (stored), 2 solid (0xff) ones, and every change of state starts a new
// word-aligned count. A row ends with its runs completed by zero counts. Word alignment is by address in the
// original; malloc's buffers are aligned, so the port aligns by offset.
uint8_t* Rle_Build(const uint8_t* bits, int width, int height)
{
	const uint8_t* aRow = bits;
	uint32_t aSize = 0xc;
	if (0 < height)
	{
		int aRows = height;
		do
		{
			uint32_t aState = 0;
			aSize = aSize + 4;
			int i = 0;
			const uint8_t* p = aRow;
			uint32_t aPrev = 0;
			if (width < 1)
				aSize = aSize + (2 - aState) * 4;
			else
			{
				do
				{
					while (true)
					{
						if (aState == 0)
						{
							if (*p != 0)
								aState = 1;
						}
						else if (aState == 1)
						{
							if (*p == 0 || *p == 0xff)
								aState = 2;
							else
								aSize = aSize + 1;
						}
						else if (aState == 2 && *p != 0xff)
							aState = 0;
						if (width <= i || aState == aPrev)
							break;
						if ((aSize & 3) != 0)
							aSize = aSize + (4 - (aSize & 3));
						aSize = aSize + 4;
						aPrev = aState;
					}
					i++;
					p++;
				} while (i < width);
				if (aState < 2)
					aSize = aSize + (2 - aState) * 4;
			}
			aRow = aRow + width;
			aRows--;
		} while (aRows != 0);
	}
	uint32_t aBufSize = aSize + 4;
	uint8_t* aBuf = (uint8_t*)std::malloc(aBufSize);
	if (aBuf == nullptr)
		return nullptr;
	std::memset(aBuf, 0, aBufSize);
	RleHeader* aHeader = (RleHeader*)aBuf;
	aHeader->size = aBufSize;
	aHeader->width = (uint32_t)width;
	aHeader->height = (uint32_t)height;
	auto Align = [&](uint32_t theAt) { return (theAt & 3) != 0 ? theAt + (4 - (theAt & 3)) : theAt; };
	auto Word = [&](uint32_t theAt) -> uint32_t& { return *(uint32_t*)(aBuf + theAt); };
	uint32_t anAt = 0xc;
	aRow = bits;
	if (0 < height)
	{
		int aRows = height;
		do
		{
			uint32_t aCount = Align(anAt);
			Word(aCount) = 0;
			anAt = aCount + 4;
			uint32_t aState = 0;
			int i = 0;
			uint32_t aPrev = 0;
			const uint8_t* p = aRow;
			if (width < 1)
			{
				int n = 2 - (int)aState;
				do
				{
					anAt = Align(anAt);
					Word(anAt) = 0;
					anAt += 4;
					n--;
				} while (n != 0);
			}
			else
			{
				do
				{
					while (true)
					{
						if (aState == 0)
						{
							if (*p == 0)
								Word(aCount) = Word(aCount) + 1;
							else
								aState = 1;
						}
						else if (aState == 1)
						{
							if (*p == 0 || *p == 0xff)
								aState = 2;
							else
							{
								Word(aCount) = Word(aCount) + 1;
								aBuf[anAt] = *p;
								anAt++;
							}
						}
						else if (aState == 2)
						{
							if (*p == 0xff)
								Word(aCount) = Word(aCount) + 1;
							else
								aState = 0;
						}
						if (width <= i || aState == aPrev)
							break;
						aCount = Align(anAt);
						Word(aCount) = 0;
						anAt = aCount + 4;
						aPrev = aState;
					}
					i++;
					p++;
				} while (i < width);
				if (aState < 2)
				{
					int n = 2 - (int)aState;
					do
					{
						anAt = Align(anAt);
						Word(anAt) = 0;
						anAt += 4;
						n--;
					} while (n != 0);
				}
			}
			aRow = aRow + width;
			aRows--;
		} while (aRows != 0);
	}
	return aBuf;
}

} // namespace r64
