#include "DrawContext.h"

#include "Surface.h"

namespace engine
{

DrawContext::DrawContext()
{
}

DrawContext::~DrawContext()
{
}

void DrawContext::drawImage(Surface* theImage, float x, float y)
{
	RectI aSource(0, 0, theImage->getWidth(), theImage->getHeight());
	drawImage(theImage, aSource, x, y);
}

void DrawContext::drawImage(Surface* theImage)
{
	RectI aSource(0, 0, theImage->getWidth(), theImage->getHeight());
	drawImage(theImage, aSource, 0.0f, 0.0f);
}

void DrawContext::drawText(const char* theText, float x, float y)
{
	Font* aFont = back().mFont;
	if (!aFont)
		return;
	// The x87 keeps the pen in a register between the glyphs, so it is a double here, rounded where the original
	// stores it
	double aHorizontalFactor = back().mTextAlignH == TEXT_ALIGN_CENTER ? 0.5 : back().mTextAlignH == TEXT_ALIGN_RIGHT ? 1.0 : 0.0;
	float aScaleX = back().mScale.x;
	int aWidth = aFont->getStringWidth(theText);
	double aPenX = (double)x - aHorizontalFactor * ((double)aWidth * aScaleX);
	double aVerticalFactor = back().mTextAlignV == TEXT_ALIGN_MIDDLE ? 0.5 : back().mTextAlignV == TEXT_ALIGN_BOTTOM ? 1.0 : 0.0;
	float aScaleY = back().mScale.y;
	double aPenY = (double)y - aVerticalFactor * ((double)aFont->getHeight() * aScaleY);
	if (back().mTextAlignV == TEXT_ALIGN_TOP)
		aPenY = aPenY + (-(double)aFont->getTopOverlap() * aScaleY);
	else if (back().mTextAlignV == TEXT_ALIGN_BOTTOM)
		aPenY = aPenY - (-(double)aFont->getTopOverlap() * aScaleY);
	float aGlyphY = (float)aPenY;
	for (; *theText != '\0'; theText++)
	{
		char aChar = *theText;
		if (aFont->hasGlyph(aChar))
		{
			float aGlyphX = (float)(aPenX + (double)aScaleX * (aFont->getLeadingSpace(aChar) - aFont->getLeftOverlap()));
			drawImage(aFont->getImage(), aFont->getGlyphRect(aChar), aGlyphX, aGlyphY);
			aPenX = (double)aGlyphX +
				(double)(aFont->getGlyphWidth(aChar) + aFont->getTrailingSpace(aChar) - aFont->getRightOverlap()) * aScaleX;
		}
		else
			aPenX = aPenX + aFont->getSpaceWidth();
	}
}

void DrawContext::drawText(const char* theText)
{
	drawText(theText, 0.0f, 0.0f);
}

void DrawContext::drawChar(char theChar)
{
	char aText[2] = { theChar, '\0' };
	drawText(aText);
}

void DrawContext::fillRect(float theWidth, float theHeight)
{
	RectI aRect(0, 0, (int)theWidth, (int)theHeight);
	fillRect(aRect);
}

void DrawContext::drawRect(float theWidth, float theHeight)
{
	RectI aRect(0, 0, (int)theWidth, (int)theHeight);
	drawRect(aRect);
}

void DrawContext::restoreClips(int theCount)
{
	mClips.resize(theCount);
}

} // namespace engine
