#ifndef __JB_BLIT_H__
#define __JB_BLIT_H__

// The software blitter's pixel library (0x447000..0x514000, CPU-dispatched; only the functions the image and the
// fills call, in their plain C paths). Pixels are 4 bytes, B G R A in memory; 32-bit images are premultiplied by
// their alpha (0x455780) before they are drawn. Every product of two bytes is divided by 255 the library's way,
// Div255.
//
// The functions return 0, or a negative code for a null pointer (-8), a bad pitch (-14) or an empty size (-6);
// nothing in the engine reads the codes.

#include <cstdint>

namespace jb
{
namespace blit
{

// (x + 1 + (x >> 8)) >> 8: x / 255 rounded, for x up to 255 * 255
inline uint32_t Div255(uint32_t x)
{
	return (x + 1 + (x >> 8)) >> 8;
}

// 0x451178: a 32-bit image, its pitch rounded up to 32 bytes
uint8_t* Malloc32(int theWidth, int theHeight, int* thePitch);
// 0x4511fc: a 24-bit image, its pitch rounded up to 32 bytes
uint8_t* Malloc24(int theWidth, int theHeight, int* thePitch);
// 0x455974
void Free(uint8_t* thePixels);

// 0x4556e0: one row's colours multiplied by their alpha
void PremultiplyRow(const uint8_t* theSrc, uint8_t* theDst, int theWidth);
// 0x455780: in place
int Premultiply(uint8_t* thePixels, int thePitch, int theWidth, int theHeight);
// 0x454400: one row of theSrc over theUnder into theDst; theSaturate adds the premultiplied colour (mode 6),
// otherwise the under colour is premultiplied first
void OverRow(const uint8_t* theSrc, const uint8_t* theUnder, uint8_t* theDst, int theWidth, bool theSaturate);
// 0x455338: the blend dispatcher; only mode 6 (premultiplied over, saturating) is used
int Blend(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theUnder, int theUnderPitch, uint8_t* theDst,
	int theDstPitch, int theWidth, int theHeight, int theMode);
// 0x4557f4: B G R copied, the alpha bytes of theDst kept
int CopyRgb(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theWidth,
	int theHeight);
// 0x451c58: every byte multiplied by theValue's bytes (0x50e17c when the four are the same)
int MulC(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theValue, uint8_t* theDst, int theDstPitch,
	int theWidth, int theHeight);
// 0x50e2f0 (through 0x45226c): B G R multiplied by theValue's first three bytes
int MulCRgb(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theValue, uint8_t* theDst, int theDstPitch,
	int theWidth, int theHeight);
// 0x451df8 with scale 0: theValue's B G R added, saturating (the other scales are not used)
int AddCRgb(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
// 0x451ae0: B G R set to theValue's
int SetRgb(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
// 0x45194c: all four bytes set
int Set4(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
// 0x453ef8 with mode 0 (0x4534a0): theSrc times theSrcAlpha plus theUnder times theUnderAlpha times the rest, for
// all four bytes
int AlphaCompC(const uint8_t* theSrc, int theSrcPitch, int theSrcAlpha, const uint8_t* theUnder,
	int theUnderPitch, int theUnderAlpha, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight);
// 0x4517f8 with nearest-neighbour interpolation (1): theSrcRect of theSrc resized into theDst
void ResizeNearest(const uint8_t* theSrc, int theSrcWidth, int theSrcHeight, int theSrcPitch, int theSrcLeft,
	int theSrcTop, int theSrcRight, int theSrcBottom, uint8_t* theDst, int theDstPitch, int theDstWidth,
	int theDstHeight, double theFactorX, double theFactorY);

} // namespace blit
} // namespace jb

#endif
