#ifndef __JB_IMAGE_H__
#define __JB_IMAGE_H__

// The engine's image (0x426c20 (mahjongchina), 0x42cdb0 (mahjongjapan), vtable 0x51e3fc (mahjongjapan) over
// 0x51e410 (mahjongjapan), 0x38 bytes) and the software screen it draws to. The pixels are kept as a DIB (rows
// bottom-up, B G R or B G R A); before the first draw they are copied to a 32-bit top-down buffer
// (0x428680 (mahjongchina), 0x42e8b0 (mahjongjapan)), premultiplied when the image has alpha, and drawn from there with
// the blit library (Blit.h). See docs/ENGINE.md, "Engine classes on the device".
//
// Port: only the software display is ported; the Direct3D path (texture tiles +0x24, 0x4281a0 (mahjongchina),
// 0x42e3d0 (mahjongjapan), 0x428310 (mahjongchina), 0x42e540 (mahjongjapan), and the accelerated fills) is not, so the
// HardwareAcceleration setting changes nothing (docs/DEVIATIONS.md). JPEG and PNG are decoded with stb_image instead of
// the exe's libjpeg and PNG decoder.

#include "JBBase.h"

#include <cstdio>

namespace jb
{

class Display;

// 0x427870 (mahjongchina), 0x42daa0 (mahjongjapan)'s output
struct ImageInfo
{
	int mWidth = 0;
	int mHeight = 0;
	int mBits = 0;
	bool mKeyed = false;
	uint32_t mKey = 0;  // a COLORREF, 0x00BBGGRR
};

class Image
{
public:
	int mWidth = 0;                  // +0x04
	int mHeight = 0;                 // +0x08
	uint8_t* mBits = nullptr;        // +0x0c the DIB's rows, bottom-up
	bool mOwnsBits = false;          // +0x10 (otherwise they are a resource's)
	int mPitch = 0;                  // +0x14
	int mBitCount = 0;               // +0x18 24 or 32
	uint32_t mKey = 0;               // +0x1c the colour key as a pixel, 0x00RRGGBB
	bool mKeyed = false;             // +0x20
	bool mPrepared = false;          // +0x21
	uint8_t* mPixels = nullptr;      // +0x28 the 32-bit copy, top-down
	int mPixelPitch = 0;             // +0x2c
	bool mCreated = false;           // +0x30 a blank image made to draw into
	bool mFlipped = false;           // +0x31 mBits are top-down now (drawn into), flipped back before the next draw
	Display* mDisplay = nullptr;     // +0x34 the display that tracks it

	// 0x426c20 (mahjongchina), 0x42cdb0 (mahjongjapan)
	Image() {}
	// 0x42ce00 (mahjongjapan)
	virtual ~Image();

	// 0x426d80 (mahjongchina), 0x42cf80 (mahjongjapan): everything freed, and the display stops tracking it
	void Release();
	// 0x426db0 (mahjongchina), 0x42cfc0 (mahjongjapan): a resource of type BITMAP, "JPG", "TGA" or "PNG"
	bool LoadResource(int theId, Module theModule);
	// 0x426e70 (mahjongchina), 0x42d080 (mahjongjapan): theType 0 BMP (a resource: no file header), 1 JPEG, 2 TGA, 3
	// PNG
	bool Decode(const uint8_t* theData, uint32_t theSize, int theType);
	// 0x426f00 (mahjongchina), 0x42d110 (mahjongjapan)
	bool DecodeBmp(const uint8_t* theData);
	// 0x426f50 (mahjongchina), 0x42d160 (mahjongjapan)
	bool DecodeJpeg(const uint8_t* theData, uint32_t theSize);
	// 0x427110 (mahjongchina), 0x42d320 (mahjongjapan): true when the data was used up exactly
	bool DecodeTga(const uint8_t* theData, int theSize);
	// 0x4271e0 (mahjongchina), 0x42d3f0 (mahjongjapan): one row of an RLE TGA; theState carries a packet over to the
	// next row
	bool DecodeTgaRow(const uint8_t*& theSrc, uint8_t* theDst, int theWidth, uint8_t& theState, int& theLeft);
	// 0x427370 (mahjongchina), 0x42d580 (mahjongjapan)
	bool DecodePng(const uint8_t* theData, uint32_t theSize);
	// 0x427460 (mahjongchina), 0x42d670 (mahjongjapan): by the name's extension (.bmp .jpg .png .tga); without one each
	// is tried
	bool LoadFile(const char* thePath);
	// 0x427710 (mahjongchina), 0x42d940 (mahjongjapan): a BMP file
	bool LoadBmpFile(FILE* theFile);
	// 0x4277f0 (mahjongchina), 0x42da20 (mahjongjapan): mBits for mBitCount
	void AllocBits();
	// 0x427840 (mahjongchina), 0x42da70 (mahjongjapan): theKey a COLORREF
	void SetColourKey(uint32_t theKey);
	// 0x427870 (mahjongchina), 0x42daa0 (mahjongjapan)
	void GetInfo(ImageInfo& theInfo);
	// 0x4278b0 (mahjongchina), 0x42dae0 (mahjongjapan): to the screen; theScaleX and theScaleY in percent
	bool Draw(int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha);
	// 0x427aa0 (mahjongchina), 0x42dcd0 (mahjongjapan): to the 32-bit top-down pixels theDst, clipped to 800x600
	bool Blit(int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha, uint8_t* theDst,
		int theDstPitch);
	// 0x428170 (mahjongchina), 0x42e3a0 (mahjongjapan)
	bool EnsurePrepared();
	// 0x428680 (mahjongchina), 0x42e8b0 (mahjongjapan)
	bool Prepare();
	// 0x428710 (mahjongchina), 0x42e940 (mahjongjapan)
	void FreePixels();
	// 0x428770 (mahjongchina), 0x42e9a0 (mahjongjapan): theHeight rows of mBits to theDst, which steps back by
	// theDstPitch (theDstBits 32 only)
	void ConvertRows(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theDstBits,
		int theWidth, int theHeight);
	// 0x428930 (mahjongchina), 0x42eb60 (mahjongjapan)
	void Create(int theWidth, int theHeight, Display* theDisplay);
	// 0x428960 (mahjongchina), 0x42eb90 (mahjongjapan): mBits made top-down to be drawn into
	void BeginDrawing();
	// 0x428980 (mahjongchina), 0x42ebb0 (mahjongjapan): theColour 0xAABBGGRR
	void Fill(uint32_t theColour);
	// 0x4289e0 (mahjongchina), 0x42ec10 (mahjongjapan): mBits upside down
	void FlipRows();
	// 0x428aa0 (mahjongchina), 0x42ecd0 (mahjongjapan): from Resource.dll, and prepared
	bool LoadDll(int theId);
	// 0x428ad0 (mahjongchina), 0x42ed00 (mahjongjapan)
	bool LoadDllKeyed(int theId, uint32_t theKey);
	// 0x428af0 (mahjongchina), 0x42ed20 (mahjongjapan): into theTarget's bits (after its BeginDrawing)
	bool DrawTo(Image* theTarget, int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY,
		int theAlpha);
	// 0x428b40 (mahjongchina), 0x42ed70 (mahjongjapan): a 24-bit BMP file
	bool SaveBmp(const char* thePath);
};

// The software screen, 800x600, 32-bit top-down (0x5d8098 (mahjongchina), 0x5d8f30 (mahjongjapan), pitch
// 0x5d809c (mahjongchina), 0x5d8f34 (mahjongjapan))
extern uint8_t* gScreen;
extern int gScreenPitch;

// 0x427dc0 (mahjongchina), 0x42dff0 (mahjongjapan): the rect (the screen when null) filled; theColour a COLORREF
void ScreenFillRect(const Rect* theRect, uint32_t theColour);
// 0x427ef0 (mahjongchina), 0x42e120 (mahjongjapan): the rect darkened by theAlpha and theColour added at theAlpha
// (theRect is clipped to the screen, as the original clipped its caller's rect)
void ScreenFillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour);
// 0x426c90 (mahjongchina), 0x42ce90 (mahjongjapan): the blit's temporary buffer
void FreeBlitBuffer();

} // namespace jb

#endif
