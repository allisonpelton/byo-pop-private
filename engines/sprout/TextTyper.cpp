#include "TextTyper.h"
#include "Font.h"
#include "Graphics.h"

using namespace engine;

// 0x482330 (pizzafrenzy)
TextTyper::TextTyper()
{
	addOwnFlags(NODE_LISTENERS);
	mLine = 0;
	mChar = 0;
	mRate = 0;
	mSentencePause = 0;
	mTimer = 0;
	mVisibleLines = 0;
	mLinePause = 0;
}

// 0x4822b0 (pizzafrenzy)
void TextTyper::setWrapAndTiming(float theWrap, float theRate, float theSentencePause)
{
	mWordWrap = (int)theWrap;
	mRate = theRate;
	mSentencePause = theSentencePause;
}

// 0x4822e0 (pizzafrenzy)
void TextTyper::setLines(int theVisibleLines, float theLinePause)
{
	mVisibleLines = theVisibleLines;
	mLinePause = theLinePause;
}

// 0x482290 (pizzafrenzy)
void TextTyper::setTypedText(const std::string& theText)
{
	setText(theText);
	reset();
}

// 0x482300 (pizzafrenzy): no updates until it starts
void TextTyper::reset()
{
	mChar = 0;
	mLine = 0;
	removeOwnFlags(NODE_LISTENERS);
}

// 0x482480 (pizzafrenzy)
void TextTyper::start()
{
	if (mLine < (int)mLines.size() && mChar < (int)mLines[mLine].size())
	{
		addOwnFlags(NODE_LISTENERS);
		mTimer = 0;
	}
}

// 0x482320 (pizzafrenzy)
void TextTyper::stop()
{
	removeOwnFlags(NODE_LISTENERS);
}

// 0x4824f0 (pizzafrenzy)
void TextTyper::skipToEnd()
{
	int aCount = (int)mLines.size();
	mLine = aCount - 1;
	mChar = (int)mLines[aCount - 1].size() - 1;
}

// 0x482430 (pizzafrenzy)
bool TextTyper::isFinished()
{
	if (mLines.empty())
		return -1 < mLine;
	return (int)mLines.size() <= mLine;
}

// 0x482730 (pizzafrenzy): each line from the first to the last is looked in for a full stop, from the first letter
// given on the first line, up to the last letter (the one before the line's end, except on the last line, where it is
// the one given)
bool TextTyper::findPause(int theFromLine, int theFromChar, int theToLine, int theToChar)
{
	size_t aFrom = theFromChar;
	for (int aLine = theFromLine; aLine <= theToLine; aLine++)
	{
		int aEnd = (int)mLines[aLine].size() - 1;
		if (aLine == theToLine)
			aEnd = theToChar;
		size_t aFound = mLines[aLine].find('.', aFrom);
		if (aFound != std::string::npos && (int)aFound < aEnd)
		{
			mLine = aLine;
			mChar = (int)aFound;
			return true;
		}
		aFrom = 0;
	}
	return false;
}

// 0x482540 (pizzafrenzy): when the time is up, as many letters are typed as the time since the last ones holds (one at
// least), a line that a pause ends being shown whole when the lines show a few at a time
void TextTyper::update(UpdateInfo& theInfo)
{
	int anOldChar = mChar;
	int anOldLine = mLine;
	mTimer = mTimer - theInfo.mSeconds;
	if (mTimer < 0.0f && !mLines.empty())
	{
		float aStep = 1.0f / mRate;
		int aCount = (int)(theInfo.mSeconds / aStep);
		if (aCount < 1)
			aCount = 1;
		int aNewChar = aCount + mChar;
		int aLineLength = (int)mLines[mLine].size();
		if (aLineLength <= aNewChar && 0 < mVisibleLines && mChar != aLineLength)
		{
			if (findPause(anOldLine, anOldChar + 1, anOldLine, aNewChar))
			{
				mTimer = mSentencePause;
				return;
			}
			mChar = (int)mLines[mLine].size();
			mTimer = mLinePause;
			return;
		}
		mChar = aNewChar;
		if (aLineLength <= aNewChar)
		{
			while (true)
			{
				int aNextLine = mLine + 1;
				if ((int)mLines.size() <= aNextLine)
					break;
				mChar = mChar - (int)mLines[mLine].size();
				mLine = aNextLine;
				if (0 < mVisibleLines || mChar < (int)mLines[aNextLine].size())
					break;
			}
		}
		if (!findPause(anOldLine, anOldChar + 1, mLine, mChar))
			mTimer = aStep;
		else
			mTimer = mSentencePause;
		if (isFinished())
			removeOwnFlags(NODE_LISTENERS);
	}
}

// 0x46f5b0 (pizzafrenzy): the item's drawing, whose lines are the ones typed
void TextTyper::draw(Graphics* g)
{
	if (mText.compare("") == 0 || !isVisible())
		return;
	if ((mStyle & TEXT_SHADOW) != 0)
	{
		g->pushState();
		g->translate(mShadowOffset.x, mShadowOffset.y);
		g->setColor(-1.0f, -1.0f, -1.0f, 1.0f);
		g->setColorMode(1);
		g->setOpacity(g->getOpacity() * mShadowOpacity);
		drawTypedLines(g);
		g->popState();
	}
	drawTypedLines(g);
}

// 0x4827f0 (pizzafrenzy): the lines as the item draws them, up to the line being typed, of which the letters typed
// show; when the lines show a few at a time, those are the last ones
void TextTyper::drawTypedLines(Graphics* g)
{
	g->setHAlign(mHAlign);
	g->setVAlign(mVAlign);
	int aVisible = (int)mLines.size();
	if (0 < mVisibleLines)
		aVisible = mVisibleLines;
	int aLineHeight = mFont->getLineHeight();
	float aY;
	if (mVAlign == 1)
		aY = g->state().mTranslation.y - ((float)(aLineHeight * aVisible) - (float)aLineHeight) * 0.5f;
	else if (mVAlign == 2)
		aY = g->state().mTranslation.y - ((float)(aLineHeight * aVisible) - (float)aLineHeight);
	else
		aY = g->state().mTranslation.y;
	g->setTranslation(mLineOffset + g->state().mTranslation.x, aY);
	int aFirst = mLine - aVisible + 1;
	if (aFirst < 1)
		aFirst = 0;
	for (int i = aFirst; i <= mLine; i++)
	{
		std::string aLine;
		if (i == mLine)
			aLine = mLines[i].substr(0, mChar + 1);
		else
			aLine = mLines[i];
		g->drawString(aLine.c_str());
		if ((mStyle & TEXT_STRIKETHROUGH) != 0)
		{
			float aWidth = (float)mFont->getStringWidth(aLine.c_str());
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
			int aWidth = mFont->getStringWidth(aLine.c_str());
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
	}
}
