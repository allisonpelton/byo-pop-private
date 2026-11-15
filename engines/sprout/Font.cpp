#include "Font.h"
#include "App.h"
#include "Build.h"
#include "ResourceManager.h"

using namespace engine;

// 0x43d770 (wordharmony), 0x47e8d0 (pizzafrenzy)
Font::Font(ResourceManager* theResources)
	: mResources(theResources), mHeight(0), mSpaceWidth(0), mGap(0), mCursorX(0), mCursorY(0), mLeftOverlap(0),
	  mRightOverlap(0), mTopOverlap(0), mBottomOverlap(0)
{
	for (int i = 0; i < 256; i++)
	{
		mA[i] = 0;
		mB[i] = 0;
		mC[i] = 0;
	}
}

// 0x43d680 (wordharmony), 0x47e7e0 (pizzafrenzy)
Font::~Font()
{
}

// 0x43d840 (wordharmony), 0x47eb90 (pizzafrenzy)
void Font::load(const char* theFileName)
{
	Ref<InputStream> aStream = mResources->openStream(theFileName);
	FontParser aParser(this);
	parseXml(aParser, aStream);
}

// 0x43d530 (wordharmony), 0x47e630 (pizzafrenzy)
bool Font::hasGlyph(char c) const
{
	return !mGlyphRects[index(c)].isEmpty();
}

// 0x43d550 (wordharmony), 0x47e650 (pizzafrenzy)
int Font::getStringWidth(const char* theString)
{
	int aWidth = 0;
	char c = *theString;
	while (c != '\0')
	{
		int i = index(c);
		int aCharWidth;
		if (!mGlyphRects[i].isEmpty())
			aCharWidth = mC[i] + mB[i] + mA[i] - mRightOverlap - mLeftOverlap;
		else
			aCharWidth = mSpaceWidth;
		aWidth += aCharWidth;
		c = *++theString;
	}
	return aWidth;
}

// 0x43d610 (wordharmony), 0x47e770 (pizzafrenzy)
int Font::getLeadingWidth(char c) const
{
	return mA[index(c)] - mLeftOverlap;
}

// 0x43d630 (wordharmony), 0x47e790 (pizzafrenzy)
int Font::getTrailingWidth(char c) const
{
	return mC[index(c)] + mB[index(c)] - mRightOverlap;
}

// 0x43d660 (wordharmony), 0x47e7c0 (pizzafrenzy)
const IntRect* Font::getGlyphRect(char c) const
{
	return &mGlyphRects[index(c)];
}

// 0x43daa0 (wordharmony), 0x47ead0 (pizzafrenzy): the listener build leaves half the gap on each side of a glyph, the
// signal build the whole gap
void Font::addGlyph(int theChar, int theA, int theB, int theC)
{
	int i = theChar & 0xff;
	mA[i] = theA;
	mB[i] = theB;
	mC[i] = theC;
	if (gSignalBuild)
	{
		int aMargins = mGap * 2;
		if (mImage->getWidth() < mCursorX + aMargins + theB)
		{
			mCursorX = 0;
			mCursorY = mCursorY + mHeight + aMargins;
		}
		int aLeft = mCursorX + mGap;
		mGlyphRects[i].left = aLeft;
		mGlyphRects[i].right = aLeft + theB;
		int aTop = mGap + mCursorY;
		mGlyphRects[i].top = aTop;
		mGlyphRects[i].bottom = mHeight + aTop;
		mCursorX = mCursorX + theB + mGap * 2;
		return;
	}
	int aGap = mGap;
	if (mImage->getWidth() < mCursorX + aGap + theB)
	{
		mCursorX = 0;
		mCursorY = mCursorY + mHeight + aGap;
	}
	int aLeft = aGap / 2 + mCursorX;
	mGlyphRects[i].left = aLeft;
	mGlyphRects[i].right = aLeft + theB;
	int aTop = mGap / 2 + mCursorY;
	mGlyphRects[i].top = aTop;
	mGlyphRects[i].bottom = mHeight + aTop;
	mCursorX = mCursorX + mGap + theB;
}

// 0x43db80 (wordharmony), 0x47eca0 (pizzafrenzy)
void FontParser::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("font") == 0)
	{
		std::string aFace = theAttributes.getString("face", "");
		std::string anImageName = theAttributes.getString("image", "");
		long aHeight = theAttributes.getInt("height", 0);
		long aSpaceWidth = theAttributes.getInt("spaceWidth", 0);
		long aGap = theAttributes.getInt("gap", 0);
		long aLeftOverlap = theAttributes.getInt("leftOverlap", 0);
		long aRightOverlap = theAttributes.getInt("rightOverlap", 0);
		long aTopOverlap = theAttributes.getInt("topOverlap", 0);
		long aBottomOverlap = theAttributes.getInt("bottomOverlap", 0);
		mFont->mImage = App::instance()->getImage(anImageName.c_str());
		mFont->mHeight = aHeight;
		mFont->mSpaceWidth = aSpaceWidth;
		mFont->mGap = aGap;
		mFont->mLeftOverlap = aLeftOverlap;
		mFont->mRightOverlap = aRightOverlap;
		mFont->mTopOverlap = aTopOverlap;
		mFont->mBottomOverlap = aBottomOverlap;
		mFont->mCursorX = 0;
		mFont->mCursorY = 0;
	}
	else if (theName.compare("glyph") == 0)
	{
		long aChar = theAttributes.getInt("char", 0);
		long anA = theAttributes.getInt("a", 0);
		long aB = theAttributes.getInt("b", 0);
		long aC = theAttributes.getInt("c", 0);
		mFont->addGlyph(aChar, anA, aB, aC);
	}
}

// 0x43cba0 (wordharmony): the string starts at the state's origin, aligned by the state's alignment: horizontally its
// width times 0, 0.5 or 1 to the left, vertically its height likewise up (and at the top or bottom, by the font's top
// overlap)
void Graphics::drawString(const char* theString)
{
	Font* aFont = state().mFont;
	if (aFont == nullptr)
		return;
	float aFactor = 0;
	switch (getHAlign())
	{
	case 0: aFactor = 0; break;
	case 1: aFactor = 0.5f; break;
	case 2: aFactor = 1.0f; break;
	}
	float aScaleX = state().mScale.x;
	float x = -((float)aFont->getStringWidth(theString) * aScaleX * aFactor);
	switch (getVAlign())
	{
	case 0: aFactor = 0; break;
	case 1: aFactor = 0.5f; break;
	case 2: aFactor = 1.0f; break;
	}
	float aScaleY = state().mScale.y;
	float y = -((float)aFont->getHeight() * aScaleY * aFactor);
	switch (getVAlign())
	{
	case 0: y = (float)aFont->getTopOffset() * aScaleY + y; break;
	case 2: y = y - (float)aFont->getTopOffset() * aScaleY; break;
	}
	char aChar = *theString;
	while (aChar != '\0')
	{
		float anAdvance;
		if (aFont->hasGlyph(aChar))
		{
			x = (float)aFont->getLeadingWidth(aChar) * aScaleX + x;
			drawImage(aFont->getImage(), *aFont->getGlyphRect(aChar), x, y);
			anAdvance = (float)aFont->getTrailingWidth(aChar) * aScaleX;
		}
		else
			anAdvance = (float)aFont->getSpaceWidth();
		x = anAdvance + x;
		aChar = *++theString;
	}
}
