#ifndef __POINTS_H__
#define __POINTS_H__

// Points.h: a score text that rises from the gems that were cleared (Sexy::Points, 0x5cf05a-0x5cf1eb; the object is 0xcc
// bytes). Board::mPoints holds them; Board::Update moves them and Board::Draw draws them. The text is drawn once into
// a small image, and the image is drawn at the size that the text has now. The offsets in the comments are the
// original's, from the start of the object.

#include "Widget.h"
#include "Color.h"
#include <string>

namespace Sexy
{

class WinDMApp;
class Font;
class MemoryImage;

class Points : public Widget
{
public:
	WinDMApp*		mApp;               // +0x88
	int				mLife;              // +0x8c updates left; the text fades in the last 18
	std::string		mText;              // +0x90
	Font*			mFont;              // +0xac
	MemoryImage*	mImage;             // +0xb4 the text, drawn once
	float			mScale;             // +0xb8 the size of the text now (1 is the image's own size)
	float			mTargetScale;       // +0xbc the size that it grows towards
	float			mVelocity;          // +0xc0 how fast the size changes
	float			mFade;              // +0xc4 the size's change is multiplied by this every update
	float			mRise;              // +0xc8 how strongly the size is pulled towards the target

public:
	Points(WinDMApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife, int theJustify,
		Color theColor);                                    // 0x5cf1eb (theJustify: 0 centred on theX, 1 ends at theX)
	virtual ~Points();                                      // 0x5cf1cc

	virtual void	Draw(Graphics* g) override;             // 0x5cf05a
};

}

#endif
