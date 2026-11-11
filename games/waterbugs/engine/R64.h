#ifndef __COSMICBUGS_R64_H__
#define __COSMICBUGS_R64_H__

// Retro64's engine (Cosmic Bugs 2007, Water Bugs): plain C over global state, no classes with virtuals. This header
// holds its data structures with the original's field offsets in comments (the 32-bit layout; the port's own is
// whatever the compiler makes of these structs). games/cosmicbugs/docs/ENGINE.md maps the engine.

#include <cstdint>
#include <cstddef>

namespace r64
{

// 0x34 bytes: bits per pixel and each component's mask, bit count, shift and loss (8 - bits).
// Initialised by PixelFormat_Init (0x4012d0; 0x42e790 is the same code). A component reads as
// ((p & mask) >> shift) << loss and packs as (c >> loss) << shift.
struct PixelFormat
{
	int bpp;          // +0x00
	uint32_t rMask;   // +0x04
	int rBits;        // +0x08
	int rShift;       // +0x0c
	int rLoss;        // +0x10
	uint32_t gMask;   // +0x14
	int gBits;        // +0x18
	int gShift;       // +0x1c
	int gLoss;        // +0x20
	uint32_t bMask;   // +0x24
	int bBits;        // +0x28
	int bShift;       // +0x2c
	int bLoss;        // +0x30
};

void PixelFormat_Init(PixelFormat* f, int bpp, uint32_t r, uint32_t g, uint32_t b);

// 0x808 bytes: an 8-bit image's colours {r, g, b, 0} and the same converted to the screen's format: p16 and p32
// both point at conv (0x4013d0 sets them), whichever the screen's depth uses.
struct Palette
{
	uint8_t rgb[0x400];   // +0x000
	uint16_t* p16;        // +0x400
	uint32_t* p32;        // +0x404
	alignas(4) uint8_t conv[0x400]; // +0x408

	Palette();
	Palette(const Palette&) = delete;
	Palette& operator=(const Palette&) = delete;
};

struct Image;
void Image_Init(Image* img);

// 0x128 bytes (constructor 0x423980, Image_Free 0x4254c0)
struct Image
{
	uint8_t* bits;          // +0x00
	int w;                  // +0x04
	int h;                  // +0x08
	int clip[4];            // +0x0c  left, top, right, bottom (inclusive)
	int pitch;              // +0x1c  bytes
	int origW;              // +0x20  the size before the bank's packer cropped it
	int origH;              // +0x24
	int xOff;               // +0x28  where the cropped image sits in the original
	int yOff;               // +0x2c
	int dataSize;           // +0x30
	int handleX;            // +0x34  the point drawn at (x, y): Image_SetMidHandle makes it the middle
	int handleY;            // +0x38
	Image* prev;            // +0x3c  an ImageList's links
	Image* next;            // +0x40
	PixelFormat fmt;        // +0x44
	Palette* palette;       // +0x78  an 8-bit image's current colours
	uint8_t* rawPalette;    // +0x7c  an 8-bit bank image's colours as the .bbk lists them (Bank_AssignRawPalettes)
	Palette* defPalette;    // +0x80
	int hasMask;            // +0x84
	Image* mask;            // +0x88  8 bits per pixel
	int maskX;              // +0x8c
	int maskY;              // +0x90
	Image* defMask;         // +0x94
	uint8_t* rle;           // +0x98  the mask's runs (Image_BuildRle), when this image is a mask
	char name[0x34];        // +0x9c  "None"
	char* fileName;         // +0xd0
	int notOwned;           // +0xd4  the bits belong to a bank
	int paletteNumberPadding;  // +0xd8  the four bytes before paletteNumber: nothing reads them
	int paletteNumber;      // +0xdc
	int isMask;             // +0xe0
	int maskNamed;              // +0xe4
	Image* autoMask;        // +0xe8
	int bankDataSize;       // +0xec
	int bankPitch;          // +0xf0
	PixelFormat bankFmt;    // +0xf4

	Image() { Image_Init(this); }
	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
};

// An RLE mask's header (Rle_Build 0x42e480): the runs follow it
struct RleHeader
{
	uint32_t size;          // +0x0  bytes, the header included
	uint32_t width;         // +0x4
	uint32_t height;        // +0x8
};

// 0x423980: the constructor's body (the original constructs malloc'd memory in place; the port news an Image and
// Image_Destroy plus delete stands for Image_Destroy plus free)
void Image_Init(Image* img);
void Image_Free(Image* img);                                                 // 0x4254c0
void Image_Create(Image* img, int theWidth, int theHeight, const PixelFormat* fmt);         // 0x425520
void Image_CreateCopy(Image* img, const uint8_t* src, int spitch, int theWidth, int theHeight, const PixelFormat* fmt); // 0x4255d0
void Image_BuildRle(Image* img);                                             // 0x425630
void Image_Destroy(Image* img);                                              // 0x425d00
void Image_Copy(Image* img, const Image* src);                               // 0x425d60
void Image_SetMidHandle(Image* img);                                         // 0x423a50
int Image_DrawX(const Image* img);                                           // 0x423a70
int Image_DrawY(const Image* img);                                           // 0x423a80
void Image_SetPalette(Image* img, Palette* pal);                             // 0x423b00
int Image_ConvertTo(Image* img, const PixelFormat* fmt, const uint8_t* rgb); // 0x423b20
void Image_FreeRle(Image* img);                                              // 0x423bf0
int Image_MaskColumnSet(const Image* img, int x);                            // 0x425470

// The drawing methods 0x423c20..0x4252f0. dst is the destination's first byte, dpitch its pitch in bytes and fmt
// its format (null: the image's own); clip is {left, top, right, bottom}, inclusive. (x, y) is where the image's
// handle lands. Each picks a blitter by the destination's depth, the image's depth and whether it has a mask and
// that mask has runs.
void Image_Blit(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y); // 0x423c20
void Image_BlitMasked(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y); // 0x423f50
void Image_BlitFill(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t colour);                                                        // 0x4241f0
void Image_BlitFillAlpha(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t colour, uint32_t alpha);                                        // 0x424360
void Image_BlitBlend(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y); // 0x4244f0
void Image_BlitAlpha(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y); // 0x424700
void Image_BlitAlphaFade(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y,
	uint32_t alpha);                                                         // 0x424a90
void Image_BlitAdd(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, const int* clip, int x, int y); // 0x424cc0
void Image_BlitRect(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	int theWidth, int theHeight);                                                           // 0x425010
void Image_BlitSubRect(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	int theWidth, int theHeight);                                                           // 0x4250f0
void Image_BlitRotated(Image* img, uint8_t* dst, int dpitch, const PixelFormat* fmt, int x, int y, int angle,
	const int* clip);                                                        // 0x4252f0

// 0x42bfb0: clips a w x h rectangle at (x, y) to {clipL, clipT, clipR, clipB} (inclusive); 1 when some of it is left
int ClipBlit(int clipL, int clipT, int clipR, int clipB, int x, int y, int theWidth, int theHeight, int* outX, int* outY,
	int* outSx, int* outSy, int* outW, int* outH);
// 0x42c0f0: a palette's colours theFrom to theTo (either way, both included) shade from one colour to the other
void Palette_Gradient(Palette* thePalette, int theFrom, int theTo, uint8_t r0, uint8_t g0, uint8_t b0, uint8_t r1,
	uint8_t g1, uint8_t b1);

// 0x42e0a0: converts w x h pixels at (sx, sy) of src to dstFmt at (dx, dy) of dst; an 8-bit source reads its colours
// from rgb ({r, g, b, 0} per index). 0 for an 8-bit destination or an unknown depth.
int ConvertPixels(const PixelFormat* dstFmt, uint8_t* dst, int dpitch, int dx, int dy, const PixelFormat* srcFmt,
	const uint8_t* rgb, const uint8_t* src, int spitch, int sx, int sy, int w, int h);
// 0x42e020: the alpha bytes of w x h 32-bit pixels as an 8-bit mask
void ExtractAlpha(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, int sx, int sy, int w,
	int h);
// 0x42d540: fills {l, t, r, b} (inclusive) of an 8-bit buffer
void Fill8(uint8_t* dst, int dpitch, int l, int t, int r, int b, int value);
// 0x42e480: an 8-bit mask's runs (RleHeader, then per row the runs R64Blit.h describes); malloc'd
uint8_t* Rle_Build(const uint8_t* bits, int width, int height);

void InitSinCos(); // 0x42be80

// 0x42bef0, 0x42bf50: the sine and cosine of a whole angle in degrees from InitSinCos's tables (0x42be80: 361
// entries each, the value times 1024), as a float
float SinDeg(int theAngle);
float CosDeg(int theAngle);

} // namespace r64

#endif
