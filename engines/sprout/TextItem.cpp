#include "TextItem.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include <cstdio>
#include <cstring>

using namespace engine;

// 0x427680 (wordharmony), 0x46fc70 (pizzafrenzy): the signal build's has no name of its own
TextItem::TextItem()
{
	mHAlign = 0;
	mVAlign = 0;
	mStyle = 0;
	mWordWrap = 0;
	mShadowOpacity = 0.5f;
	mShadowOffset = Point(2.0f, 2.0f);
	mLineOffset = 0;
	mNextLineOffset = 0;
	if (!gSignalBuild)
		setName("TextItem");
	addOwnFlags(NODE_DRAWABLE);
	setFlags(NODE_LAYOUT);
}

// 0x4277d0 (wordharmony)
TextItem::~TextItem()
{
}

// 0x427260 (wordharmony): the block of lines starts above the position by the alignment, each line at the render
// state's origin moved down by the line height; the underline and the strike-through are lines in the state's colour.
// The total height is taken as an unsigned number, as the original did.
void TextItem::draw(Graphics* g)
{
	if (gSignalBuild)
	{
		// 0x46f5b0 (pizzafrenzy): the shadow is the lines again, moved by the shadow's offset, in black added to the
		// glyphs (colour mode 1, colour -1) with the opacity times the shadow's
		if (mText.compare("") == 0 || !isVisible())
			return;
		if ((mStyle & TEXT_SHADOW) != 0)
		{
			g->pushState();
			g->translate(mShadowOffset.x, mShadowOffset.y);
			g->setColor(-1.0f, -1.0f, -1.0f, 1.0f);
			g->setColorMode(1);
			g->setOpacity(g->getOpacity() * mShadowOpacity);
			drawLines(g);
			g->popState();
		}
		drawLines(g);
		return;
	}
	if (mText.compare("") == 0)
		return;
	if (!mVisible)
		return;
	g->setHAlign(mHAlign);
	g->setVAlign(mVAlign);
	int aCount = (int)mLines.size();
	float aHeight = (float)(unsigned int)(mFont->getLineHeight() * aCount);
	float y;
	if (mVAlign == 1)
		y = g->state().mTranslation.y - (aHeight - (float)mFont->getLineHeight()) * 0.5f;
	else if (mVAlign == 2)
		y = g->state().mTranslation.y - (aHeight - (float)mFont->getLineHeight());
	else
		y = g->state().mTranslation.y;
	g->setTranslation(g->state().mTranslation.x, y);
	unsigned int i = 0;
	while (i < mLines.size())
	{
		const char* aLine = mLines[i].c_str();
		g->drawString(aLine);
		if ((mStyle & TEXT_STRIKETHROUGH) != 0)
		{
			float aWidth = (float)mFont->getStringWidth(aLine);
			float x = 0;
			if (mHAlign == 1)
				x = aWidth * -0.5f;
			else if (mHAlign == 2)
				x = -aWidth;
			float aLineY = 0;
			if (mVAlign == 2)
				aLineY = (float)-(mFont->getLineHeight() / 2);
			else if (mVAlign == 0)
				aLineY = (float)(mFont->getLineHeight() / 2);
			g->drawLine(Point(x, aLineY), Point(x + aWidth, aLineY));
		}
		else if ((mStyle & TEXT_UNDERLINE) != 0)
		{
			int aWidth = mFont->getStringWidth(aLine);
			float x = 0;
			if (mHAlign == 1)
				x = (float)-(aWidth / 2);
			else if (mHAlign == 2)
				x = -(float)aWidth;
			float aLineY = 0;
			if (mVAlign == 0)
				aLineY = (float)mFont->getLineHeight();
			else if (mVAlign == 1)
				aLineY = (float)(mFont->getLineHeight() / 2);
			g->drawLine(Point(x, aLineY), Point((float)aWidth + x, aLineY));
		}
		g->setTranslation(g->state().mTranslation.x, (float)mFont->getLineHeight() + g->state().mTranslation.y);
		i++;
	}
}

// 0x46f830 (pizzafrenzy): the lines as the block of lines of the original's draw above, the first line's x moved by
// mLineOffset and the next ones' by mNextLineOffset; a strike-through and an underline can both be drawn
void TextItem::drawLines(Graphics* g)
{
	g->setHAlign(mHAlign);
	g->setVAlign(mVAlign);
	int aCount = (int)mLines.size();
	float aHeight = (float)(unsigned int)(mFont->getLineHeight() * aCount);
	float y;
	if (mVAlign == 1)
		y = g->state().mTranslation.y - (aHeight - (float)mFont->getLineHeight()) * 0.5f;
	else if (mVAlign == 2)
		y = g->state().mTranslation.y - (aHeight - (float)mFont->getLineHeight());
	else
		y = g->state().mTranslation.y;
	g->setTranslation(mLineOffset + g->state().mTranslation.x, y);
	unsigned int i = 0;
	while (i < mLines.size())
	{
		const char* aLine = mLines[i].c_str();
		g->drawString(aLine);
		if ((mStyle & TEXT_STRIKETHROUGH) != 0)
		{
			float aWidth = (float)mFont->getStringWidth(aLine);
			float x = 0;
			if (mHAlign == 1)
				x = aWidth * -0.5f;
			else if (mHAlign == 2)
				x = -aWidth;
			float aLineY = 0;
			if (mVAlign == 2)
				aLineY = (float)-(mFont->getLineHeight() / 2);
			else if (mVAlign == 0)
				aLineY = (float)(mFont->getLineHeight() / 2);
			g->drawLine(Point(x, aLineY), Point(x + aWidth, aLineY));
		}
		if ((mStyle & TEXT_UNDERLINE) != 0)
		{
			int aWidth = mFont->getStringWidth(aLine);
			float x = 0;
			if (mHAlign == 1)
				x = (float)-(aWidth / 2);
			else if (mHAlign == 2)
				x = -(float)aWidth;
			float aLineY = 0;
			if (mVAlign == 0)
				aLineY = (float)mFont->getLineHeight();
			else if (mVAlign == 1)
				aLineY = (float)(mFont->getLineHeight() / 2);
			g->drawLine(Point(x, aLineY), Point((float)aWidth + x, aLineY));
		}
		g->setTranslation(mNextLineOffset + g->state().mTranslation.x,
			(float)mFont->getLineHeight() + g->state().mTranslation.y);
		i++;
	}
}

// 0x4270b0 (wordharmony): the widest line by the line count's height, placed by the alignment (the scale is not
// applied). 0x46f670 (pizzafrenzy): each line's width has the line offset added (the first line's, or the next
// lines'; a line with no text stays 0).
void TextItem::layout()
{
	if (mFont != nullptr)
	{
		float aMaxWidth = 0;
		unsigned int i = 0;
		while (i < mLines.size())
		{
			float aWidth;
			if (mLines[i].size() == 0)
				aWidth = 0;
			else
			{
				aWidth = (float)mFont->getStringWidth(mLines[i].c_str());
				if (gSignalBuild)
					aWidth = aWidth + (i == 0 ? mLineOffset : mNextLineOffset);
			}
			if (aMaxWidth < aWidth)
				aMaxWidth = aWidth;
			i++;
		}
		float aHeight = (float)(unsigned int)(mFont->getLineHeight() * (int)mLines.size());
		float aLeft;
		if (mHAlign == 1)
			aLeft = mPosition.x - aMaxWidth * 0.5f;
		else if (mHAlign == 2)
			aLeft = mPosition.x - aMaxWidth;
		else
			aLeft = mPosition.x;
		float aTop;
		if (mVAlign == 1)
			aTop = mPosition.y - aHeight * 0.5f;
		else if (mVAlign == 2)
			aTop = mPosition.y - aHeight;
		else
			aTop = mPosition.y;
		mBounds.set(aLeft, aTop, aLeft + aMaxWidth, aTop + aHeight);
		clearFlags(NODE_LAYOUT);
		return;
	}
	mBounds = Rect();
	clearFlags(NODE_LAYOUT);
}

// 0x427070 (wordharmony), 0x46f520 (pizzafrenzy)
void TextItem::setHAlign(int theAlign)
{
	mHAlign = theAlign;
	setFlags(NODE_LAYOUT);
}

// 0x427090 (wordharmony), 0x46f540 (pizzafrenzy)
void TextItem::setVAlign(int theAlign)
{
	mVAlign = theAlign;
	setFlags(NODE_LAYOUT);
}

// 0x427600 (wordharmony), 0x46f560 (pizzafrenzy)
void TextItem::setFont(std::string theName)
{
	setFont(App::instance()->getFont(theName.c_str()));
	setFlags(NODE_LAYOUT);
}

// The width of one character in the font (0x47e6c0 (pizzafrenzy)): the glyph's three widths less the overlaps, a
// space's width for a character with no glyph
static int getCharWidth(Font* theFont, char theChar)
{
	char aString[2] = {theChar, '\0'};
	return theFont->getStringWidth(aString);
}

// 0x4278c0 (wordharmony), 0x46fee0 (pizzafrenzy): the last piece is what follows the last break (empty when the text
// ends with one). The signal build's, with a wrap width, adds up the characters' widths and, when the line reaches the
// width, breaks it after the last space (or before the character that reached it when the line has none); the
// character after a backslash is measured without the backslash, and the character that broke a line with no space is
// dropped.
void TextItem::setText(std::string theText)
{
	mText = theText;
	mLines.clear();
	if (gSignalBuild && 0 < mWordWrap)
	{
		int aStart = 0;
		int aLastSpace = 0;
		int aWidth = 0;
		unsigned int i = 0;
		if (theText.size() != 0)
		{
			do
			{
				int aCharWidth;
				if (theText[i] == ' ')
				{
					aCharWidth = mFont->getSpaceWidth();
					aLastSpace = (int)i;
				}
				else
				{
					if (theText[i] == '\\')
					{
						i++;
						if (theText[i] == 'n')
						{
							mLines.push_back(theText.substr(aStart, (i - aStart) - 1));
							aWidth = 0;
							aStart = (int)i + 1;
							i++;
							continue;
						}
					}
					aCharWidth = getCharWidth(mFont, theText[i]);
				}
				aWidth += aCharWidth;
				if (mWordWrap <= aWidth)
				{
					if (aLastSpace <= aStart)
					{
						mLines.push_back(theText.substr(aStart, i - aStart));
						aLastSpace = (int)i + 1;
						aWidth = 0;
						aStart = (int)i + 1;
					}
					else
					{
						mLines.push_back(theText.substr(aStart, aLastSpace - aStart));
						aLastSpace++;
						std::string aRest = theText.substr(aLastSpace, (i - aLastSpace) + 1);
						aWidth = mFont->getStringWidth(aRest.c_str());
						aStart = aLastSpace;
					}
				}
				i++;
			} while (i < theText.size());
		}
		mLines.push_back(theText.substr(aStart, i - aStart));
		layout();
		setFlags(NODE_LAYOUT);
		return;
	}
	size_t aStart = 0;
	size_t anEnd;
	do
	{
		anEnd = theText.find("\\n", aStart, 2);
		if (anEnd == std::string::npos)
			anEnd = theText.size();
		mLines.push_back(theText.substr(aStart, anEnd - aStart));
		aStart = anEnd + 2;
	} while (anEnd != theText.size());
	layout();
	setFlags(NODE_LAYOUT);
}

// 0x4279f0 (wordharmony): the digits are moved right in place, a comma before every third from the end (a minus sign
// counts as a digit, so -123 becomes "-,123")
void TextItem::setNumber(int theNumber)
{
	char aBuffer[32];
	sprintf(aBuffer, "%d", theNumber);
	mLines.clear();
	int aLast = (int)strlen(aBuffer) - 1;
	int aNewLast = aLast + aLast / 3;
	int aDigits = 0;
	if (-1 < aNewLast)
	{
		const char* aSource = aBuffer + aLast;
		int aDest = aNewLast;
		do
		{
			int aPos = aDest;
			if (aDigits == 3)
			{
				aBuffer[aDest] = ',';
				aPos = aDest - 1;
				aDigits = 0;
			}
			aDest = aPos - 1;
			aBuffer[aPos] = *aSource;
			aSource--;
			aDigits++;
		} while (-1 < aDest);
	}
	aBuffer[aNewLast + 1] = '\0';
	mText = std::string(aBuffer, strlen(aBuffer));
	mLines.push_back(mText);
	layout();
}

// 0x470340 (pizzafrenzy): the digits grouped as setNumber does, with the prefix before them
void TextItem::setNumber(int theNumber, const std::string& thePrefix)
{
	char aBuffer[32];
	sprintf(aBuffer, "%d", theNumber);
	mLines.clear();
	int aLast = (int)strlen(aBuffer) - 1;
	int aNewLast = aLast + aLast / 3;
	int aDigits = 0;
	if (-1 < aNewLast)
	{
		const char* aSource = aBuffer + aLast;
		int aDest = aNewLast;
		do
		{
			int aPos = aDest;
			if (aDigits == 3)
			{
				aBuffer[aDest] = ',';
				aPos = aDest - 1;
				aDigits = 0;
			}
			aDest = aPos - 1;
			aBuffer[aPos] = *aSource;
			aSource--;
			aDigits++;
		} while (-1 < aDest);
	}
	aBuffer[aNewLast + 1] = '\0';
	mText = thePrefix + aBuffer;
	mLines.push_back(mText);
	layout();
}

// 0x427b60 (wordharmony)
void TextItem::setTime(int theSeconds)
{
	char aBuffer[32];
	const char* aSecondsPad = "0";
	int aMinutes = (theSeconds / 60) % 60;
	if (9 < theSeconds % 60)
		aSecondsPad = "";
	const char* aMinutesPad = "0";
	if (9 < aMinutes)
		aMinutesPad = "";
	sprintf(aBuffer, "%d:%s%d:%s%d", (theSeconds / 60) / 60, aMinutesPad, aMinutes, aSecondsPad, theSeconds % 60);
	mLines.clear();
	mText = std::string(aBuffer, strlen(aBuffer));
	mLines.push_back(mText);
	layout();
}
