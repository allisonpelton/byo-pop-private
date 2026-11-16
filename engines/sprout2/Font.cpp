#include "Font.h"

#include "Engine.h"
#include "Surface.h"

namespace engine
{

// The font file's handler: <font> sets the image and the metrics, each <glyph> adds a glyph
class Font::FontDocHandler : public AbstractDocHandler
{
public:
	// 0x4077b0 (feedingfrenzy2)
	explicit FontDocHandler(Font* theFont) : mFont(theFont) {}

	// 0x453180 (feedingfrenzy2)
	void startElement(const std::string& theName, Params& theAttributes) override
	{
		if (theName == "font")
		{
			std::string aFace = theAttributes.getString("face", "");
			std::string anImagePath = theAttributes.getString("image", "");
			int aHeight = theAttributes.getInt("height", 0);
			int aSpaceWidth = theAttributes.getInt("spaceWidth", 0);
			int aGap = theAttributes.getInt("gap", 0);
			int aLeftOverlap = theAttributes.getInt("leftOverlap", 0);
			int aRightOverlap = theAttributes.getInt("rightOverlap", 0);
			int aTopOverlap = theAttributes.getInt("topOverlap", 0);
			int aBottomOverlap = theAttributes.getInt("bottomOverlap", 0);
			mFont->mImage = getEngine()->getImage(anImagePath);
			mFont->mHeight = aHeight;
			mFont->mSpaceWidth = aSpaceWidth;
			mFont->mGap = aGap;
			mFont->mLeftOverlap = aLeftOverlap;
			mFont->mRightOverlap = aRightOverlap;
			mFont->mTopOverlap = aTopOverlap;
			mFont->mBottomOverlap = aBottomOverlap;
			mFont->mLayoutX = 0;
			mFont->mLayoutY = 0;
		}
		else if (theName == "glyph")
		{
			int aChar = theAttributes.getInt("char", 0);
			int aLeadingSpace = theAttributes.getInt("a", 0);
			int aGlyphWidth = theAttributes.getInt("b", 0);
			int aTrailingSpace = theAttributes.getInt("c", 0);
			mFont->addGlyph(aChar, aLeadingSpace, aGlyphWidth, aTrailingSpace);
		}
	}

private:
	Font* mFont;  // +0x30
};

Font::Font(Engine* theEngine)
	: mEngine(theEngine), mLeadingSpaces(), mGlyphWidths(), mTrailingSpaces(), mHeight(0), mSpaceWidth(0), mGap(0),
	  mLayoutX(0), mLayoutY(0), mLeftOverlap(0), mRightOverlap(0), mTopOverlap(0), mBottomOverlap(0)
{
	// port: the original leaves the width tables and the metrics as the allocation found them; only the glyph
	// rectangles start empty, and a glyph's widths are read only where its rectangle is set
}

Font::~Font()
{
}

void Font::load(const std::string& thePath)
{
	FontDocHandler aHandler(this);
	aHandler.addRef();  // port: a stack object, so a reference taken during the parse never deletes it
	mEngine->parseXml(thePath, &aHandler);
}

int Font::getCharWidth(char theChar) const
{
	int anIndex = glyphIndex(theChar);
	const RectI& aRect = mGlyphRects[anIndex];
	if (aRect.left < aRect.right && aRect.top < aRect.bottom)
		return mTrailingSpaces[anIndex] + mGlyphWidths[anIndex] + mLeadingSpaces[anIndex] - mRightOverlap - mLeftOverlap;
	return mSpaceWidth;
}

int Font::getStringWidth(const char* theText) const
{
	int aWidth = 0;
	for (; *theText != '\0'; theText++)
		aWidth += getCharWidth(*theText);
	return aWidth;
}

void Font::addGlyph(int theChar, int theLeadingSpace, int theGlyphWidth, int theTrailingSpace)
{
	int anIndex = glyphIndex((char)theChar);
	mLeadingSpaces[anIndex] = theLeadingSpace;
	mGlyphWidths[anIndex] = theGlyphWidth;
	mTrailingSpaces[anIndex] = theTrailingSpace;
	int aGap = mGap;
	if (mImage->getWidth() < theGlyphWidth + aGap * 2 + mLayoutX)
	{
		mLayoutX = 0;
		mLayoutY = mLayoutY + mHeight + aGap * 2;
	}
	RectI& aRect = mGlyphRects[anIndex];
	aRect.left = mLayoutX + aGap;
	aRect.right = mLayoutX + aGap + theGlyphWidth;
	aRect.top = mLayoutY + mGap;
	aRect.bottom = mHeight + mLayoutY + mGap;
	mLayoutX = mLayoutX + theGlyphWidth + mGap * 2;
}

} // namespace engine
