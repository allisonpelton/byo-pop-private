#ifndef __COSMICBUGS_R64FLIC_H__
#define __COSMICBUGS_R64FLIC_H__

// The exe's FLIC reader (0x40c1a0..0x40c890): the whole file is kept in memory after a 0x14-byte handle, and each
// call decodes the next frame onto an 8-bit buffer, the palette chunks going to gFlicPalette.

#include <cstddef>
#include <cstdint>

namespace r64
{

// The original's handle is the first 0x14 bytes of one block whose size the header gives, the file from its first
// frame following; here the fields are kept apart and mem is that block (its first 0x14 bytes unused)
struct Flic
{
	uint16_t frames;    // +0x00
	uint16_t width;     // +0x02
	uint16_t height;    // +0x04
	uint16_t speed;     // +0x06  in 1/70 s
	uint16_t frame;     // +0x08  frames decoded since the first (the ring frame included)
	uint16_t unk0a;     // +0x0a
	uint32_t offset;    // +0x0c  of the next frame's header, from the handle
	uint16_t fd;        // +0x10  0: read from memory (the port only reads from memory)
	uint16_t unk12;     // +0x12
	uint8_t* mem;       // the port's
	uint32_t memSize;
};

extern uint8_t gFlicPalette[0x300]; // 0x6e57a0: r, g, b per index
extern int gFlicSkipPalette;        // 0x6e5aa0: palette chunks are read past and not kept

// 0x40c1a0 (theMode 0): a FLI or FLC file's handle, or null
Flic* Flic_Open(const uint8_t* theFile, size_t theFileSize);
// 0x40c690: decodes the next frame onto theDst (theDPitch bytes a row); after the ring frame it goes on with the
// second
void Flic_NextFrame(Flic* theFlic, uint8_t* theDst, int theDPitch);
// 0x40c890
void Flic_Close(Flic* theFlic);

} // namespace r64

#endif
