#ifndef __QUANTIZE_H__
#define __QUANTIZE_H__

#include "Common.h"

namespace Sexy
{

bool Quantize8Bit(const ulong* theSrcBits, int theWidth, int theHeight, uchar* theDestColorIndices, ulong* theDestColorTable);

// port: Quantize8Bit's result for pixels that came from a GIF's colour indices (Platform::GIFIndices), without its search a
// pixel. theSrcBits[i] should be theColors[theIndices[i]], or with theMaskIndices that colour's red, green and blue with
// theMaskColors[theMaskIndices[i]]'s low byte as alpha. Each pixel is checked against theSrcBits as it is numbered, so
// the result is always Quantize8Bit's on theSrcBits: 1 with its indices and table (each distinct colour numbered by its
// first pixel, the entries past the last left as they were), 0 where it refuses (a 257th colour), and -1 when a pixel is
// not what its indices give, with nothing usable written (Quantize8Bit is left to do it).
int QuantizeIndices(const ulong* theSrcBits, int theWidth, int theHeight, const uint8_t* theIndices, const uint32_t* theColors,
	const uint8_t* theMaskIndices, const uint32_t* theMaskColors, uchar* theDestColorIndices, ulong* theDestColorTable);

}

#endif //__QUANTIZE_H__