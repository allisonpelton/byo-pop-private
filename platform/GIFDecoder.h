// A GIF decoder for the image loader (ImageLoader.cpp): the first frame, pixel for pixel as stb_image 2.30 decodes it, in
// a fraction of the time (stb's took 90 to 106 ms of Mummy Maze's start for its 237 GIFs, docs/MEMORY.md).

#ifndef PLATFORM_GIFDECODER_H
#define PLATFORM_GIFDECODER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Platform
{

// Decodes theData's first frame into theBits (0xAARRGGBB, theWidth x theHeight, the logical screen's size) with the result
// stb_image gives it, its quirks included: pixels no frame pixel reached are 0, or with a background index above 0 that
// colour opaque with its red and blue swapped; transparent pixels stay 0. theHasAlpha is whether any pixel's alpha is not
// 255. False, with nothing decoded, for anything that is not a GIF or that stb would refuse or decode some other way: the
// caller leaves those to stb.
bool DecodeGIF(const uint8_t* theData, size_t theSize, int& theWidth, int& theHeight, std::vector<uint32_t>& theBits, bool* theHasAlpha);

}

#endif
