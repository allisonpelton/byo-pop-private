// The widgets (see Widget.h).

#include "Widget.h"
#include "App.h"
#include "Draw.h"
#include "Events.h"
#include "Font.h"
#include "Graphics.h"
#include "Host.h"
#include "Input.h"
#include "Platform.h"
#include "Resources.h"
#include "Screen.h"
#include "Sound.h"

#include <cmath>

namespace bonnies
{

namespace
{
inline void playSound(Sample* theSample)
{
	gSound->play(theSample, 1.0f, 1, 0, 1.0f);
}
}

// ---- The base

// 0x42c240
Widget::Widget()
{
}

// 0x42c290
Widget::~Widget()
{
}

// 0x42c2f0
void Widget::init()
{
	mId = 0;
	mX = 0.0f;
	mY = 0.0f;
	mWidth = 0.0f;
	mHeight = 0.0f;
	mHitX = 0.0f;
	mHitY = 0.0f;
	mHitWidth = 0.0f;
	mHitHeight = 0.0f;
	mFlags = 0;
	mTextFlags = 0;
	mColor = 0xffffff00;
	mPriority = 0;
	mFocusColor = 0;
	mFocusColor2 = 0;
	mNext = nullptr;
	mPrev = nullptr;
	mImage = nullptr;
	mFont = nullptr;
	mText.clear();
	mTooltip.clear();
	mOwner = nullptr;
	mKeys[0] = 0;
	mKeys[1] = 0;
	mKeys[2] = 0;
	mKeys[3] = 0;
	mSounds[0] = nullptr;
	mSounds[1] = nullptr;
	mSounds[2] = nullptr;
}

// 0x42c370
void Widget::shutdown()
{
	if ((mFlags & WIDGET_FOCUS) != 0)
		unfocus();
	if (mImage != nullptr)
		gResources->release(mImage);
	mImage = nullptr;
	if (mFont != nullptr)
		gResources->release(mFont);
	mFont = nullptr;
	Sample** aSound = mSounds;
	int i = 3;
	do
	{
		if (*aSound != nullptr)
			gResources->release(*aSound);
		aSound++;
		i--;
	} while (i != 0);
}

// 0x42c3e0
void Widget::setFont(FontDesc* theDesc)
{
	if (mFont != nullptr)
		gResources->release(mFont);
	mFont = gResources->getFont(theDesc);
}

// 0x42b9a0
int Widget::contains(float theX, float theY)
{
	return contains(theX, theY, 0, 0.0f);
}

// 0x42b980
int Widget::contains(float theX, float theY, int theCheck)
{
	return contains(theX, theY, theCheck, 0.0f);
}

// 0x42c490
int Widget::contains(float theX, float theY, int theCheck, float theMargin)
{
	if (theCheck != 0 && mType != 1 && mType != 5 && mType != 3 && mType != 4)
		return 0;
	if (isVisible() == 0)
		return 0;
	double dx = (double)theX - mOwner->mX;
	float dy = (float)((double)theY - mOwner->mY);
	double aBottom;
	if (mHitX == 0.0f && mHitY == 0.0f)
	{
		if (dx < (double)mX - theMargin)
			return 0;
		if (!(dx <= ((double)mWidth + mX) + theMargin))
			return 0;
		if (!((double)mY - theMargin <= dy))
			return 0;
		aBottom = ((double)theMargin + mHeight) + mY;
	}
	else
	{
		if (dx < ((double)mHitX + mX) - theMargin)
			return 0;
		if (!(dx <= (((double)mHitWidth + mHitX) + mX) + theMargin))
			return 0;
		if (!(((double)mHitY + mY) - theMargin <= dy))
			return 0;
		aBottom = (((double)mHitHeight + mHitY) + theMargin) + mY;
	}
	if (aBottom < dy)
		return 0;
	return 1;
}

// 0x42c5c0
void Widget::draw()
{
	if (isVisible() == 0 || gApp->mDebugRects[mType] == 0)
		return;
	drawRect((float)((double)mOwner->mX + mX), (float)((double)mOwner->mY + mY), mWidth, mHeight, 0xffff7fff, 1);
	if (mHitX != 0.0f)
		drawRect((float)(((double)mOwner->mX + mX) + mHitX), (float)(((double)mOwner->mY + mHitY) + mY), mHitWidth,
			mHitHeight, 0xffff7f7f, 1);
}

// 0x41f560
void Widget::update()
{
}

// 0x42c670
int Widget::isVisible()
{
	if (mOwner != nullptr && mOwner->isVisible() != 0 && (mFlags & WIDGET_HIDDEN) == 0)
		return 1;
	return 0;
}

// 0x42c6a0
int Widget::isActive()
{
	if (isVisible() != 0 && (mFlags & WIDGET_INACTIVE) == 0)
		return 1;
	return 0;
}

// 0x42c6c0
int Widget::hasFocus()
{
	return mFlags >> 2 & 1;
}

// 0x42c6d0
int Widget::handleEvent(Event* theEvent)
{
	int aType = theEvent->mType;
	if (aType == 1)
	{
		if (contains(theEvent->mX, theEvent->mY, 1) != 0)
		{
			if (mType == 1 && Input::leftButtonDown() && static_cast<Button*>(this)->mMousePressed == 0)
				return 1;
			if ((mFlags & WIDGET_FOCUS) == 0)
				focus();
			return 0;
		}
		if ((mFlags & WIDGET_FOCUS) != 0)
			unfocus();
	}
	else if (1 < aType && aType < 4 && contains(theEvent->mX, theEvent->mY, 0) != 0 && theEvent->mA == 0 &&
		mType != 1)
	{
		gEvents->push((theEvent->mType != 2) + 0xc, mId, 0);
		theEvent->mA = mId;
		return 1;
	}
	return 1;
}

// 0x42c7b0
void Widget::setSound(int theIndex, const Str& theName)
{
	if (mSounds[theIndex] != nullptr)
		gResources->release(mSounds[theIndex]);
	mSounds[theIndex] = gResources->getSound(theName);
}

// 0x42c830
void Widget::focus()
{
	Widget* aFocus = gApp->mFocus;
	if (aFocus != nullptr && aFocus != this)
		aFocus->unfocus();
	mFlags |= WIDGET_FOCUS;
	if (!mTooltip.empty())
		gApp->showStatus(mTooltip);
	gApp->setFocus(this);
}

// 0x42c890
void Widget::unfocus()
{
	if (gApp->mFocus == this)
		gApp->setFocus(nullptr);
	mFlags &= ~WIDGET_FOCUS;
	if (!mTooltip.empty() && !gApp->mStatus.empty() && equalsNoCase(mTooltip, gApp->mStatus))
		gApp->showStatus("");
}

// ---- Type 1, the button

// 0x42b7a0
Button::Button()
{
}

// 0x42b810
Button::~Button()
{
}

// 0x42b890
void Button::init()
{
	mUpName.clear();
	mFocusName.clear();
	mDownName.clear();
	mUpImage = nullptr;
	mFocusImage = nullptr;
	mDownImage = nullptr;
	mUpFrame = 0;
	mFocusFrame = 0;
	mDownFrame = 0;
	mPanelColor = 0xffe7d13a;
	mEdgeColor = 0xff010101;
	mFocusPanelColor = 0;
	Widget::init();
	mKeyPressed = 0;
	mMousePressed = 0;
	mTextX = 0.0f;
	mTextY = 0.0f;
	mTextWidth = 0.0f;
	mTextHeight = 0.0f;
	mType = 1;
}

// 0x42b930
void Button::shutdown()
{
	Widget::shutdown();
	if (mUpImage != nullptr)
		gResources->release(mUpImage);
	if (mFocusImage != nullptr)
		gResources->release(mFocusImage);
	if (mDownImage != nullptr)
		gResources->release(mDownImage);
}

// 0x42b9c0
void Button::draw()
{
	if (isVisible() == 0)
	{
		Widget::draw();
		return;
	}
	uint32_t aColor2 = mColor2;
	uint32_t anEdge = mEdgeColor;
	uint32_t aColor = mColor;
	uint32_t aPanel = mPanelColor;
	float aDX = 0.0f;
	float aDY = 0.0f;
	float aSize = 3.0f;
	if ((mFlags & WIDGET_FOCUS) != 0)
	{
		uint32_t aFocus2 = aColor2;
		if ((mFlags & WIDGET_FOCUS_TEXT) != 0)
		{
			aColor = mFocusColor;
			if (aColor == 0)
				aColor = addRGB(mColor, 0x2d, 0x2d, 0x2d);
			aFocus2 = mFocusColor2;
			if (mFocusColor2 == 0)
				aFocus2 = addRGB(aColor2, 0x2d, 0x2d, 0x2d);
		}
		aColor2 = aFocus2;
		if ((mFlags & WIDGET_FOCUS_PANEL) != 0)
		{
			aPanel = mFocusPanelColor;
			if (aPanel == 0)
				aPanel = addRGB(mPanelColor, 0x19, 0x19, 0x19);
		}
	}
	if ((mFlags & WIDGET_PRESSED) != 0)
	{
		if ((mFlags & (WIDGET_BEVEL | WIDGET_FLAT_PRESS)) == 0)
		{
			aColor = addRGB(aColor, 5, 5, 5);
			aColor2 = addRGB(aColor2, 5, 5, 5);
		}
		else
		{
			aDX = -0.00125f;
			aDY = 0.0016666667f;
		}
		aPanel = addRGB(aPanel, 5, 5, 5);
		aSize = 2.5f;
	}
	if ((mFlags & WIDGET_PANEL) != 0)
		drawPanel((float)(((double)mOwner->mX + mX) + aDX), (float)(((double)mOwner->mY + mY) + aDY), mWidth, mHeight, 7,
			aPanel, 0xfffcff29, 1, anEdge, 2, 0.0f, 0.0f, 0.0f, 0.0f);
	if ((mFlags & WIDGET_BEVEL) != 0)
	{
		float aX = (float)((double)mOwner->mX + mX);
		float aY = (float)((double)mOwner->mY + mY);
		fillRoundRect((float)((double)aX - 0.00375f), (float)((double)aY - 0.0025f), (float)((double)mWidth + 0.0075f),
			(float)((double)mHeight + 0.0075f), 0x24000000, 6);
		fillRoundRect((float)((double)aX - 0.00625f), (float)((double)aY - 0.005f), (float)((double)mWidth + 0.0125f),
			(float)((double)mHeight + 0.01125f), 0x24000000, 6);
		drawBevel((float)((double)aX + aDX), (float)((double)aY + aDY), mWidth, mHeight, aPanel, aPanel, 1, aSize, 2.0f);
	}
	if ((mFlags & WIDGET_IMAGE) != 0)
	{
		int aFrame = mUpFrame;
		Image* anImage = mUpImage;
		if ((mFlags & WIDGET_PRESSED) != 0)
		{
			anImage = mDownImage;
			aFrame = mDownFrame;
		}
		else if ((mFlags & WIDGET_FOCUS) != 0)
		{
			anImage = mFocusImage;
			aFrame = mFocusFrame;
		}
		if (anImage != nullptr)
			drawFrame(&anImage->mFrames[aFrame], (float)((double)mOwner->mX + mX), (float)((double)mOwner->mY + mY), 1.0f,
				0);
	}
	if (mFont != nullptr && !mText.empty())
	{
		double aX = (double)mOwner->mX + mX;
		double aTop;
		float aWidth;
		float aHeight;
		if (mHitX == 0.0f)
		{
			aTop = mOwner->mY;
			aWidth = mWidth;
			aHeight = mHeight;
		}
		else
		{
			aX = aX + mHitX;
			aWidth = mHitWidth;
			aTop = (double)mOwner->mY + mHitY;
			aHeight = mHitHeight;
		}
		if (mTextWidth != 0.0f)
			aWidth = (float)((double)aWidth - ((double)mWidth - mTextWidth));
		if (mTextHeight != 0.0f)
			aHeight = (float)((double)aHeight - ((double)mHeight - mTextHeight));
		mFont->drawText(mText, (float)((aX + mTextX) + aDX), (float)(((aTop + mY) + mTextY) + aDY), aWidth, aHeight,
			aColor, aColor2, mTextFlags, 1.0f, 0.0f);
	}
	Widget::draw();
}

// 0x42bd90
void Button::press()
{
	if ((mFlags & WIDGET_PRESSED) != 0)
		return;
	Sample* aSound = mSounds[0];
	if (aSound != nullptr || (aSound = gApp->mSounds[0]) != nullptr)
		playSound(aSound);
	mFlags |= WIDGET_PRESSED;
	gEvents->push(0xc, mId, 0);
}

// 0x42bdf0
void Button::release()
{
	if ((mFlags & WIDGET_PRESSED) == 0)
		return;
	mFlags &= ~WIDGET_PRESSED;
	Sample* aSound = mSounds[1];
	mMousePressed = 0;
	mKeyPressed = 0;
	if (aSound != nullptr || (aSound = gApp->mSounds[1]) != nullptr)
		playSound(aSound);
	gEvents->push(0xd, mId, 0);
}

// 0x42be60
void Button::update()
{
	if ((mFlags & WIDGET_PRESSED) != 0 && mKeyPressed != 0 && !Input::anyKeyDown())
		release();
	if (mMousePressed != 0 && !Input::leftButtonDown())
	{
		mMousePressed = 0;
		mFlags &= ~WIDGET_PRESSED;
	}
}

// 0x42bec0
int Button::handleEvent(Event* theEvent)
{
	switch (theEvent->mType)
	{
	case 2:
		if (contains(theEvent->mX, theEvent->mY) != 0)
		{
			mMousePressed = 1;
			press();
			return 0;
		}
		break;
	case 3:
		if ((mFlags & WIDGET_PRESSED) != 0)
		{
			if (contains(theEvent->mX, theEvent->mY) != 0 && mMousePressed != 0)
			{
				release();
				return 0;
			}
			mFlags &= ~WIDGET_PRESSED;
		}
		mMousePressed = 0;
		return Widget::handleEvent(theEvent);
	case 8:
	{
		int aKey = theEvent->mA;
		if (aKey != 0 && (aKey == mKeys[0] || aKey == mKeys[1] || aKey == mKeys[2] || aKey == mKeys[3]) &&
			isActive() != 0)
		{
			mKeyPressed = 1;
			press();
			return 0;
		}
		break;
	}
	case 9:
	{
		int aKey = theEvent->mA;
		if (aKey != 0 && (aKey == mKeys[0] || aKey == mKeys[1] || aKey == mKeys[2] || aKey == mKeys[3]))
		{
			if (isActive() != 0)
			{
				release();
				return 0;
			}
			mFlags &= ~WIDGET_PRESSED;
			mMousePressed = 0;
			mKeyPressed = 0;
		}
		break;
	}
	}
	return Widget::handleEvent(theEvent);
}

namespace
{
// 0x42c000, 0x42c0c0, 0x42c180, 0x42df10: an image by name, loaded again only when the name changes
void setNamedImage(Str& theName, Image*& theImage, int& theFrame, const Str& theNewName, int theNewFrame)
{
	if (equalsNoCase(theName, theNewName))
		return;
	if (theImage != nullptr)
		gResources->release(theImage);
	theFrame = theNewFrame;
	theImage = nullptr;
	theName = theNewName;
	if (!theName.empty())
		theImage = gResources->getImage(theName);
}
}

// 0x42c000
void Button::setUpImage(const Str& theName, int theFrame)
{
	setNamedImage(mUpName, mUpImage, mUpFrame, theName, theFrame);
}

// 0x42c0c0
void Button::setFocusImage(const Str& theName, int theFrame)
{
	setNamedImage(mFocusName, mFocusImage, mFocusFrame, theName, theFrame);
}

// 0x42c180
void Button::setDownImage(const Str& theName, int theFrame)
{
	setNamedImage(mDownName, mDownImage, mDownFrame, theName, theFrame);
}

// ---- Type 0, the text field

// 0x42c900
TextField::TextField()
{
}

// 0x42c960
TextField::~TextField()
{
}

// 0x42c9d0
void TextField::init()
{
	Widget::init();
	mType = 0;
	mEditing = 0;
	mEditable = 0;
	mEscapeCancels = 1;
	mTabEnds = 0;
	mNamesOnly = 0;
	mOriginal.clear();
	mDisplay.clear();
	mCursor = 0;
	mBlinkTime = 0;
	mSelecting = 0;
	mBlinkOn = 0;
	mPanelColor = 0;
	mMaxLength = 0x1e;
	mAnchor = -1;
}

// 0x42ca60
int TextField::contains(float theX, float theY)
{
	if (mEditable != 0)
		return Widget::contains(theX, theY, 0, 0.015f);
	return 0;
}

// 0x42ca90
void TextField::draw()
{
	if (mPanelColor != 0)
		drawBevel((float)((double)mOwner->mX + mX), (float)((double)mOwner->mY + mY), mWidth, mHeight, mPanelColor,
			mPanelColor, 0, 3.0f, 2.0f);
	bool aHasText = mEditing == 0 ? !mText.empty() : !mDisplay.empty();
	if (aHasText)
	{
		if (mSelecting != 0 && mAnchor != mCursor)
		{
			mFont->mSelecting = 1;
			mFont->mSelStart = mCursor <= mAnchor ? mCursor : mAnchor;
			mFont->mSelEnd = (mAnchor <= mCursor ? mCursor : mAnchor) - 1;
			if (mBlinkOn != 0)
				mFont->mSelEnd++;
		}
		mFont->drawText(mEditing == 0 ? mText : mDisplay, (float)((double)mOwner->mX + mX),
			(float)((double)mOwner->mY + mY), mWidth, mHeight, mColor, mColor2, mTextFlags, 1.0f, 0.0f);
		mFont->mSelecting = 0;
	}
	Widget::draw();
}

// 0x42cbd0
void TextField::refreshDisplay(int theIgnored)
{
	(void)theIgnored;
	mDisplay = mText;
	uint32_t aBlink = ~((int)(host()->getTicks() - mBlinkTime) / 0x1a4) & 1;
	mBlinkOn = (int)aBlink;
	if (aBlink == 0)
		return;
	if ((int)mText.size() < mCursor)
	{
		mDisplay += '|';
		return;
	}
	Str aText = "";
	if (0 < mCursor)
		aText += substr(mText, 0, mCursor);
	aText += '|';
	if (mCursor < (int)mText.size())
		aText += substr(mText, mCursor, (int)mText.size());
	mDisplay = aText;
}

// 0x42cd10
void TextField::beginEdit()
{
	mEditing = 1;
	mBlinkTime = host()->getTicks();
	mOriginal = mText;
	mCursor = (int)mText.size();
	refreshDisplay();
	gEvents->push(0x13, mId, 0);
}

// 0x42cd70
int TextField::deleteSelection()
{
	if (mSelecting == 0)
		return 0;
	int aStart = mAnchor;
	int anEnd = mCursor;
	if (anEnd <= aStart)
	{
		aStart = anEnd;
		if (anEnd < mAnchor)
			anEnd = mAnchor;
	}
	Str aText = "";
	if (0 < aStart)
		aText += substr(mText, 0, aStart);
	if (anEnd < (int)mText.size())
		aText += substr(mText, anEnd, (int)mText.size());
	mText = aText;
	mCursor = aStart;
	refreshDisplay();
	mSelecting = 0;
	if (aStart != anEnd)
		return 1;
	return 0;
}

// 0x42ce90
int TextField::handleEvent(Event* theEvent)
{
	if (theEvent->mType == 2)
	{
		if (mEditable != 0 && contains(theEvent->mX, theEvent->mY) != 0)
		{
			beginEdit();
			return 0;
		}
		return Widget::handleEvent(theEvent);
	}
	if (theEvent->mType != 8 || mEditing == 0)
		return Widget::handleEvent(theEvent);
	int aKey = theEvent->mA;
	int anOldCursor = mCursor;
	bool aMods = theEvent->mMeta || theEvent->mCtrl || theEvent->mAlt;
	enum { MOVED, ESCAPE, COPY, PASTE, CUT, DELETE, CHARACTER } anAction;
	switch (aKey)
	{
	case SDLK_RIGHT:
		mCursor = anOldCursor + 1;
		anAction = MOVED;
		break;
	case SDLK_LEFT:
		mCursor = anOldCursor - 1;
		anAction = MOVED;
		break;
	case SDLK_INSERT:
		if (theEvent->mCtrl)
			anAction = COPY;
		else if (theEvent->mShift)
			anAction = PASTE;
		else
			return 0;
		break;
	case SDLK_HOME:
	case SDLK_PAGEUP:
	case SDLK_UP:
		mCursor = 0;
		anAction = MOVED;
		break;
	case SDLK_END:
	case SDLK_PAGEDOWN:
	case SDLK_DOWN:
		mCursor = (int)mText.size();
		anAction = MOVED;
		break;
	case SDLK_ESCAPE:
		anAction = ESCAPE;
		break;
	case SDLK_BACKSPACE:
	{
		if (deleteSelection() != 0)
			return 0;
		if (mCursor < 1)
			return 0;
		Str aText = "";
		if (1 < mCursor)
			aText += substr(mText, 0, mCursor - 1);
		if (mCursor < (int)mText.size())
			aText += substr(mText, mCursor, (int)mText.size());
		mText = aText;
		mCursor--;
		refreshDisplay(1);
		return 0;
	}
	case SDLK_TAB:
		if (mTabEnds != 0)
		{
			mEditing = 0;
			gEvents->push(0x12, mId, 0);
			return 0;
		}
		anAction = CHARACTER;
		break;
	case SDLK_RETURN:
		if (!mText.empty())
		{
			mEditing = 0;
			gEvents->push(0x10, mId, 0);
			return 0;
		}
		anAction = ESCAPE;
		break;
	case SDLK_DELETE:
		anAction = theEvent->mShift ? CUT : DELETE;
		break;
	default:
		if (aKey == 'v' && aMods)
			anAction = PASTE;
		else if (aKey == 'c' && aMods)
			anAction = COPY;
		else if (aKey == 'x' && aMods)
			anAction = CUT;
		else
			anAction = CHARACTER;
		break;
	}
	switch (anAction)
	{
	case ESCAPE:
		if (mEscapeCancels != 0)
		{
			mText = mOriginal;
			mEditing = 0;
			gEvents->push(0x11, mId, 0);
		}
		return 0;
	case COPY:
		if (mSelecting != 0)
		{
			int aLow = mAnchor < mCursor ? mAnchor : mCursor;
			int aHigh = mAnchor < mCursor ? mCursor : mAnchor;
			if (aLow != aHigh)
				setClipboard(substr(mText, aLow, aHigh));
		}
		return 0;
	case CUT:
		if (mSelecting != 0)
		{
			int aLow = mAnchor < mCursor ? mAnchor : mCursor;
			int aHigh = mAnchor < mCursor ? mCursor : mAnchor;
			if (aLow != aHigh)
				setClipboard(substr(mText, aLow, aHigh));
			deleteSelection();
			return 0;
		}
		// Without a selection, a cut deletes the character at the cursor
		// fall through
	case DELETE:
	{
		if (deleteSelection() != 0)
			return 0;
		if ((int)mText.size() <= mCursor)
			return 0;
		Str aText = "";
		if (0 < mCursor)
			aText += substr(mText, 0, mCursor);
		if (mCursor + 1 < (int)mText.size())
			aText += substr(mText, mCursor + 1, (int)mText.size());
		mText = aText;
		refreshDisplay();
		return 0;
	}
	case PASTE:
	{
		deleteSelection();
		Str aClip = trim(getClipboard(), true);
		if (aClip.empty())
			return 0;
		Str aText = "";
		if (0 < mCursor)
			aText += substr(mText, 0, mCursor);
		aText += aClip;
		int aNewCursor = (int)aText.size();
		if (mCursor < (int)mText.size())
			aText += substr(mText, mCursor, (int)mText.size());
		mText = aText;
		mCursor = aNewCursor;
		break;
	}
	case CHARACTER:
	{
		int aChar = theEvent->mB;
		if (aChar == 0)
			aChar = aKey;
		if (aChar < 0x20 || 0xff < aChar || aChar == '|')
			return 0;
		if ((int)mText.size() >= mMaxLength)
			return 0;
		if (mNamesOnly != 0 && aChar != ' ' && aChar != '_' && (aChar < '0' || '9' < aChar) &&
			(aChar < 'A' || 'Z' < aChar) && (aChar < 'a' || 'z' < aChar) && aChar < 0xc4)
			return 0;
		deleteSelection();
		Str aText = "";
		if (0 < mCursor)
			aText += substr(mText, 0, mCursor);
		aText += (char)(unsigned char)aChar;
		if (mCursor < (int)mText.size())
			aText += substr(mText, mCursor, (int)mText.size());
		mText = aText;
		mCursor++;
		refreshDisplay();
		return 0;
	}
	case MOVED:
		break;
	}
	// The cursor moved (or text was pasted): shift extends the selection from where the cursor was
	if (!theEvent->mShift || aKey == SDLK_INSERT)
		mSelecting = 0;
	else if (mSelecting == 0)
	{
		mSelecting = 1;
		mAnchor = anOldCursor;
	}
	if (mCursor < 0)
		mCursor = 0;
	if ((int)mText.size() < mCursor)
		mCursor = (int)mText.size();
	refreshDisplay();
	return 0;
}

// 0x42d6a0
void TextField::update()
{
	if (mEditing != 0)
		refreshDisplay();
}

// ---- Type 3, the check box

// 0x42d6c0
CheckBox::CheckBox()
{
}

// 0x42d6e0
CheckBox::~CheckBox()
{
}

// 0x42d6f0
void CheckBox::init()
{
	Widget::init();
	mType = 3;
	mChecked = 0;
	mBoxWidth = 0.1f;
	mBoxHeight = 0.1f;
}

// 0x42d720
void CheckBox::toggle()
{
	mFlags &= ~WIDGET_PRESSED;
	mChecked = mChecked == 0;
	gEvents->push(0xe, mId, 0);
}

// 0x42d750
void CheckBox::update()
{
	if (!Input::leftButtonDown() && (mFlags & WIDGET_PRESSED) != 0)
		toggle();
}

// 0x42d7a0
void CheckBox::draw()
{
	if (isVisible() == 0)
	{
		Widget::draw();
		return;
	}
	float aX = (float)((double)mOwner->mX + mX);
	float aTop = (float)((double)mOwner->mY + mY);
	float aY = (float)(((double)mHeight - mBoxHeight) * 0.5f + aTop);
	drawBevel(aX, aY, mBoxWidth, mBoxHeight, 0xff2f2f2f, 0xff2f2f2f, 0, 4.0f, 1.0f);
	if (mChecked != 0)
	{
		float aLeft = (float)((double)aX + 0.01f);
		drawLine(aLeft, (float)((double)aY + 0.011666667f), (float)(((double)aX + mBoxWidth) - 0.01f),
			(float)(((double)aY + mBoxHeight) - 0.013333334f), 0xffdcc491, 3, 1);
		drawLine(aLeft, (float)(((double)aY + mBoxHeight) - 0.013333334f), (float)(((double)aX + mBoxWidth) - 0.01f),
			(float)((double)aY + 0.011666667f), 0xffdcc491, 3, 1);
	}
	if (mFont != nullptr && !mText.empty())
	{
		double aGap = (double)mBoxWidth + 0.00625f;
		mFont->drawText(mText, (float)(aGap + aX), aTop, (float)((double)mWidth - aGap), mHeight, mColor, mColor2,
			mTextFlags, 1.0f, 0.0f);
	}
	Widget::draw();
}

// 0x42d940
void CheckBox::press()
{
	if ((mFlags & WIDGET_PRESSED) != 0)
		return;
	Sample* aSound = mSounds[2];
	if (aSound != nullptr || (aSound = gApp->mSounds[2]) != nullptr)
		playSound(aSound);
	mFlags |= WIDGET_PRESSED;
}

// 0x42d980
int CheckBox::handleEvent(Event* theEvent)
{
	if (theEvent->mType == 2)
	{
		if (contains(theEvent->mX, theEvent->mY) != 0)
		{
			press();
			return 0;
		}
	}
	else if (theEvent->mType == 3 && (mFlags & WIDGET_PRESSED) != 0)
	{
		if (contains(theEvent->mX, theEvent->mY) != 0)
		{
			toggle();
			return 0;
		}
		mFlags &= ~WIDGET_PRESSED;
	}
	return Widget::handleEvent(theEvent);
}

// ---- Type 4, the slider

// 0x42d9f0
Slider::Slider()
{
}

// 0x42da40
Slider::~Slider()
{
}

// 0x42daa0
void Slider::init()
{
	Widget::init();
	mType = 4;
	mKnobImage = nullptr;
	mKnobName = "";
	mKnobFrame = 0;
	mValue = 0.0f;
	mSteps = 0;
	mMaxStep = 0;
	mWidth = 0.015f;
	mTrackColor = 0xff2f2f2f;
	mKnobColor = 0xff8f8f8f;
}

// 0x42db00
void Slider::shutdown()
{
	Widget::shutdown();
	if (mKnobImage != nullptr)
		gResources->release(mKnobImage);
}

// 0x42db20
void Slider::update()
{
	if (!Input::leftButtonDown() && (mFlags & WIDGET_PRESSED) != 0)
		mFlags &= ~WIDGET_PRESSED;
}

// 0x42db50
void Slider::draw()
{
	float aX = (float)((double)mOwner->mX + mX);
	float aY = (float)(((double)mHeight - 0.008333334f) * 0.5f + ((double)mOwner->mY + mY));
	drawBevel((float)((double)aX + 0.03f), aY, (float)((double)mWidth - 0.06f), 0.008333334f, mTrackColor, mTrackColor,
		0, 2.0f, 1.0f);
	float aKnob = 0.015f;
	if (mKnobImage != nullptr)
		aKnob = 0.0f;
	float aKnobX = (float)((((double)mWidth - aKnob) - 0.06f) * mValue + 0.03f + aX);
	float aKnobY = (float)(((double)aY + 0.004166667f) - 0.018333333f);
	uint32_t anEdge = addColor(mKnobColor, 0, -10, -10, -10);
	if (mKnobImage != nullptr)
	{
		const Frame* aFrame = &mKnobImage->mFrames[mKnobFrame];
		float aHalf = (float)((double)aFrame->mWidth * 0.5f);
		drawFrame(aFrame, (float)((double)aKnobX - (double)aHalf / screenWidth()), (float)((double)aKnobY - 0.005f), 1.0f,
			0);
		Widget::draw();
		return;
	}
	drawBevel(aKnobX, aKnobY, aKnob, 0.036666665f, mKnobColor, anEdge, 1, 2.0f, 2.0f);
	Widget::draw();
}

// 0x42dd00
void Slider::track()
{
	float aMargin = 0.015f;
	if (mKnobImage != nullptr)
		aMargin = 0.0f;
	float aLow = (float)((double)aMargin + mOwner->mX + mX + 0.03f);
	float aHigh = (float)((((double)mWidth + mOwner->mX) + mX - aMargin) - 0.03f);
	double aMouse = gInput->mouseX();
	if (aMouse <= aLow)
		aMouse = aLow;
	if (!(aMouse < aHigh))
		aMouse = aHigh;
	double aRange = (double)aHigh - aLow;
	if (!(0.001f <= aRange))
		aRange = 0.001f;
	float aValue = (float)((aMouse - aLow) / aRange);
	if (std::fabs((double)mValue - aValue) <= 0.005f)
		return;
	double anOld = mValue;
	mValue = aValue;
	if (1 < mSteps)
	{
		double aLast = mSteps - 1;
		int anOldStep = ftol(aLast * anOld + 0.5f);
		int aStep = ftol(aLast * aValue + 0.5f);
		bool aFast = false;
		if (Input::leftButtonDown())
		{
			if (500 < (int)(host()->getTicks() - gInput->mDownTime) ||
				!(distance(gInput->mouseX(), gInput->mouseY(), gInput->mDownX, gInput->mDownY) < 0.005f))
				aFast = true;
		}
		if (0 < mMaxStep && !aFast)
		{
			int aClamped = aStep;
			if (anOldStep + mMaxStep < aStep)
			{
				aStep = anOldStep + mMaxStep;
				aClamped = aStep;
			}
			if (aStep < anOldStep - mMaxStep)
				aClamped = anOldStep - mMaxStep;
			mValue = (float)((double)aClamped / (double)(mSteps - 1));
		}
		else
			mValue = (float)((double)ftol(aLast * mValue + 0.5f) / aLast);
	}
	gEvents->push(0xf, mId, 0);
}

// 0x42df10
void Slider::setKnob(const Str& theName, int theFrame)
{
	setNamedImage(mKnobName, mKnobImage, mKnobFrame, theName, theFrame);
}

// 0x42dfd0
int Slider::handleEvent(Event* theEvent)
{
	int aType = theEvent->mType;
	if (aType == 1)
	{
		if ((mFlags & WIDGET_PRESSED) != 0)
			track();
	}
	else if (aType == 2)
	{
		if (contains(theEvent->mX, theEvent->mY) != 0)
		{
			mFlags |= WIDGET_PRESSED;
			track();
			return 0;
		}
	}
	else if (aType == 3 && (mFlags & WIDGET_PRESSED) != 0)
	{
		if (contains(theEvent->mX, theEvent->mY) != 0)
		{
			mFlags &= ~WIDGET_PRESSED;
			return 0;
		}
		mFlags &= ~WIDGET_PRESSED;
		return Widget::handleEvent(theEvent);
	}
	return Widget::handleEvent(theEvent);
}

// ---- Type 5, the list

// 0x42f660
ListItem::ListItem()
{
	mText = "";
	mData = 0;
	mSpare = 0;
}

// 0x42f6d0
ListItem& ListItem::operator=(const ListItem& theItem)
{
	if (this != &theItem)
	{
		mText = theItem.mText;
		mData = theItem.mData;
		mSpare = theItem.mSpare;
	}
	return *this;
}

// 0x42e070
ListBox::ListBox()
{
}

// 0x42e090
ListBox::~ListBox()
{
}

// 0x42e0a0
void ListBox::init()
{
	Widget::init();
	mType = 5;
	mCount = 0;
	// A new[] of one item (the count 0 makes it unused)
	ListItem* anItems = new ListItem[1];
	mTop = 0;
	mListX = 0.0f;
	mListY = 0.0f;
	mClickTime = 0;
	mTracking = 0;
	mScrollTime = 0;
	mItems = anItems;
	mRows = 4;
	mSelected = -1;
	mRowWidth = 1.0f;
	mRowHeight = 1.0f;
}

// 0x42e170
void ListBox::shutdown()
{
	Widget::shutdown();
	if (mItems != nullptr)
		delete[] mItems;
}

// 0x42e1b0
void ListBox::scroll(int theRows)
{
	int aTop = mTop + theRows;
	mTop = aTop;
	if (aTop < 0)
		mTop = 0;
	if (mCount <= mRows - 1 + mTop)
	{
		int aLast = mCount - mRows;
		mTop = aLast < 0 ? 0 : aLast;
	}
}

// 0x42e200
void ListBox::select(int theIndex)
{
	int aCount = mCount;
	mSelected = theIndex;
	if (aCount <= theIndex)
		mSelected = aCount - 1;
	int aRows = mRows;
	int aSelected = mSelected;
	if (aRows - 1 + mTop < aSelected)
		mTop = (aSelected - aRows) + 1;
	if (-1 < aSelected && aSelected < mTop)
		mTop = aSelected;
	if (aCount <= aRows - 1 + mTop)
	{
		int aLast = aCount - aRows;
		mTop = aLast < 0 ? 0 : aLast;
	}
	mScrollTime = host()->getTicks();
	gEvents->push(0x14, mId, 0);
}

// 0x42e2a0
int ListBox::handleEvent(Event* theEvent)
{
	switch (theEvent->mType)
	{
	case 1:
		if (mTracking == 0)
			break;
		// fall through
	case 2:
	{
		uint32_t aNow = host()->getTicks();
		uint32_t aLastClick = mClickTime;
		if (theEvent->mType == 2)
			mClickTime = host()->getTicks();
		if (contains(theEvent->mX, theEvent->mY) == 0)
			break;
		int aRow = ftol(((double)theEvent->mY - (((double)mOwner->mY + mListY) + mY)) / mRowHeight) + mTop;
		if (aRow < mCount)
		{
			mTracking = 1;
			if (theEvent->mType != 1)
				mScrollTime = host()->getTicks();
			if (aRow == mSelected)
			{
				if (theEvent->mType != 1 && (int)(aNow - aLastClick) < 500)
				{
					gEvents->push(0x15, mId, 0);
					return 0;
				}
			}
			else
			{
				select(aRow);
				if (theEvent->mType == 1)
					mClickTime = 0;
			}
		}
		return 0;
	}
	case 3:
	case 6:
	case 7:
		mTracking = 0;
		break;
	}
	return Widget::handleEvent(theEvent);
}

// 0x42e3d0
void ListBox::insert(const ListItem& theItem, int theIndex)
{
	ListItem* anItems = new ListItem[mCount + 1];
	int i = 0;
	for (;;)
	{
		int aStop = mCount;
		if (theIndex < mCount)
			aStop = theIndex;
		if (aStop <= i)
			break;
		anItems[i] = mItems[i];
		i++;
	}
	anItems[i] = theItem;
	i++;
	if (i < mCount + 1)
	{
		do
		{
			anItems[i] = mItems[i - 1];
			i++;
		} while (i < mCount + 1);
	}
	if (mItems != nullptr)
		delete[] mItems;
	mItems = anItems;
	mCount++;
}

// 0x42e510
void ListBox::insert(const Str& theText, int theData, int theIndex)
{
	ListItem anItem;
	anItem.mText = theText;
	anItem.mData = theData;
	anItem.mSpare = 0;
	insert(anItem, theIndex);
}

// 0x42e5b0
void ListBox::remove(int theIndex)
{
	if (theIndex < mCount - 1)
	{
		do
		{
			mItems[theIndex] = mItems[theIndex + 1];
			theIndex++;
		} while (theIndex < mCount - 1);
	}
	int aCount = mCount;
	mCount = aCount - 1;
	if (aCount - 1 <= mSelected)
	{
		mSelected = aCount - 2;
		gEvents->push(0x14, mId, 0);
	}
	if (mCount <= mTop - 1 + mRows)
	{
		int aLast = mCount - mRows;
		mTop = aLast < 0 ? 0 : aLast;
	}
}

// 0x42e660
void ListBox::clear()
{
	while (mCount != 0)
		remove(0);
}

// 0x42e690
void ListBox::draw()
{
	float aX = (float)(((double)mOwner->mX + mListX) + mX);
	float aY = (float)(((double)mOwner->mY + mListY) + mY);
	drawBevel(aX, aY, mWidth, mHeight, 0x8f341f12, 0x8f341f12, 0, 2.0f, 2.0f);
	int i = 0;
	if (0 < mRows)
	{
		do
		{
			int anIndex = mTop + i;
			if (mCount <= anIndex)
				break;
			if (mFont != nullptr && !mItems[anIndex].mText.empty())
			{
				if (anIndex == mSelected)
					fillRect(aX, aY, (float)((double)mRowWidth - 0.0025f), mRowHeight, 0x3f000000);
				mFont->drawText(mItems[anIndex].mText, aX, aY, mRowWidth, mRowHeight, mColor, 0, mTextFlags, 1.0f, 0.0f);
			}
			aY = (float)((double)aY + mRowHeight);
			i++;
		} while (i < mRows);
	}
	Widget::draw();
}

// 0x42e7e0
void ListBox::update()
{
	if (mTracking == 0)
		return;
	float aMouseX;
	float aMouseY;
	gInput->getMousePos(&aMouseX, &aMouseY);
	double dx = (double)aMouseX - (((double)mOwner->mX + mListX) + mX);
	if (dx < -0.0625f)
		return;
	if (!(dx <= (double)mRowWidth + 0.0625f))
		return;
	int aRow = ftol(((double)aMouseY - (((double)mOwner->mY + mListY) + mY)) / mRowHeight);
	if (-1 < aRow && aRow < mRows)
	{
		mScrollTime = 0;
		return;
	}
	if (mScrollTime == 0)
	{
		mScrollTime = host()->getTicks();
		return;
	}
	if (100 < (int)(host()->getTicks() - mScrollTime))
	{
		mScrollTime = 0;
		int anIndex = (-1 < aRow ? 2 : 0) - 1 + mSelected;
		if (-1 < anIndex && anIndex < mCount)
			select(anIndex);
	}
}

// ---- Type 6, the plain area

// 0x42e910
Area::Area()
{
}

// 0x42e930
Area::~Area()
{
}

// 0x42e940
void Area::init()
{
	Widget::init();
	mType = 6;
}

} // namespace bonnies
