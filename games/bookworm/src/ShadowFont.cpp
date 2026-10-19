#include "ShadowFont.h"
#include "Graphics.h"

#include <stdlib.h>

using namespace Sexy;

// 0x433250: shares the source font's data (ImageFont copy constructor)
ShadowFont::ShadowFont(const ImageFont& theFont, const Color& theShadowColor, const Point& theOffset) :
	ImageFont(theFont)
{
	mShadowColor = theShadowColor;
	mShadowOffset = theOffset;
}

// 0x433200
ShadowFont::ShadowFont(const ShadowFont& theFont) :
	ImageFont(theFont)
{
	mShadowColor = theFont.mShadowColor;
	mShadowOffset = theFont.mShadowOffset;
}

// 0x4330c0 (the ImageFont destructor)
ShadowFont::~ShadowFont()
{
}

// 0x4330d0: the width as drawn (measured by drawing into a Graphics without a destination, so kerning
// counts exactly as in DrawString), plus the shadow's horizontal offset
int ShadowFont::StringWidth(const SexyString& theString)
{
	Graphics aGraphics(NULL);
	int aWidth;
	DrawStringEx(&aGraphics, 0, 0, theString, Color(), NULL, NULL, &aWidth);
	return abs(mShadowOffset.mX) + aWidth;
}

// 0x433160: the shadow in mShadowColor at the offset (when there is one), then the text
void ShadowFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor,
	const Rect& theClipRect)
{
	if ((mShadowOffset.mX != 0) || (mShadowOffset.mY != 0))
		ImageFont::DrawStringEx(g, theX + mShadowOffset.mX, theY + mShadowOffset.mY, theString, mShadowColor,
			&theClipRect, NULL, NULL);
	ImageFont::DrawStringEx(g, theX, theY, theString, theColor, &theClipRect, NULL, NULL);
}

// 0x4332a0
Font* ShadowFont::Duplicate()
{
	return new ShadowFont(*this);
}
