#include "TextItem.h"
#include "App.h"
#include "Font.h"
#include "Graphics.h"
#include <cstdio>
#include <cstring>

using namespace engine;

// 0x427680
TextItem::TextItem()
{
	mHAlign = 0;
	mVAlign = 0;
	mStyle = 0;
	setName("TextItem");
	addOwnFlags(NODE_DRAWABLE);
	setFlags(NODE_LAYOUT);
}

// 0x4277d0
TextItem::~TextItem()
{
}

// 0x427260: the block of lines starts above the position by the alignment, each line at the render state's origin
// moved down by the line height; the underline and the strike-through are lines in the state's colour. The total
// height is taken as an unsigned number, as the original did.
void TextItem::draw(Graphics* g)
{
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

// 0x4270b0: the widest line by the line count's height, placed by the alignment (the scale is not applied)
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
				aWidth = (float)mFont->getStringWidth(mLines[i].c_str());
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

// 0x427070
void TextItem::setHAlign(int theAlign)
{
	mHAlign = theAlign;
	setFlags(NODE_LAYOUT);
}

// 0x427090
void TextItem::setVAlign(int theAlign)
{
	mVAlign = theAlign;
	setFlags(NODE_LAYOUT);
}

// 0x427600
void TextItem::setFont(std::string theName)
{
	setFont(App::instance()->getFont(theName.c_str()));
	setFlags(NODE_LAYOUT);
}

// 0x4278c0: the last piece is what follows the last break (empty when the text ends with one)
void TextItem::setText(std::string theText)
{
	mText = theText;
	mLines.clear();
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

// 0x4279f0: the digits are moved right in place, a comma before every third from the end (a minus sign counts as a
// digit, so -123 becomes "-,123")
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

// 0x427b60
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
