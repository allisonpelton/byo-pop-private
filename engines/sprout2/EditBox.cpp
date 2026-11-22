#include "EditBox.h"

#include "DrawContext.h"
#include "Engine.h"

namespace engine
{

namespace
{

// The keys the box takes (Windows' virtual keys)
const int KEY_BACKSPACE = 0x08;
const int KEY_CONTROL = 0x11;
const int KEY_END = 0x23;
const int KEY_HOME = 0x24;
const int KEY_LEFT = 0x25;
const int KEY_RIGHT = 0x27;
const int KEY_DELETE = 0x2e;
const int KEY_V = 0x56;

// 0x4ab0f1 (feedingfrenzy2): the C library's _ismbcprint over the character as a signed number. In the "C" locale a
// character from 0 to 0x7f is printable from the space to the tilde; one from 0x80 is negative, so it is taken as a
// double-byte character, which a single-byte code page has none of
bool isPrintable(char theChar)
{
	return theChar >= 0x20 && theChar <= 0x7e;
}

} // namespace

EditBox::EditBox()
	: mMaxLength(0), mCursorIndex(0), mBlinkTime(0.0f), mBlinkLeft(0.0f), mFilter(nullptr)
{
}

void EditBox::enable()
{
	if (mCursor != nullptr)
	{
		mCursor->setVisible(true);
		mBlinkLeft = mBlinkTime;
	}
	Engine* anEngine = getEngine();
	anEngine->mKeyDownSignal.disconnect(this);
	anEngine->mKeyRepeatSignal.disconnect(this);
	anEngine->mCharSignal.disconnect(this);
	anEngine->mKeyDownSignal.connect(this, &EditBox::keyDown);
	anEngine->mKeyRepeatSignal.connect(this, &EditBox::keyRepeat);
	anEngine->mCharSignal.connect(this, &EditBox::charTyped);
	addOwnState(STATE_ANIMATED);
}

void EditBox::disable()
{
	if (mCursor != nullptr)
		mCursor->setVisible(false);
	Engine* anEngine = getEngine();
	anEngine->mKeyDownSignal.disconnect(this);
	anEngine->mKeyRepeatSignal.disconnect(this);
	anEngine->mCharSignal.disconnect(this);
	removeOwnState(STATE_ANIMATED);
}

void EditBox::update(UpdateContext& theContext)
{
	if (mBlinkTime == 0.0f)
	{
		mCursor->setVisible(true);
		return;
	}
	double aLeft = (double)mBlinkLeft - theContext.mElapsed;
	mBlinkLeft = (float)aLeft;
	if (aLeft < 0.0)
	{
		mCursor->setVisible(!mCursor->isVisible());
		mBlinkLeft = mBlinkTime;
	}
}

void EditBox::setLook(Surface* theBackground, Surface* theCursor, Font* theFont)
{
	addChild(new Image(theBackground));
	mTextItem = new TextItem();
	mTextItem->setFont(theFont);
	mTextItem->setAlignH(TEXT_ALIGN_LEFT);
	addChild(mTextItem.get());
	mCursor = new Image(theCursor);
	addChild(mCursor.get());
	mFont = theFont;
}

void EditBox::setTextColor(const Color& theColor)
{
	mTextItem->setColor(theColor);
	mTextItem->setColorMode(COLOR_MODE_MULTIPLY);
}

void EditBox::setTextOffset(const Vector2f& theOffset)
{
	mTextItem->setPosition(theOffset);
}

void EditBox::restartBlink()
{
	mCursor->setVisible(true);
	mBlinkLeft = mBlinkTime;
}

void EditBox::keyDown(int theKey)
{
	switch (theKey)
	{
	case KEY_BACKSPACE:
		deleteBack();
		break;
	case KEY_END:
		mCursorIndex = (int)mText.size();
		updateCursor();
		break;
	case KEY_HOME:
		mCursorIndex = 0;
		updateCursor();
		break;
	case KEY_LEFT:
		mCursorIndex = mCursorIndex - 1 < 0 ? 0 : mCursorIndex - 1;
		updateCursor();
		break;
	case KEY_RIGHT:
		mCursorIndex = mCursorIndex + 1 >= (int)mText.size() ? (int)mText.size() : mCursorIndex + 1;
		updateCursor();
		break;
	case KEY_DELETE:
		deleteForward();
		break;
	case KEY_V:
		// (GetAsyncKeyState: the key as it is now, not as the messages left it)
		if (getEngine()->isKeyDown(KEY_CONTROL))
			paste();
		break;
	default:
		return;
	}
	restartBlink();
}

void EditBox::keyRepeat(int theKey, int theCount)
{
	for (int i = 0; i < theCount; i++)
		keyDown(theKey);
}

void EditBox::charTyped(char theChar)
{
	if (mText.size() >= (size_t)mMaxLength)
		return;
	if (!isPrintable(theChar))
		return;
	if (mFilter != nullptr)
	{
		theChar = mFilter(theChar);
		if (theChar == 0)
			return;
	}
	if (mFont != nullptr && !mFont->hasGlyph(theChar))
		return;
	std::string aBefore = mText.substr(0, mCursorIndex);
	std::string anAfter = mText.substr(mCursorIndex, mText.size());
	mText = aBefore;
	mText.append(1, theChar);
	mText.append(anAfter);
	mCursorIndex++;
	restartBlink();
	textChanged();
}

void EditBox::deleteBack()
{
	if (mCursorIndex <= 0)
		return;
	std::string aBefore = mText.substr(0, mCursorIndex - 1);
	std::string anAfter = mText.substr(mCursorIndex, mText.size());
	mText = aBefore;
	mText.append(anAfter);
	mCursorIndex--;
	textChanged();
}

void EditBox::deleteForward()
{
	if (mCursorIndex >= (int)mText.size())
		return;
	std::string aBefore = mText.substr(0, mCursorIndex);
	std::string anAfter = mText.substr(mCursorIndex + 1, mText.size());
	mText = aBefore;
	mText.append(anAfter);
	textChanged();
}

void EditBox::paste()
{
	// port: the original opens the clipboard and, when the box is already full, leaves it open (a fix candidate); the
	// host's clipboard is opened and closed again. The original also keeps the copy it makes of the clipboard's text
	// (a leak the port does not repeat)
	std::string aClipboard;
	if (!getEngine()->getClipboardText(aClipboard))
		return;
	int aRoom = mMaxLength - (int)mText.size();
	if (aRoom <= 0)
		return;
	std::string aPasted = aClipboard.substr(0, aRoom);
	std::string aBefore = mText.substr(0, mCursorIndex);
	std::string anAfter = mText.substr(mCursorIndex, mText.size());
	std::string aKept;
	if (mFilter == nullptr)
		aKept = aPasted;
	else
	{
		for (size_t i = 0; i < aPasted.size(); i++)
		{
			char aChar = mFilter(aPasted[i]);
			if (aChar != 0 && mFont != nullptr && mFont->hasGlyph(aChar))
				aKept.append(1, aChar);
		}
	}
	mText = aBefore;
	mText.append(aKept);
	mCursorIndex = (int)mText.size();
	mText.append(anAfter);
	textChanged();
}

void EditBox::textChanged()
{
	if (mTextItem != nullptr)
		mTextItem->setText(mText);
	updateCursor();
}

void EditBox::updateCursor()
{
	if (mCursor == nullptr || mFont == nullptr)
		return;
	int aWidth = mFont->getStringWidth(mText.substr(0, mCursorIndex).c_str());
	Vector2f aPosition = mCursorOffset;
	aPosition.x = (float)aWidth + aPosition.x;
	mCursor->setPosition(aPosition);
}


void EditBox::setText(const std::string& theText)
{
	mText = theText.substr(0, mMaxLength);
	mCursorIndex = (int)mText.size();
	textChanged();
	if ((int)theText.size() < mCursorIndex)
		mCursorIndex = (int)theText.size();
	updateCursor();
}

} // namespace engine
