#include "EditBox.h"
#include "App.h"
#include "Font.h"
#include "Host.h"

using namespace engine;

// 0x472ea0 (pizzafrenzy): the blink times are not set by the original (the layout reader always sets them)
EditBox::EditBox()
{
	mMaxLength = 0;
	mCursorIndex = 0;
	mBlinkTime = 0;
	mBlinkTimeLeft = 0;
	mCharFilter = nullptr;
}

// 0x4727f0 (pizzafrenzy)
void EditBox::setText(const std::string& theText)
{
	mText = theText.substr(0, mMaxLength);
	mCursorIndex = (int)mText.size();
	if (mTextItem != nullptr)
		mTextItem->setText(mText);
	updateCursor();
	if ((int)theText.size() < mCursorIndex)
		mCursorIndex = (int)theText.size();
	updateCursor();
}

// port (the code that 0x4727f0 (pizzafrenzy), 0x4723c0 (pizzafrenzy), 0x4724d0 (pizzafrenzy) and 0x472240 (pizzafrenzy)
// share): the text item shows the text, and the cursor follows the end of the text before it
void EditBox::textChanged()
{
	if (mTextItem != nullptr)
		mTextItem->setText(mText);
	updateCursor();
}

// the cursor shows and its blink starts again (the end of 0x472920 (pizzafrenzy), 0x472240 (pizzafrenzy))
void EditBox::showCursor()
{
	mCursor->setVisible(true);
	mBlinkTimeLeft = mBlinkTime;
}

// 0x473060 (pizzafrenzy)
void EditBox::connect()
{
	if (mCursor != nullptr)
	{
		mCursor->setVisible(true);
		mBlinkTimeLeft = mBlinkTime;
	}
	App* anApp = App::instance();
	anApp->mKeyDownSignal.disconnect(this);
	anApp->mKeyRepeatSignal.disconnect(this);
	anApp->mCharSignal.disconnect(this);
	anApp->mKeyDownSignal.connect(this, &EditBox::keyDown);
	anApp->mKeyRepeatSignal.connect(this, &EditBox::keyRepeat);
	anApp->mCharSignal.connect(this, &EditBox::charTyped);
	addOwnFlags(NODE_LISTENERS);
}

// 0x4728c0 (pizzafrenzy)
void EditBox::disconnect()
{
	if (mCursor != nullptr)
		mCursor->setVisible(false);
	App* anApp = App::instance();
	anApp->mKeyDownSignal.disconnect(this);
	anApp->mKeyRepeatSignal.disconnect(this);
	anApp->mCharSignal.disconnect(this);
	removeOwnFlags(NODE_LISTENERS);
}

// 0x471fd0 (pizzafrenzy): with no blink time the cursor stays shown; otherwise it turns over each time the time is up
void EditBox::update(UpdateInfo& theInfo)
{
	if (mBlinkTime == 0.0f)
	{
		mCursor->setVisible(true);
		return;
	}
	mBlinkTimeLeft = mBlinkTimeLeft - theInfo.mSeconds;
	if (mBlinkTimeLeft < 0.0f)
	{
		mCursor->setVisible(!mCursor->isVisible());
		mBlinkTimeLeft = mBlinkTime;
	}
}

// 0x4724d0 (pizzafrenzy)
void EditBox::eraseBeforeCursor()
{
	if (0 < mCursorIndex)
	{
		mText = mText.substr(0, mCursorIndex - 1) + mText.substr(mCursorIndex);
		mCursorIndex = mCursorIndex - 1;
		textChanged();
	}
}

// 0x4723c0 (pizzafrenzy)
void EditBox::eraseAfterCursor()
{
	if (mCursorIndex < (int)mText.size())
	{
		mText = mText.substr(0, mCursorIndex) + mText.substr(mCursorIndex + 1);
		textChanged();
	}
}

// 0x4725e0 (pizzafrenzy): the clipboard's text, as much of it as the box has room for, goes in at the cursor, which
// ends after it
void EditBox::paste()
{
	int aRoom = mMaxLength - (int)mText.size();
	if (0 < aRoom)
	{
		std::string aPasted = getHost()->clipboardText().substr(0, aRoom);
		std::string aAfter = mText.substr(mCursorIndex);
		mText = mText.substr(0, mCursorIndex) + aPasted;
		mCursorIndex = (int)mText.size();
		mText += aAfter;
		textChanged();
	}
}

// 0x472920 (pizzafrenzy): the keys are the virtual keys: 8 back, 0x23 end, 0x24 home, 0x25 left, 0x27 right, 0x2e
// delete
// and 0x56 (V) with the Ctrl key down
void EditBox::keyDown(int theKey)
{
	switch (theKey)
	{
	case 8:
		eraseBeforeCursor();
		break;
	default:
		return;
	case 0x23:
		mCursorIndex = (int)mText.size();
		updateCursor();
		break;
	case 0x24:
		mCursorIndex = 0;
		updateCursor();
		break;
	case 0x25:
		mCursorIndex = mCursorIndex - 1 < 1 ? 0 : mCursorIndex - 1;
		updateCursor();
		break;
	case 0x27:
		mCursorIndex = mCursorIndex + 1 < (int)mText.size() ? mCursorIndex + 1 : (int)mText.size();
		updateCursor();
		break;
	case 0x2e:
		eraseAfterCursor();
		break;
	case 0x56:
		if (getHost()->controlDown())
			paste();
	}
	showCursor();
}

// 0x472a70 (pizzafrenzy)
void EditBox::keyRepeat(int theKey, int theCount)
{
	for (int i = 0; i < theCount; i++)
		keyDown(theKey);
}

// 0x472240 (pizzafrenzy): a printable character, while the text has room and the filter (if there is one) takes it,
// goes in at the cursor. The original asks the multibyte printable test of the character as a signed number, which
// only the characters of the ASCII range pass here.
void EditBox::charTyped(int theChar)
{
	if ((int)mText.size() < mMaxLength && theChar >= 0x20 && theChar < 0x7f)
	{
		if (mCharFilter != nullptr && mCharFilter((char)theChar) == '\0')
			return;
		mText = mText.substr(0, mCursorIndex) + (char)theChar + mText.substr(mCursorIndex);
		mCursorIndex = mCursorIndex + 1;
		showCursor();
		textChanged();
	}
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
