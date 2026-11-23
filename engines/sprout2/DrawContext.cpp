#include "DrawContext.h"

#include "Surface.h"

namespace engine
{

DrawContext::DrawContext() : mDetailLevel(0)
{
	// 0x47d130 (feedingfrenzy2): the first state (0x47d0d0 (feedingfrenzy2)) pushed
	pushState(DrawState());
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

void DrawContext::popState()
{
	mStates.pop_back();
	if (back().mClipCount != getClipCount())
		restoreClips(back().mClipCount);
}

void DrawContext::pushClip(const RectI& theRect, ClipOp theOp)
{
	if (theRect.left < theRect.right && theRect.top < theRect.bottom)
	{
		Clip aClip;
		aClip.mRect = theRect;
		aClip.mOp = theOp;
		mClips.push_back(aClip);
		mClipRegion.clear();
	}
	back().mClipCount = getClipCount();
}

void DrawContext::restoreClips(int theCount)
{
	Clip anEmpty;
	anEmpty.mOp = CLIP_ADD;
	mClips.resize(theCount, anEmpty);
	mClipRegion.clear();
}

namespace
{

bool isEmpty(const RectI& theRect)
{
	return theRect.left >= theRect.right || theRect.top >= theRect.bottom;
}

RectI intersection(const RectI& a, const RectI& b)
{
	return RectI(maxOf(a.left, b.left), maxOf(a.top, b.top), minOf(a.right, b.right), minOf(a.bottom, b.bottom));
}

// The parts of a rectangle outside another, as up to four rectangles: the bands above and below, then the pieces left
// and right of it between them
void subtract(const RectI& theRect, const RectI& theHole, std::vector<RectI>& theParts)
{
	RectI aHole = intersection(theRect, theHole);
	if (isEmpty(aHole))
	{
		theParts.push_back(theRect);
		return;
	}
	if (theRect.top < aHole.top)
		theParts.push_back(RectI(theRect.left, theRect.top, theRect.right, aHole.top));
	if (aHole.bottom < theRect.bottom)
		theParts.push_back(RectI(theRect.left, aHole.bottom, theRect.right, theRect.bottom));
	if (theRect.left < aHole.left)
		theParts.push_back(RectI(theRect.left, aHole.top, aHole.left, aHole.bottom));
	if (aHole.right < theRect.right)
		theParts.push_back(RectI(aHole.right, aHole.top, theRect.right, aHole.bottom));
}

} // namespace

// port: the region covers the same pixels as the original's, but its rectangles may be cut differently (0x430c40,
// 0x430fa0 and 0x513fa8 (feedingfrenzy2) split them their own way), which only matters to a blitter whose result
// depends on where its clip rectangle starts; the looks pass compares them
const std::vector<RectI>& DrawContext::getClipRegion()
{
	if (!mClipRegion.empty())
		return mClipRegion;
	for (const Clip& aClip : mClips)
	{
		std::vector<RectI> aRegion;
		if (aClip.mOp == CLIP_ADD)
		{
			// The pieces of the rectangle that no rectangle of the region covers yet
			std::vector<RectI> aPieces(1, aClip.mRect);
			for (const RectI& aCovered : mClipRegion)
			{
				std::vector<RectI> aLeft;
				for (const RectI& aPiece : aPieces)
					subtract(aPiece, aCovered, aLeft);
				aPieces.swap(aLeft);
			}
			aRegion = mClipRegion;
			aRegion.insert(aRegion.end(), aPieces.begin(), aPieces.end());
		}
		else if (aClip.mOp == CLIP_INTERSECT)
		{
			if (mClipRegion.empty())
				aRegion.push_back(aClip.mRect);
			for (const RectI& aRect : mClipRegion)
			{
				RectI aPart = intersection(aRect, aClip.mRect);
				if (!isEmpty(aPart))
					aRegion.push_back(aPart);
			}
		}
		else if (aClip.mOp == CLIP_SUBTRACT)
		{
			for (const RectI& aRect : mClipRegion)
				subtract(aRect, aClip.mRect, aRegion);
		}
		mClipRegion.swap(aRegion);
	}
	return mClipRegion;
}

} // namespace engine
