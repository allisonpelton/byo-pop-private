#include "TextItem.h"

#include "DrawContext.h"
#include "Font.h"

namespace engine
{

TextItem::TextItem()
	: mAlignH(TEXT_ALIGN_LEFT), mAlignV(TEXT_ALIGN_TOP), mFlags(0), mFirstIndent(0), mIndent(0), mWrapWidth(0),
	  mShadowOpacity(0.5f), mShadowX(2.0f), mShadowY(2.0f)
{
	addOwnState(STATE_VISIBLE);
	addState(STATE_BOUNDS_DIRTY);
}

void TextItem::setText(const std::string& theText)
{
	mText = theText;
	mLines.clear();
	Font* aFont = mFont;
	// (wrapping without a font crashes the original; the port breaks only at "\n" then)
	if (mWrapWidth < 1 || aFont == nullptr)
	{
		size_t aStart = 0;
		size_t anEnd;
		do
		{
			anEnd = mText.find("\\n", aStart);
			if (anEnd == std::string::npos)
				anEnd = mText.size();
			mLines.push_back(mText.substr(aStart, anEnd - aStart));
			aStart = anEnd + 2;
		} while (anEnd != mText.size());
	}
	else
	{
		size_t aLineStart = 0;
		size_t aLastSpace = 0;
		// _ftol2 truncates
		int aWidth = (int)mFirstIndent;
		size_t aNext = 1;
		size_t i;
		for (i = 0; i < mText.size(); i++, aNext++)
		{
			char aChar = mText[i];
			int anAdvance;
			if (aChar == ' ')
			{
				anAdvance = aFont->getSpaceWidth();
				aLastSpace = i;
			}
			else if (aChar != '\\')
				anAdvance = aFont->getCharWidth(aChar);
			else
			{
				i++;
				aNext++;
				if (mText[i] == 'n')
				{
					mLines.push_back(mText.substr(aLineStart, i - 1 - aLineStart));
					aLineStart = aNext;
					aWidth = (int)mIndent;
					continue;
				}
				anAdvance = aFont->getCharWidth(mText[i]);
			}
			aWidth += anAdvance;
			if (aWidth >= mWrapWidth)
			{
				if (aLineStart < aLastSpace)
				{
					mLines.push_back(mText.substr(aLineStart, aLastSpace - aLineStart));
					aLastSpace++;
					aLineStart = aLastSpace;
					aWidth = aFont->getStringWidth(mText.substr(aLastSpace, i + 1 - aLastSpace).c_str());
				}
				else
				{
					// A word longer than the width: the line ends before the character, and the character is lost
					mLines.push_back(mText.substr(aLineStart, i - aLineStart));
					aLastSpace = aNext;
					aLineStart = aNext;
					aWidth = (int)mIndent;
				}
			}
		}
		mLines.push_back(mText.substr(aLineStart, i - aLineStart));
	}
	computeBounds();
	addState(STATE_BOUNDS_DIRTY);
}

void TextItem::paint(DrawContext* theContext)
{
	if (mText.empty() || !isVisible())
		return;
	if ((mFlags & TEXT_ITEM_SHADOW) != 0)
	{
		theContext->pushState();
		// 0x4523a0 (feedingfrenzy2)
		DrawState& aState = theContext->back();
		aState.mPosition.x = aState.mPosition.x + mShadowX;
		aState.mPosition.y = aState.mPosition.y + mShadowY;
		// 0x41a660 (feedingfrenzy2): black, as a colour added
		aState.mColor = Color(-1.0f, -1.0f, -1.0f, 1.0f);
		aState.mColorMode = COLOR_MODE_ADD;
		aState.mOpacity = aState.mOpacity * mShadowOpacity;
		paintLines(theContext);
		theContext->popState();
	}
	paintLines(theContext);
}

void TextItem::paintLines(DrawContext* theContext)
{
	theContext->back().mTextAlignH = (TextAlignH)mAlignH;
	theContext->back().mTextAlignV = (TextAlignV)mAlignV;
	Font* aFont = theContext->back().mFont;
	// (no font crashes the original)
	if (aFont == nullptr)
		return;
	int aLineHeight = aFont->getHeight() - aFont->getTopOverlap() - aFont->getBottomOverlap();
	float aHeight = (float)(unsigned int)(aLineHeight * (int)mLines.size());
	// The x87 keeps the line's top in a register between the lines
	double y;
	if (mAlignV == TEXT_ALIGN_MIDDLE)
		y = (double)theContext->back().mPosition.y - ((double)aHeight - (float)aLineHeight) * 0.5;
	else if (mAlignV == TEXT_ALIGN_BOTTOM)
		y = (double)theContext->back().mPosition.y - ((double)aHeight - (float)aLineHeight);
	else
		y = theContext->back().mPosition.y;
	theContext->back().mPosition.x = theContext->back().mPosition.x + mFirstIndent;
	for (size_t i = 0;; i++)
	{
		theContext->back().mPosition.y = (float)y;
		if (i >= mLines.size())
			break;
		const char* aLine = mLines[i].c_str();
		theContext->drawText(aLine);
		if ((mFlags & TEXT_ITEM_STRIKE) != 0)
		{
			float aWidth = (float)aFont->getStringWidth(aLine);
			float aLeft = 0.0f;
			if (mAlignH == TEXT_ALIGN_CENTER)
				aLeft = aWidth * -0.5f;
			else if (mAlignH == TEXT_ALIGN_RIGHT)
				aLeft = -aWidth;
			float aLineY = 0.0f;
			if (mAlignV == TEXT_ALIGN_BOTTOM)
				aLineY = (float)-(aLineHeight / 2);
			else if (mAlignV == TEXT_ALIGN_TOP)
				aLineY = (float)(aLineHeight / 2);
			theContext->drawLine(Vector2f(aLeft, aLineY), Vector2f(aLeft + aWidth, aLineY));
		}
		if ((mFlags & TEXT_ITEM_UNDERLINE) != 0)
		{
			int aWidth = aFont->getStringWidth(aLine);
			float aLeft = 0.0f;
			if (mAlignH == TEXT_ALIGN_CENTER)
				aLeft = (float)-(aWidth / 2);
			else if (mAlignH == TEXT_ALIGN_RIGHT)
				aLeft = -(float)aWidth;
			float aLineY = 0.0f;
			if (mAlignV == TEXT_ALIGN_TOP)
				aLineY = (float)aLineHeight;
			else if (mAlignV == TEXT_ALIGN_MIDDLE)
				aLineY = (float)(aLineHeight / 2);
			theContext->drawLine(Vector2f(aLeft, aLineY), Vector2f((float)aWidth + aLeft, aLineY));
		}
		y = (double)(float)aLineHeight + theContext->back().mPosition.y;
		theContext->back().mPosition.x = theContext->back().mPosition.x + mIndent;
	}
}

void TextItem::computeBounds()
{
	Font* aFont = mFont;
	float aHeight = 0.0f;
	if (aFont == nullptr)
	{
		mBounds.left = 0.0f;
		mBounds.top = 0.0f;
		mBounds.right = 0.0f;
	}
	else
	{
		float aWidth = 0.0f;
		for (size_t i = 0; i < mLines.size(); i++)
		{
			float anIndent = i == 0 ? mFirstIndent : mIndent;
			float aLineWidth = 0.0f;
			if (!mLines[i].empty())
				aLineWidth = anIndent + (float)aFont->getStringWidth(mLines[i].c_str());
			if (!(aLineWidth < aWidth))
				aWidth = aLineWidth;
		}
		int aLineHeight = aFont->getHeight() - aFont->getBottomOverlap() - aFont->getTopOverlap();
		aHeight = (float)(unsigned int)(aLineHeight * (int)mLines.size());
		float aLeft = mPosition.x;
		if (mAlignH == TEXT_ALIGN_CENTER)
			aLeft = mPosition.x - aWidth * 0.5f;
		else if (mAlignH == TEXT_ALIGN_RIGHT)
			aLeft = mPosition.x - aWidth;
		float aTop = mPosition.y;
		if (mAlignV == TEXT_ALIGN_MIDDLE)
			aTop = mPosition.y - aHeight * 0.5f;
		else if (mAlignV == TEXT_ALIGN_BOTTOM)
			aTop = mPosition.y - aHeight;
		mBounds.left = aLeft;
		mBounds.top = aTop;
		mBounds.right = aLeft + aWidth;
		aHeight = aHeight + aTop;
	}
	mBounds.bottom = aHeight;
	removeState(STATE_BOUNDS_DIRTY);
}

} // namespace engine
