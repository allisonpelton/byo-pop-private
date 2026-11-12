#ifndef __TIPTOP_RES_H__
#define __TIPTOP_RES_H__

// TipTop's resource manager (TipTop.exe 0x438390-0x439d70): a table of named resources, each loaded from the
// data file TipTop.dat on first use, converted by type and reference-counted. The archive stores every byte
// with its bits permuted (0x438600); games/tiptop/tools/tiptopdat.py reads it the same way.

#include "Lowos.h"

#include <cstdint>

namespace res
{

// Converter types (0x4386d0)
enum
{
	TYPE_RAW = 0,     // kept as read (bkgd.mid)
	TYPE_PCX = 2,     // PCX, converted to the display format (0x438d40)
	TYPE_JPEG = 4,    // JPEG, converted to the display format (0x4391a0)
	TYPE_TEXT = 7,    // a copy with a NUL at the end (0x438a70)
	TYPE_OGG = 8,     // Ogg Vorbis, decoded to a 16-bit WAV image (0x4395a0)
};

// Groups the game uses
enum
{
	GROUP_IMAGES = 0,
	GROUP_MUSIC = 1,
	GROUP_SOUNDS = 2,
	GROUP_TEXTS = 3,
};

// A table entry (0x74 bytes in the original)
struct Entry
{
	uint8_t mFlags;        // +0x00: & 0x3f the source (3 = the data file), 0x40 loaded by a thread, 0x80 a mark
	const char* mName;     // +0x01
	uint32_t mType;        // +0x4c
	uint32_t mGroup;       // +0x50
	uint32_t mId;          // +0x54
	int32_t mSize;         // +0x48: the bytes read (-1 = disabled)
	void* mHandle;         // +0x58: the converted resource
	int32_t mRefCount;     // +0x5c
};

// A WAV image as the Ogg converter makes it (0x2c-byte header, then 16-bit samples)
struct WavHeader
{
	uint32_t mRiff;          // +0x00 "RIFF"
	uint32_t mRiffSize;      // +0x04
	uint32_t mWave;          // +0x08 "WAVE"
	uint32_t mFmt;           // +0x0c "fmt "
	uint32_t mFmtSize;       // +0x10
	uint16_t mFormat;        // +0x14
	uint16_t mChannels;      // +0x16
	uint32_t mRate;          // +0x18
	uint32_t mByteRate;      // +0x1c
	uint16_t mBlockAlign;    // +0x20
	uint16_t mBitsPerSample; // +0x22
	uint32_t mData;          // +0x24 "data"
	uint32_t mDataSize;      // +0x28
};

// 0x439c20: the table the game uses (the port's copy of 0x477060)
void SetTable(Entry* theTable);
Entry* GameTable();
// 0x439c00: the data file of source 0 (TipTop.dat, a game path for Platform::VFS)
void SetDataFile(int theSource, const char* theName);
// 0x439b30: loads (and references) theCount ids of a group. Returns 1 if one failed.
char Load(int theGroup, int theCount, ...);
// 0x439ba0: releases theCount ids of a group
void Free(int theGroup, int theCount, ...);
// 0x439a40: makes exactly these ids of a group the loaded ones. Returns 1 if one failed.
char KeepOnly(int theGroup, int theCount, ...);
// 0x439ca0: the loaded resource (an Image*, a WAV image, text or raw bytes), 0 if not loaded
void* Get(int theGroup, int theId);
// 0x439d70: the entry's index in the table
int IndexOf(int theGroup, int theId);
// The size the archive read gave (entry +0x48)
int32_t SizeOf(int theGroup, int theId);
// BYOPOP_SOUNDLOG: the name an entry of the archive is stored under (the case TipTop.dat spells it in, which the table's
// name matches without regard to case), or null while the archive's table is not read
const char* StoredName(int theGroup, int theId);

// The converters, for the game's own use
lowos::Image* ConvertPcx(const uint8_t* theFile, uint32_t theSize);
lowos::Image* ConvertJpeg(const uint8_t* theFile, uint32_t theSize);

} // namespace res

#endif
