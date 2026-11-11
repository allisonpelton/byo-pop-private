#ifndef __COSMICBUGS_R64BLIT_H__
#define __COSMICBUGS_R64BLIT_H__

// Retro64's software blitters (0x42fe50..0x401000), the 8-bit and 16-bit ones: the port's screen is 16-bit (the
// original asked DirectDraw for 16 bits per pixel, DAT_00460008, and never changed it), so the 24- and 32-bit
// blitters the wrappers 0x4295f0..0x42a210 also pick from are never reached and are not translated. A wrapper's
// third argument is the destination's format; the 16-bit blitters read its masks.
//
// Arguments keep the original order. dst/src/mask are a buffer's first byte, pitches are in bytes, (dstX, dstY) is the
// destination point, (srcX, srcY) the first source pixel and (maskX, maskY) the mask's offset from the source. A mask is 8
// bits per pixel: 0 transparent, 0xff opaque, anything between a blend; alpha is a further 0..255 factor.
//
// RLE masks (Image_BuildRle 0x425630, Image::rle) are a header {unused, width, height} and per row runs of
// {skip, n, n alpha bytes padded to 4, solid}: skip transparent pixels, n blended ones, then solid opaque ones.

#include <cstdint>

namespace r64
{

struct PixelFormat;
struct Palette;

// 0x42fe50: an 8-bit copy
void Copy8(uint8_t* theDst, int theDstX, int theDstY, int theDstPitch, int, const uint8_t* theSrc, int theSrcX, int theSrcY, int theSrcPitch, int,
	int theWidth, int theHeight);
// 0x42fee0: an 8-bit copy where 0 is transparent
void Copy8Key(uint8_t* theDst, int theDstX, int theDstY, int theDstPitch, int, const uint8_t* theSrc, int theSrcX, int theSrcY, int theSrcPitch, int,
	int theWidth, int theHeight);

// 0x42ffb0: a 16-bit copy
void Copy16(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, int theWidth, int theHeight);
// 0x430020: a 16-bit copy where the mask is not 0
void Copy16Mask(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY,
	const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight);
// 0x430260: a colour through a mask
void Fill16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX, int theSrcY, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight, uint16_t theColour);
// 0x430870: a colour through a mask, faded by alpha
void Fill16MaskAlpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX, int theSrcY,
	const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight, uint16_t theColour, uint8_t theAlpha);
// 0x430ef0: a blend through a mask (0xff blends too)
void Blend16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight);
// 0x4315a0: a blend through a mask, 0xff copies
void Alpha16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight);
// 0x431d00: Alpha16Mask faded by alpha
void Alpha16MaskFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc,
	int theSrcPitch, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight, int theAlpha);
// 0x432440: additive where the mask is not 0
void Add16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight);
// 0x432970: rotated by angle degrees about (x, y), where the mask is not 0, clipped to clip (inclusive)
void Rotate16(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, int theWidth, int theHeight, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int x, int y, int theAngle, const int* theClip);
// 0x42d590: Rotate16 for an 8-bit source and destination, where the source is not 0
void Rotate8(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, int theWidth, int theHeight, int x, int y, int theAngle,
	const int* theClip);
// 0x432be0: 8-bit through its palette
void Pal16(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX,
	int theSrcY, int theWidth, int theHeight);
// 0x432c70: 8-bit through its palette where the mask is not 0
void Pal16Mask(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX,
	int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight);
// 0x432ee0, 0x4335c0, 0x433db0, 0x434520: the 8-bit sources' Blend16Mask, Alpha16Mask, Alpha16MaskFade, Add16Mask
void PalBlend16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int w, int h);
void PalAlpha16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int w, int h);
void PalAlpha16MaskFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc,
	int theSrcPitch, const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int w, int h,
	int theAlpha);
void PalAdd16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int w, int h);
// 0x434a80: Rotate16 for an 8-bit source
void PalRotate16(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int w, int h,
	const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int x, int y, int theAngle, const int* theClip);

// The RLE blitters draw the whole image (the callers take them only unclipped and at full size)
// 0x434d00: copy, blended pixels copied too
void Rle16Copy(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const uint8_t* theRle);
// 0x434e20, 0x434fe0: a colour
void Rle16Fill(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theRle, uint16_t theColour);
void Rle16FillAlpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theRle,
	uint16_t theColour, uint32_t theAlpha);
// 0x435270 (solid pixels blended at 0xff), 0x435500 (solid copied), 0x435700 (faded), 0x4359e0 (additive)
void Rle16Blend(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const uint8_t* theRle);
void Rle16Alpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const uint8_t* theRle);
void Rle16AlphaFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const uint8_t* theRle, uint32_t theAlpha);
void Rle16Add(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const uint8_t* theRle);
// 0x435bf0, 0x435d10, 0x435fb0, 0x4361b0, 0x436490: the same for 8-bit sources
void RlePal16Copy(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, const uint8_t* theRle);
void RlePal16Blend(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, const uint8_t* theRle);
void RlePal16Alpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, const uint8_t* theRle);
void RlePal16AlphaFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc,
	int theSrcPitch, const Palette* thePal, const uint8_t* theRle, uint32_t theAlpha);
void RlePal16Add(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, const uint8_t* theRle);

} // namespace r64

#endif
