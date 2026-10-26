#ifndef __DYNOMITE_SPRITE_H__
#define __DYNOMITE_SPRITE_H__

// The game's own sprite toolkit (0x435b00-0x437500, docs/GAME_NOTES.md section 2): pictures, cells of a sheet, picture
// fonts, masked pictures and sounds, drawn into a current Graphics with a global drawing offset.

#include "Graphics.h"
#include "MemoryImage.h"
#include "Rect.h"
#include <string>

namespace Sexy
{

class SexyAppBase;

// The current Graphics (0x5304dc) and the drawing offset (0x5304e0/e4) every Draw adds
void		SetDrawGraphics(Graphics* g);				// 0x4360f0
Graphics*	GetDrawGraphics();							// 0x436100
void		SetDrawOffset(const Point& theOffset);		// 0x436110
Point		GetDrawOffset();							// 0x436130
void		AddDrawOffset(const Point& theOffset);		// 0x436150

// vtable 0x4fcc88, 0x18 bytes
class Pic
{
public:
	MemoryImage*	mImage;			// +4
	SexyAppBase*	mApp;			// +8
	int				mWidth;			// +0xc
	int				mHeight;		// +0x10
	bool			mShared;		// +0x14 the image is not this Pic's to delete

	Pic() : mImage(NULL), mApp(NULL), mWidth(0), mHeight(0), mShared(false) {}
	// 0x437210: images\<name> through the app (a popup and the load-failed flag if it is missing)
	Pic(SexyAppBase* theApp, const std::string& thePath, bool palletize);
	// 0x435ee0: a picture of theWidth x theHeight made from 32-bit pixels
	Pic(SexyAppBase* theApp, const unsigned int* theBits, int theWidth, int theHeight);
	// 0x435fc0: a copy with its own image
	Pic(const Pic& theOther);
	virtual ~Pic();										// 0x4360c0

	void			Load(const std::string& thePath, bool palletize);	// 0x4370a0
	unsigned int*	GetBits() { return (unsigned int*) mImage->GetBits(); }
	void			BitsChanged() { mImage->BitsChanged(); }

	void			Draw(int theX, int theY);			// 0x436170
	// 0x436a40: copies centred on every point of the line from (x1, y1) to (x2, y2)
	void			DrawLine(int theX1, int theY1, int theX2, int theY2);
	// 0x436b50: stretched over the rectangle (x1, y1)-(x2, y2)
	void			DrawStretched(int theX1, int theY1, int theX2, int theY2, bool fast);
	// 0x4361a0: the alpha of the pixels under thePic at theX, theY scaled by thePic's alpha
	void			MultiplyAlpha(Pic* thePic, int theX, int theY);
	// 0x436260: a new picture of theColor around this one's opaque pixels, theWidth passes wide, each pass
	// outward a step of 255 / theWidth fainter
	Pic*			MakeGlow(const Color& theColor, int theWidth);
};

// vtable 0x4fcc8c, 0x20 bytes: a rectangle of a Pic's image
class Cell
{
public:
	Rect			mSrc;			// +4
	int				mWidth;			// +0x14 the width of a draw (a Rect cell: its width plus one)
	int				mHeight;		// +0x18
	MemoryImage*	mImage;			// +0x1c

	// 0x436cb0: cell theIndex of a grid of theCellWidth x theCellHeight cells, row by row
	Cell(Pic* thePic, int theCellWidth, int theCellHeight, int theIndex);
	// 0x436d40: the inclusive rectangle (x1, y1)-(x2, y2)
	Cell(Pic* thePic, int theX1, int theY1, int theX2, int theY2);
	virtual ~Cell() {}

	void			Draw(int theX, int theY);								// 0x4365a0
	void			DrawPart(int theX, int theY, int theSrcX, int theSrcY, int theWidth, int theHeight); // 0x436d90
	void			DrawStretched(int theX1, int theY1, int theX2, int theY2);	// 0x436df0
};

// vtable 0x4fcc90: a font of glyphs cut from a strip
class PicFont : public Pic
{
public:
	Cell*			mGlyphs[256];	// +0x18
	int				mAdvance[256];	// +0x418
	int				mSpaceWidth;	// +0x818

	// 0x437250: theCharset lists the strip's glyphs in order
	PicFont(SexyAppBase* theApp, const std::string& thePath, const char* theCharset, int theSpaceWidth, bool palletize);
	virtual ~PicFont();												// 0x4365d0

	void			Cut(const char* theCharset);						// 0x436f70
	void			AdjustAdvances(int theDelta);						// 0x436650
	void			Draw(const char* theText, int theX, int theY);		// 0x436670
	int				Width(const char* theText);							// 0x4366d0
};

// vtable 0x4fcc94: a picture with the alpha of <name>_
class MaskedPic : public Pic
{
public:
	Pic*			mMask;			// +0x18

	MaskedPic(SexyAppBase* theApp, const std::string& thePath);		// 0x4372e0
	MaskedPic(const MaskedPic& theOther);								// 0x436720
	virtual ~MaskedPic();												// 0x4367a0

	// 0x436800: the alpha from the mask's colour channels (theChannels: 1 red, 2 green, 4 blue; 0xff all, 0
	// none). The fossils' masks keep a fragment in each channel.
	void			AlphaFromColour(int theChannels);
};

// vtable 0x4fcc5c, 0xc bytes
class GameSound
{
public:
	SexyAppBase*	mApp;			// +4
	int				mId;			// +8

	GameSound(SexyAppBase* theApp, const std::string& thePath);		// 0x435b50
	virtual ~GameSound() {}

	void			Play();												// 0x435b10
};

}

#endif
