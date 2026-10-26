#ifndef __COSMICBUGS_R64BANK_H__
#define __COSMICBUGS_R64BANK_H__

// Retro64's image banks (0x425de0..0x428640): a .bbk names the images and palettes of a .dat file (XML: BANK
// datafile, IMAGELIST of IMAGE, PALETTELIST of PALETTE); Bank_Load reads both. Mode 0 keeps the pixels as stored
// (art8: 8-bit images through palettes the screen's format converts), mode 1 converts each image to the screen's
// format and gives it a mask from its alpha (art: 32-bit images). An image named mask_<name> becomes <name>'s mask.

#include "R64.h"

#include <vector>

namespace r64
{

// The original's lists are MSVC std::lists, read by position (0x426430, 0x426480); vectors read the same.
struct Bank
{
	int mode;                           // +0x00  Bank_Load's mode
	std::vector<Image*> images;         // +0x04  (+0x10 is the same as an array, for Bank_ImageAt)
	std::vector<uint8_t*> rawPalettes;  // +0x14  {r, g, b, unused} x 256 as the .bbk lists them
	int palettesBuilt;                  // +0x20
	std::vector<Palette*> palettes;     // +0x24  the raw palettes in the screen's format
	uint8_t* data;                      // +0x30  every image's pixels
	uint32_t dataSize;                  // +0x34
	char dataPath[0x200];               // +0x38  the .dat: the .bbk's folder and its datafile

	Bank();   // 0x427790
	~Bank();  // 0x427560
	Bank(const Bank&) = delete;
	Bank& operator=(const Bank&) = delete;
};

void Bank_Reset(Bank* theBank);                                      // 0x4261b0
void Bank_Clear(Bank* theBank);                                      // 0x426250
int Bank_Alloc(Bank* theBank, uint32_t theSize);                     // 0x425de0
Image* Bank_ImageAt(const Bank* theBank, uint32_t theIndex);         // 0x425eb0
int Bank_Count(const Bank* theBank);                                 // cb:0x4259f0
uint8_t* Bank_RawPaletteAt(const Bank* theBank, uint32_t theIndex);  // 0x426430
Palette* Bank_PaletteAt(const Bank* theBank, uint32_t theIndex);     // 0x426480
int Bank_LoadRaw(Bank* theBank, const char* thePath);                // 0x4264d0
int Bank_LoadConverted(Bank* theBank, const char* thePath, const PixelFormat* theFmt); // 0x4265d0
int Bank_LinkMasks(Bank* theBank);                                   // 0x427050
int Bank_AssignPalettes(Bank* theBank);                              // 0x427270
int Bank_AssignRawPalettes(Bank* theBank);                           // 0x427340
Image* Bank_FindImage(const Bank* theBank, const char* theName, int* theIndex); // 0x4273d0
int Bank_IndexOf(const Bank* theBank, const char* theName);          // 0x4275c0
Image* Bank_FindImage2(const Bank* theBank, const char* theName);    // 0x427820
int Bank_BuildPalettes(Bank* theBank, const PixelFormat* theFmt);    // 0x427c20
int Bank_ParseXml(Bank* theBank, const char* thePath);               // 0x427df0
int Bank_Load(Bank* theBank, const char* thePath, int theMode, const PixelFormat* theFmt); // 0x4284f0
int Bank_Reconvert(Bank* theBank, const PixelFormat* theFmt);        // 0x428640

// 0x401a80, 0x401b10: n {r, g, b, unused} colours in theFmt, as 32- or 16-bit pixels
int PixelFormat_PackPalette32(const PixelFormat* theFmt, uint32_t* dst, const uint8_t* rgb, int n);
int PixelFormat_PackPalette16(const PixelFormat* theFmt, uint16_t* dst, const uint8_t* rgb, int n);

} // namespace r64

#endif
