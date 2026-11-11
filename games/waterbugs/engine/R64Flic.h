#ifndef __COSMICBUGS_R64FLIC_H__
#define __COSMICBUGS_R64FLIC_H__

// The exe's FLIC reader (0x40c390..0x40ca80): the whole file is kept in memory after a 0x14-byte handle, and each
// call decodes the next frame onto an 8-bit buffer, the palette chunks going to gFlicPalette.

#include <cstddef>
#include <cstdint>

namespace r64
{

// The original's handle is the first 0x14 bytes of one block whose size the header gives, the file from its first
// frame following; here the fields are kept apart and mem is that block (its first 0x14 bytes unused)
struct Flic
{
	uint16_t mFrames;    // +0x00
	uint16_t mWidth;     // +0x02
	uint16_t mHeight;    // +0x04
	uint16_t mSpeed;     // +0x06  in 1/70 s
	uint16_t mFrame;     // +0x08  frames decoded since the first (the ring frame included)
	uint16_t mOffsetPadding;    // +0x0a  the two bytes before offset: nothing reads them
	uint32_t mOffset;    // +0x0c  of the next frame's header, from the handle
	uint16_t mFd;        // +0x10  0: read from memory (the port only reads from memory)
	uint16_t mEndPadding;       // +0x12  nothing reads it
	uint8_t* mMem;       // the port's
	uint32_t mMemSize;
};

extern uint8_t gFlicPalette[0x300]; // 0x6e57a0: r, g, b per index
extern int gFlicSkipPalette;        // 0x6e5aa0: palette chunks are read past and not kept

// 0x40c390 (theMode 0): a FLI or FLC file's handle, or null
Flic* Flic_Open(const uint8_t* theFile, size_t theFileSize);
// 0x40c880: decodes the next frame onto theDst (theDPitch bytes a row); after the ring frame it goes on with the
// second
void Flic_NextFrame(Flic* theFlic, uint8_t* theDst, int theDPitch);
// 0x40ca80
void Flic_Close(Flic* theFlic);

} // namespace r64

#endif
