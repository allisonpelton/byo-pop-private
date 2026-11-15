#include "EditBox.h"
#include "App.h"
#include "Font.h"

using namespace engine;

// 0x472ea0 (pizzafrenzy): the blink times are not set by the original (the layout reader always sets them)
EditBox::EditBox()
{
	mMaxLength = 0;
	mCursorIndex = 0;
	mBlinkTime = 0;
	mBlinkTimeLeft = 0;
	mCursorShown = 0;
}

// 0x471db0 (pizzafrenzy)
void EditBox::setImages(const std::string& theBackgroundImage, const std::string& theCursorImage,
	const std::string& theFont)
{
	Image* aBackground = App::instance()->getImage(theBackgroundImage.c_str());
	addChild(new ImageNode(aBackground));
	mTextItem = new TextItem();
	mTextItem->setFont(theFont);
	mTextItem->setHAlign(0);
	addChild(mTextItem);
	Image* aCursor = App::instance()->getImage(theCursorImage.c_str());
	mCursor = new ImageNode(aCursor);
	addChild(mCursor);
	mFont = App::instance()->getFont(theFont.c_str());
}

// 0x472220 (pizzafrenzy)
void EditBox::setCursorOffset(const Point& theOffset)
{
	mCursorOffset = theOffset;
	updateCursor();
}

// 0x471fc0 (pizzafrenzy)
void EditBox::setTextOffset(const Point& theOffset)
{
	mTextItem->setPosition(theOffset);
}

// 0x472130 (pizzafrenzy)
void EditBox::updateCursor()
{
	if (mCursor != nullptr && mFont != nullptr)
	{
		std::string aBeforeCursor = mText.substr(0, mCursorIndex);
		int aWidth = mFont->getStringWidth(aBeforeCursor.c_str());
		Point aPosition = mCursorOffset;
		aPosition.x = (float)aWidth + aPosition.x;
		mCursor->setPosition(aPosition);
	}
}
