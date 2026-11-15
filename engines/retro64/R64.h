#ifndef __RETRO64_R64_H__
#define __RETRO64_R64_H__

// Retro64's engine, the one copy both Cosmic Bugs and Water Bugs (2007) are built on (docs/ENGINES.md): plain C over
// global state, no classes with virtuals. This header holds its data structures with the original's field offsets in
// comments (the 32-bit layout; the port's own is whatever the compiler makes of these structs). Every function's comment
// gives its address in each game's exe, tagged with the game's folder name. games/cosmicbugs/docs/ENGINE.md maps the
// engine.

#include <cstdint>
#include <cstddef>

namespace r64
{

// The screen: 800 by 600 pixels
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr int SCREEN_CENTRE_X = SCREEN_WIDTH / 2;
constexpr int SCREEN_CENTRE_Y = SCREEN_HEIGHT / 2;

// 0x34 bytes: bits per pixel and each component's mask, bit count, shift and loss (8 - bits).
// Initialised by PixelFormat_Init (0x401230 (cosmicbugs), 0x4012d0 (waterbugs); 0x42e2b0 (cosmicbugs),
// 0x42e790 (waterbugs) is the same code). A component reads as
// ((p & mask) >> shift) << loss and packs as (c >> loss) << shift.
struct PixelFormat
{
	int mBpp;          // +0x00
	uint32_t mRMask;   // +0x04
	int mRBits;        // +0x08
	int mRShift;       // +0x0c
	int mRLoss;        // +0x10
	uint32_t mGMask;   // +0x14
	int mGBits;        // +0x18
	int mGShift;       // +0x1c
	int mGLoss;        // +0x20
	uint32_t mBMask;   // +0x24
	int mBBits;        // +0x28
	int mBShift;       // +0x2c
	int mBLoss;        // +0x30
};

void PixelFormat_Init(PixelFormat* f, int theBpp, uint32_t r, uint32_t g, uint32_t b);

// 0x808 bytes: an 8-bit image's colours {r, g, b, 0} and the same converted to the screen's format: mP16 and mP32
// both point at mConv (0x401330 (cosmicbugs), 0x4013d0 (waterbugs) sets them), whichever the screen's depth uses.
struct Palette
{
	uint8_t mRgb[0x400];   // +0x000
	uint16_t* mP16;        // +0x400
	uint32_t* mP32;        // +0x404
	alignas(4) uint8_t mConv[0x400]; // +0x408

	Palette();
	Palette(const Palette&) = delete;
	Palette& operator=(const Palette&) = delete;
};

struct Image;
void Image_Init(Image* theImg);

// 0x128 bytes (constructor 0x4234a0 (cosmicbugs), 0x423980 (waterbugs), Image_Free 0x424fe0 (cosmicbugs),
// 0x4254c0 (waterbugs))
struct Image
{
	uint8_t* mBits;          // +0x00
	int mW;                  // +0x04
	int mH;                  // +0x08
	int mClip[4];            // +0x0c  left, top, right, bottom (inclusive)
	int mPitch;              // +0x1c  bytes
	int mOrigW;              // +0x20  the size before the bank's packer cropped it
	int mOrigH;              // +0x24
	int mXOff;               // +0x28  where the cropped image sits in the original
	int mYOff;               // +0x2c
	int mDataSize;           // +0x30
	int mHandleX;            // +0x34  the point drawn at (x, y): Image_SetMidHandle makes it the middle
	int mHandleY;            // +0x38
	Image* mPrev;            // +0x3c  an ImageList's links
	Image* mNext;            // +0x40
	PixelFormat mFmt;        // +0x44
	Palette* mPalette;       // +0x78  an 8-bit image's current colours
	uint8_t* mRawPalette;    // +0x7c  an 8-bit bank image's colours as the .bbk lists them (Bank_AssignRawPalettes)
	Palette* mDefPalette;    // +0x80
	int mHasMask;            // +0x84
	Image* mMask;            // +0x88  8 bits per pixel
	int mMaskX;              // +0x8c
	int mMaskY;              // +0x90
	Image* mDefMask;         // +0x94
	uint8_t* mRle;           // +0x98  the mask's runs (Image_BuildRle), when this image is a mask
	char mName[0x34];        // +0x9c  "None"
	char* mFileName;         // +0xd0
	int mNotOwned;           // +0xd4  the bits belong to a bank
	int mPaletteNumberPadding;  // +0xd8  the four bytes before mPaletteNumber: nothing reads them
	int mPaletteNumber;      // +0xdc
	int mIsMask;             // +0xe0
	int mMaskNamed;              // +0xe4
	Image* mAutoMask;        // +0xe8
	int mBankDataSize;       // +0xec
	int mBankPitch;          // +0xf0
	PixelFormat mBankFmt;    // +0xf4

	Image() { Image_Init(this); }
	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
};

// An RLE mask's header (Rle_Build 0x42dfa0 (cosmicbugs), 0x42e480 (waterbugs)): the runs follow it
struct RleHeader
{
	uint32_t mSize;          // +0x0  bytes, the header included
	uint32_t mWidth;         // +0x4
	uint32_t mHeight;        // +0x8
};

// 0x4234a0 (cosmicbugs), 0x423980 (waterbugs): the constructor's body (the original constructs malloc'd memory in
// place; the port news an Image and Image_Destroy plus delete stands for Image_Destroy plus free)
void Image_Init(Image* theImg);
// 0x424fe0 (cosmicbugs), 0x4254c0 (waterbugs)
void Image_Free(Image* theImg);
// 0x425040 (cosmicbugs), 0x425520 (waterbugs)
void Image_Create(Image* theImg, int theWidth, int theHeight, const PixelFormat* theFmt);
// 0x4250f0 (cosmicbugs), 0x4255d0 (waterbugs)
void Image_CreateCopy(Image* theImg, const uint8_t* theSrc, int theSrcPitch, int theWidth, int theHeight,
	const PixelFormat* theFmt);
// 0x425150 (cosmicbugs), 0x425630 (waterbugs)
void Image_BuildRle(Image* theImg);
// 0x425820 (cosmicbugs), 0x425d00 (waterbugs)
void Image_Destroy(Image* theImg);
// 0x425880 (cosmicbugs), 0x425d60 (waterbugs)
void Image_Copy(Image* theImg, const Image* theSrc);
// 0x423570 (cosmicbugs), 0x423a50 (waterbugs)
void Image_SetMidHandle(Image* theImg);
// 0x423590 (cosmicbugs), 0x423a70 (waterbugs)
int Image_DrawX(const Image* theImg);
// 0x4235a0 (cosmicbugs), 0x423a80 (waterbugs)
int Image_DrawY(const Image* theImg);
// 0x423620 (cosmicbugs), 0x423b00 (waterbugs)
void Image_SetPalette(Image* theImg, Palette* thePal);
// 0x423640 (cosmicbugs), 0x423b20 (waterbugs)
int Image_ConvertTo(Image* theImg, const PixelFormat* theFmt, const uint8_t* theRgb);
// 0x423710 (cosmicbugs), 0x423bf0 (waterbugs)
void Image_FreeRle(Image* theImg);
// 0x424f90 (cosmicbugs), 0x425470 (waterbugs)
int Image_MaskColumnSet(const Image* theImg, int x);

// The drawing methods 0x423740..0x424e10 (cosmicbugs), 0x423c20..0x4252f0 (waterbugs). dst is the destination's first
// byte, dstPitch its pitch in bytes and fmt its format (null: the image's own); clip is {left, top, right, bottom},
// inclusive. (x, y) is where the image's handle lands. Each picks a blitter by the destination's depth, the image's
// depth and whether it has a mask and that mask has runs.
// 0x423740 (cosmicbugs), 0x423c20 (waterbugs)
void Image_Blit(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip, int x,
	int y);
// 0x423a70 (cosmicbugs), 0x423f50 (waterbugs)
void Image_BlitMasked(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y);
// 0x423d10 (cosmicbugs), 0x4241f0 (waterbugs)
void Image_BlitFill(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y,
	uint32_t theColour);
// 0x423e80 (cosmicbugs), 0x424360 (waterbugs)
void Image_BlitFillAlpha(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y,
	uint32_t theColour, uint32_t theAlpha);
// 0x424010 (cosmicbugs), 0x4244f0 (waterbugs)
void Image_BlitBlend(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y);
// 0x424220 (cosmicbugs), 0x424700 (waterbugs)
void Image_BlitAlpha(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y);
// 0x4245b0 (cosmicbugs), 0x424a90 (waterbugs)
void Image_BlitAlphaFade(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y, uint32_t theAlpha);
// 0x4247e0 (cosmicbugs), 0x424cc0 (waterbugs)
void Image_BlitAdd(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, const int* theClip,
	int x, int y);
// 0x424b30 (cosmicbugs), 0x425010 (waterbugs)
void Image_BlitRect(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX,
	int theDstY, int theSrcX, int theSrcY,
	int theWidth, int theHeight);
// 0x424c10 (cosmicbugs), 0x4250f0 (waterbugs)
void Image_BlitSubRect(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX,
	int theDstY, int theSrcX, int theSrcY,
	int theWidth, int theHeight);
// 0x424e10 (cosmicbugs), 0x4252f0 (waterbugs)
void Image_BlitRotated(Image* theImg, uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int x, int y,
	int theAngle,
	const int* theClip);

// 0x42bad0 (cosmicbugs), 0x42bfb0 (waterbugs): clips a w x h rectangle at (x, y) to {theClipL, theClipT, theClipR,
// theClipB} (inclusive); 1 when some of it is left
int ClipBlit(int theClipL, int theClipT, int theClipR, int theClipB, int x, int y, int theWidth, int theHeight,
	int* theOutX, int* theOutY, int* theOutSx, int* theOutSy, int* theOutW, int* theOutH);
// 0x42bc10 (cosmicbugs), 0x42c0f0 (waterbugs): a palette's colours theFrom to theTo (either way, both included) shade
// from one colour to the other
void Palette_Gradient(Palette* thePalette, int theFrom, int theTo, uint8_t theR0, uint8_t theG0, uint8_t theB0,
	uint8_t theR1, uint8_t theG1, uint8_t theB1);

// 0x42dbc0 (cosmicbugs), 0x42e0a0 (waterbugs): converts w x h pixels at (srcX, srcY) of src to theDstFmt at (dstX,
// dstY) of dst; an 8-bit source reads its colours from rgb ({r, g, b, 0} per index). 0 for an 8-bit destination or an
// unknown depth.
int ConvertPixels(const PixelFormat* theDstFmt, uint8_t* theDst, int theDstPitch, int theDstX, int theDstY,
	const PixelFormat* theSrcFmt, const uint8_t* theRgb, const uint8_t* theSrc, int theSrcPitch, int theSrcX,
	int theSrcY, int theWidth, int theHeight);
// 0x42db40 (cosmicbugs), 0x42e020 (waterbugs): the alpha bytes of w x h 32-bit pixels as an 8-bit mask
void ExtractAlpha(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, int theWidth, int theHeight);
// 0x42d060 (cosmicbugs), 0x42d540 (waterbugs): fills {l, t, r, b} (inclusive) of an 8-bit buffer
void Fill8(uint8_t* theDst, int theDstPitch, int l, int t, int r, int b, int theValue);
// 0x42dfa0 (cosmicbugs), 0x42e480 (waterbugs): an 8-bit mask's runs (RleHeader, then per row the runs R64Blit.h
// describes); malloc'd
uint8_t* Rle_Build(const uint8_t* theBits, int theWidth, int theHeight);

void InitSinCos(); // 0x42b9a0 (cosmicbugs), 0x42be80 (waterbugs)

// 0x42ba10 (cosmicbugs), 0x42bef0 (waterbugs), 0x42ba70 (cosmicbugs), 0x42bf50 (waterbugs): the sine and cosine of a
// whole angle in degrees from InitSinCos's tables (0x42b9a0 (cosmicbugs), 0x42be80 (waterbugs): 361
// entries each, the value times 1024), as a float
float SinDeg(int theAngle);
float CosDeg(int theAngle);

} // namespace r64

#endif
