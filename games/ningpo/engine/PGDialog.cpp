// PGDialog and its items (see PGDialog.h).

#include "PGDialog.h"
#include "HDisplay.h"
#include "HInput.h"
#include "PGApp.h"
#include "PGControl.h"
#include "PGSprite.h"

#include <cstring>
#include <vector>

namespace pg
{

PGDialogItem* const PGDialog::TIMED_OUT = reinterpret_cast<PGDialogItem*>(intptr_t(-1));
PGDialogItem* const PGDialog::CLOSED = reinterpret_cast<PGDialogItem*>(intptr_t(-2));

// 0x4c1d60: the open dialogs, the top one first (the original's HObjectList adds at the front)
static std::vector<PGDialog*> gDialogStack;
// 0x4c1d80
static HRectList gDialogDirty;

void InvalidateDialogRect(const PGRect* theRect)
{
	gDialogDirty.add(*theRect);
	if (GetApp() != nullptr)
		GetApp()->invalidateRect(theRect);
}

void DrawDialogs(HDisplay* theDisplay)
{
	int i = (int)gDialogStack.size();
	while (--i >= 0)
		gDialogStack[i]->draw(theDisplay);
}

// The clipping of the buttons and the sliders (0x412240, 0x412ee0): theRect is clamped into theClip and drawn when
// both are not empty and they overlap; theSrc is the part of the sprite that lands there
static bool ClampToClip(const PGRect* theRect, const PGRect* theClip, PGRect* theDest, PGRect* theSrc)
{
	int aLeft = theRect->left;
	int aTop = theRect->top;
	int aRight = theRect->right;
	int aBottom = theRect->bottom;
	if (aRight == aLeft || aBottom == aTop)
		return false;
	if (theClip->left == theClip->right || theClip->bottom == theClip->top)
		return false;
	bool anOverlapX = !((aLeft < theClip->left || theClip->right < aLeft) &&
		(theClip->right < aRight || aRight < theClip->left));
	bool anOverlapY = !((aTop < theClip->top || theClip->bottom < aTop) &&
		(theClip->bottom < aBottom || aBottom < theClip->top));
	if (!anOverlapX && aLeft <= theClip->left && theClip->right <= aRight)
		anOverlapX = true;
	if (!anOverlapY && aTop <= theClip->top && theClip->bottom <= aBottom)
		anOverlapY = true;
	if (!anOverlapX || !anOverlapY)
		return false;
	auto Clamp = [](int v, int lo, int hi) { return v < lo ? lo : (hi < v ? hi : v); };
	theDest->left = Clamp(aLeft, theClip->left, theClip->right);
	theDest->right = Clamp(aRight, theClip->left, theClip->right);
	theDest->top = Clamp(aTop, theClip->top, theClip->bottom);
	theDest->bottom = Clamp(aBottom, theClip->top, theClip->bottom);
	theSrc->left = theDest->left - aLeft;
	theSrc->right = theDest->right - aLeft;
	theSrc->top = theDest->top - aTop;
	theSrc->bottom = theDest->bottom - aTop;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialogItem

PGDialogItem::PGDialogItem()
{
	mRefCount = 0;
	mRect = {0, 0, 0, 0};
	mUserData = 0;
	mPosition = {0, 0};
	mNext = nullptr;
	mDialog = nullptr;
	mVisible = true;
	mName[0] = 0;
}

void PGDialogItem::setPosition(const PGPoint* thePosition)
{
	mPosition = *thePosition;
	updateRect();
}

void PGDialogItem::setName(const char* theName)
{
	HStrncpy(mName, theName, 0x100);
}

void PGDialogItem::invalidate(const PGRect* theRect)
{
	if (mDialog != nullptr)
		mDialog->invalidate(theRect);
}

void PGDialogItem::setOwner(PGDialog* theDialog)
{
	mDialog = theDialog;
	updateRect();
	invalidate(getRect());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialogButton

PGDialogButton::PGDialogButton()
{
	mHover = false;
	mLit = nullptr;
	mNormal = nullptr;
	mDisabled = nullptr;
}

bool PGDialogButton::handleEvent(const PGEvent* theEvent)
{
	if (!isVisible())
		return false;
	if (theEvent->mType == EV_MOUSE_MOVE)
	{
		const PGRect* aRect = getRect();
		if (theEvent->mX < aRect->left || aRect->right < theEvent->mX || theEvent->mY < aRect->top ||
			aRect->bottom < theEvent->mY)
		{
			if (mHover)
				invalidate(getRect());
			mHover = false;
			return false;
		}
		if (!mHover)
			invalidate(getRect());
		mHover = true;
	}
	else if (theEvent->mType == EV_DOWN)
	{
		if (theEvent->mCode == BUTTON_LEFT)
		{
			const PGRect* aRect = getRect();
			if (aRect->left <= theEvent->mX && theEvent->mX <= aRect->right && aRect->top <= theEvent->mY &&
				theEvent->mY <= aRect->bottom)
				return true;
		}
		if (theEvent->mChar == '\r')
			return true;
	}
	return false;
}

void PGDialogButton::setSprites(PGSprite* theNormal, PGSprite* theLit, PGSprite* theDisabled)
{
	mNormal = theNormal;
	mLit = theLit;
	mDisabled = theDisabled;
	updateRect();
	invalidate(getRect());
}

void PGDialogButton::updateRect()
{
	int aLeft = mPosition.x;
	int aTop = mPosition.y;
	if (mDialog != nullptr)
	{
		aLeft += mDialog->mRect.left;
		aTop += mDialog->mRect.top;
	}
	// port: the original leaves right and bottom unset without a sprite
	int aRight = aLeft;
	int aBottom = aTop;
	if (mNormal != nullptr)
	{
		aRight = mNormal->width() + aLeft;
		aBottom = mNormal->height() + aTop;
	}
	if (mLit != nullptr)
	{
		aRight = mLit->width() + aLeft;
		aBottom = mLit->height() + aTop;
	}
	if (mDisabled != nullptr)
	{
		aRight = mDisabled->width() + aLeft;
		aBottom = mDisabled->height() + aTop;
	}
	mRect.left = aLeft;
	mRect.top = aTop;
	mRect.right = aRight;
	mRect.bottom = aBottom;
}

void PGDialogButton::draw(HDisplay* theDisplay, const PGRect* theClip)
{
	(void)theDisplay;
	PGSprite* aSprite = mNormal;
	if (!isVisible())
		aSprite = mDisabled;
	else if (mHover)
		aSprite = mLit;
	if (aSprite == nullptr)
		return;
	PGRect aDest;
	PGRect aSrc;
	if (ClampToClip(getRect(), theClip, &aDest, &aSrc))
		aSprite->blitEx(&aDest, &aSrc, BLEND_ALPHA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialogText

PGDialogText::PGDialogText()
{
	mFlags = 0;
	mText[0] = 0;
	mLength = 0;
	mFont = nullptr;
	mEditable = false;
	mAlign = 0;
	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mMaxLength = 0x3ff;
	mVisibleLength = 0x40;
	mRect = {-1, -1, -1, -1};
}

void PGDialogText::setColor(const float* theColor)
{
	mColor[0] = theColor[0];
	mColor[1] = theColor[1];
	mColor[2] = theColor[2];
	mColor[3] = theColor[3];
}

void PGDialogText::setText(const char* theText)
{
	HStrncpy(mText, theText, mMaxLength + 1);
	mLength = (int)strlen(mText);
	if (mLength < 0)
		mLength = 0;
	if (getRect()->left == -1 && getRect()->right == -1 && getRect()->top == -1 && getRect()->bottom == -1)
	{
		// the original logs "uninitialized text rect" and redraws the whole dialog
		if (mDialog != nullptr)
			mDialog->invalidate(&mDialog->mRect);
		return;
	}
	if (mDialog != nullptr)
		mDialog->invalidate(getRect());
}

// 0x4c3d98: when the blinking lines were last redrawn
static PyTime gTextBlinkTime = 0;

bool PGDialogText::handleEvent(const PGEvent* theEvent)
{
	int aType = theEvent->mType;
	if (aType == EV_DOWN && mEditable)
	{
		if (theEvent->mCode == BUTTON_LEFT)
		{
			const PGRect* aRect = getRect();
			PGDialog* aDialog = mDialog;
			if (aRect->left + aDialog->mRect.left <= theEvent->mX &&
				theEvent->mX <= aRect->right + aDialog->mRect.left &&
				aRect->top + aDialog->mRect.top <= theEvent->mY &&
				theEvent->mY <= aRect->bottom + aDialog->mRect.top)
			{
				aDialog->setFocus(this);
				return true;
			}
		}
		else if (mDialog->mFocus == this)
		{
			uint8_t aChar = theEvent->mChar;
			invalidate(&mDialog->mRect);
			if (aChar == 8)
			{
				int aLength = mLength;
				if (aLength > 0)
				{
					mLength = aLength - 1;
					mText[aLength - 1] = 0;
					return false;
				}
			}
			else
			{
				if (aChar == '\r')
					return true;
				if (aChar > 0x1f && mLength < mMaxLength)
				{
					mText[mLength] = (char)aChar;
					int aLength = mLength;
					mLength = aLength + 1;
					mText[aLength + 1] = 0;
					return false;
				}
			}
		}
	}
	else if (aType == EV_PASTE && mEditable)
	{
		if (mDialog->mFocus == this)
		{
			// 0x414122: the clipboard's text
			std::string aClip = GetHost()->ClipboardText();
			char aBuf[0x400];
			HStrncpy(aBuf, aClip.c_str(), 0x400);
			if (!aClip.empty())
			{
				const char* p = aBuf;
				while (*p == ' ')
					p++;
				setText(p);
			}
			return true;
		}
	}
	else if (aType == EV_TICK && ((getFlags() & TEXT_BLINK) != 0 || mEditable))
	{
		if (gTextBlinkTime == 0)
		{
			gTextBlinkTime = theEvent->mTime;
			return false;
		}
		if (PyTimeSub(theEvent->mTime, gTextBlinkTime) > 250 && mDialog != nullptr)
		{
			const PGRect* aRect = getRect();
			PGRect anInvalid = {aRect->left + mDialog->mRect.left, aRect->top + mDialog->mRect.top,
				aRect->right + mDialog->mRect.left, aRect->bottom + mDialog->mRect.top};
			invalidate(&anInvalid);
			gTextBlinkTime = theEvent->mTime;
		}
	}
	return false;
}

void PGDialogText::draw(HDisplay* theDisplay, const PGRect* theClip)
{
	char aBuf[0x100];
	memset(aBuf, 0, sizeof(aBuf));
	if (!isVisible())
		return;
	int aX = mPosition.x;
	int aY = mPosition.y;
	if (mDialog != nullptr)
	{
		aX += mDialog->mRect.left;
		aY += mDialog->mRect.top;
	}
	int aStart = mLength - mVisibleLength;
	if (aStart < 0)
		aStart = 0;
	HStrncpy(aBuf, mText + aStart, 0xfe);
	// the cursor, every other half second
	if (mEditable && mDialog->mFocus == this && ((int)HTimeGetTime() / 500 & 1) != 0)
	{
		size_t aLen = strlen(aBuf);
		aBuf[aLen] = '|';
		aBuf[aLen + 1] = 0;
	}
	if ((getFlags() & TEXT_BLINK) != 0 && ((int)HTimeGetTime() / 500 & 1) != 0)
		return;
	theDisplay->drawText(aBuf, aX, aY, theClip, mAlign, mFont, mColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialogImage

PGDialogImage::PGDialogImage()
{
	mSprite = nullptr;
	mProgress = 0.0f;
}

void PGDialogImage::setSprite(PGSprite* theSprite)
{
	mSprite = theSprite;
	mRect.left = mPosition.x;
	mRect.right = theSprite->width() + mRect.left;
	mRect.top = mPosition.y;
	mRect.bottom = theSprite->height() + mRect.top;
}

void PGDialogImage::setPosition(const PGPoint* thePosition)
{
	PGDialogItem::setPosition(thePosition);
	if (mSprite != nullptr)
	{
		mRect.left = mPosition.x;
		mRect.top = mPosition.y;
		mRect.right = mSprite->width() + mRect.left;
		mRect.bottom = mSprite->height() + mRect.top;
	}
}

void PGDialogImage::setProgress(float theProgress)
{
	mProgress = theProgress;
	if (theProgress < 0.0f)
		mProgress = 0.0f;
	if (1.0f < mProgress)
		mProgress = 1.0f;
	invalidate(getRect());
}

void PGDialogImage::draw(HDisplay* theDisplay, const PGRect* theClip)
{
	(void)theDisplay;
	int aHeight = mSprite->height();
	int aWidth = (int)(mSprite->width() * mProgress);
	PGRect aDest = {mPosition.x, mPosition.y, aWidth + mPosition.x, aHeight + mPosition.y};
	PGRect aSrc = {0, 0, aWidth, aHeight};
	if (!ClipToRect(&aDest, &aSrc, theClip))
		return;
	mSprite->blitEx(&aDest, &aSrc, BLEND_ALPHA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialogSlider

PGDialogSlider::PGDialogSlider()
{
	setRange(0, 0xfa);
	mKnob = nullptr;
	mValue = 0.0f;
	mDragging = false;
}

bool PGDialogSlider::handleEvent(const PGEvent* theEvent)
{
	int aType = theEvent->mType;
	if (aType == EV_DOWN && theEvent->mCode == BUTTON_LEFT)
	{
		const PGRect* aRect = getRect();
		if (aRect->left <= theEvent->mX && theEvent->mX <= aRect->right && aRect->top <= theEvent->mY &&
			theEvent->mY <= aRect->bottom)
		{
			mDragging = true;
			mDialog->setFocus(this);
			return true;
		}
	}
	else if (aType == EV_UP && theEvent->mCode == BUTTON_LEFT)
	{
		if (mDragging)
		{
			mDragging = false;
			mDialog->setFocus(nullptr);
			return true;
		}
	}
	else if (aType == EV_MOUSE_MOVE && mDragging)
	{
		PGDialog* aDialog = mDialog;
		float aValue = (float)((theEvent->mX - aDialog->mRect.left) - mPosition.x) / (float)mLength;
		if (aValue < 0.0f)
			aValue = 0.0f;
		else if (1.0f < aValue)
			aValue = 1.0f;
		aDialog->invalidate(getRect());
		setValue(aValue);
		mDialog->invalidate(getRect());
		return true;
	}
	return false;
}

void PGDialogSlider::setValue(float theValue)
{
	mValue = theValue;
	updateRect();
}

void PGDialogSlider::setKnob(PGSprite* theKnob)
{
	mKnob = theKnob;
	updateRect();
}

void PGDialogSlider::updateRect()
{
	if (mKnob == nullptr)
		return;
	PGRect aKnob;
	mKnob->getRect(&aKnob);
	int aLeft = mPosition.x - aKnob.right / 2;
	int aTop = mPosition.y - aKnob.bottom / 2;
	if (mDialog != nullptr)
	{
		aLeft += mDialog->mRect.left;
		aTop += mDialog->mRect.top;
	}
	int aRight = aKnob.right + aLeft;
	int aBottom = aKnob.bottom + aTop;
	int anOffset = (int)(mValue * (float)mLength);
	if (mVertical == 0)
	{
		aLeft += anOffset;
		aRight += anOffset;
	}
	else
	{
		aTop += anOffset;
		aBottom += anOffset;
	}
	mRect.left = aLeft;
	mRect.top = aTop;
	mRect.right = aRight;
	mRect.bottom = aBottom;
}

void PGDialogSlider::draw(HDisplay* theDisplay, const PGRect* theClip)
{
	(void)theDisplay;
	if (mKnob == nullptr)
		return;
	PGRect aDest;
	PGRect aSrc;
	if (ClampToClip(getRect(), theClip, &aDest, &aSrc))
		mKnob->blitEx(&aDest, &aSrc, BLEND_ALPHA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGDialog

PGDialog::PGDialog()
{
	mRefCount = 0;
	mPosition = {0, 0};
	mMouse = {0, 0};
	mRect = {0, 0, 0, 0};
	mFirstItem = nullptr;
	mBackground = nullptr;
	mApp = nullptr;
	mParam = 0;
	mFocus = nullptr;
	mDefault = nullptr;
	gDialogDirty.clear();
}

void PGDialog::removeAll()
{
	mFirstItem = nullptr;
	mBackground = nullptr;
	setDefault(nullptr);
	setFocus(nullptr);
}

void PGDialog::addItem(PGDialogItem* theItem)
{
	theItem->setNext(nullptr);
	theItem->setOwner(this);
	if (mDefault == nullptr)
		setDefault(theItem);
	setFocus(theItem);
	PGDialogItem* anItem = mFirstItem;
	if (anItem != nullptr)
	{
		while (anItem->getNext() != nullptr)
			anItem = anItem->getNext();
		anItem->setNext(theItem);
		return;
	}
	mFirstItem = theItem;
}

bool PGDialog::handleEvent(const PGEvent* theEvent)
{
	int aType = theEvent->mType;
	if (aType == EV_QUIT)
		return false;
	if (aType == EV_DOWN)
	{
		// Tab: the focus to the next item that takes it
		PGDialogItem* aStart = mFocus;
		if (theEvent->mChar == '\t' && aStart != nullptr)
		{
			PGDialogItem* anItem = aStart;
			while (true)
			{
				mFocus = anItem->getNext();
				if (mFocus == nullptr)
					mFocus = mFirstItem;
				if (mFocus->canFocus())
					break;
				anItem = mFocus;
				if (mFocus == aStart)
					return true;
			}
		}
	}
	else if (aType == EV_PAINT)
	{
		mApp->invalidateAll();
		InvalidateDialogRect(&mRect);
		return true;
	}
	// a mouse move redrew the sprite cursor (0x411580); port: Ning Po has none (0x401a30)
	return true;
}

PGDialogItem* PGDialog::doModal(PGApp* theApp, HDisplay* theDisplay, HInput* theInput, int theTimeout, int theParam)
{
	PGEvent anEvent;
	memset(&anEvent, 0, sizeof(anEvent));
	PGDialogItem* aResult = nullptr;
	gDialogStack.insert(gDialogStack.begin(), this);
	mApp = theApp;
	mParam = theParam;
	PyTime aStart = theApp->mEventTime;
	theApp->invalidateAll();
	InvalidateDialogRect(&mRect);
	bool aDone = false;
	mMouse.x = theInput->mMouseX;
	mMouse.y = theInput->mMouseY;
	while (true)
	{
		if (theInput->getEvent(&anEvent, true))
		{
			theApp->mEventTime = anEvent.mTime;
			if (!handleEvent(&anEvent))
			{
				aDone = true;
				aResult = CLOSED;
			}
			else
			{
				aResult = hitTest(&anEvent);
				if (aResult != nullptr && anEvent.mType == EV_DOWN &&
					(anEvent.mCode == BUTTON_LEFT || anEvent.mChar == '\r'))
					aDone = onItemClicked(aResult);
				if (!aDone)
					continue;
			}
		}
		theApp->renderDirty(true);
		DrawDialogs(theDisplay);
		theApp->presentDirty();
		theApp->endFrame();
		gDialogDirty.clear();
		if (aDone)
			break;
		if (theTimeout > 0 && theTimeout < PyTimeSub(theApp->mEventTime, aStart) && onTimeout())
		{
			aResult = TIMED_OUT;
			break;
		}
	}
	InvalidateDialogRect(&mRect);
	theApp->invalidateAll();
	if (!gDialogStack.empty())
		gDialogStack.erase(gDialogStack.begin());
	// two moves, so that what is under the mouse lights up again
	anEvent.mType = EV_MOUSE_MOVE;
	anEvent.mX = theInput->mMouseX + 1;
	anEvent.mY = theInput->mMouseY + 1;
	theInput->postEvent(anEvent);
	anEvent.mX = anEvent.mX - 1;
	anEvent.mY = anEvent.mY - 1;
	theInput->postEvent(anEvent);
	return aResult;
}

PGDialogItem* PGDialog::hitTest(const PGEvent* theEvent)
{
	if (theEvent->mType == EV_DOWN && theEvent->mChar == '\r' && mDefault != nullptr)
	{
		if (mDefault->handleEvent(theEvent))
			return mDefault;
	}
	PGDialogItem* anItem = mFocus;
	if (anItem != nullptr)
	{
		do
		{
			if (anItem->handleEvent(theEvent))
				return anItem;
			anItem = anItem->getNext();
			if (anItem == nullptr)
				anItem = mFirstItem;
		} while (anItem != mFocus);
	}
	return nullptr;
}

void PGDialog::draw(HDisplay* theDisplay)
{
	// what the dialogs need redrawn, then what the app does
	HRectList* aLists[2] = {&gDialogDirty, mApp != nullptr ? &mApp->mDirty : nullptr};
	for (HRectList* aList : aLists)
	{
		if (aList == nullptr)
			continue;
		for (int i = 0; i < aList->count(); i++)
		{
			const PGRect* aClip = aList->get(i);
			if (mBackground != nullptr)
			{
				PGRect aDest = mRect;
				PGRect aSrc;
				mBackground->getRect(&aSrc);
				if (ClipToRect(&aDest, &aSrc, aClip))
					mBackground->blit(&aDest, &aSrc);
			}
			for (PGDialogItem* anItem = mFirstItem; anItem != nullptr; anItem = anItem->getNext())
				anItem->draw(theDisplay, aClip);
		}
	}
}

void PGDialog::setPosition(const PGPoint* thePosition)
{
	mPosition = *thePosition;
	updateRect();
}

void PGDialog::setBackground(PGSprite* theBackground)
{
	mBackground = theBackground;
	updateRect();
}

void PGDialog::updateRect()
{
	int aLeft = mPosition.x;
	int aTop = mPosition.y;
	// port: the original leaves right and bottom unset without a background
	int aRight = aLeft;
	int aBottom = aTop;
	if (mBackground != nullptr)
	{
		aRight = mBackground->width() + aLeft;
		aBottom = mBackground->height() + aTop;
	}
	mRect.left = aLeft;
	mRect.top = aTop;
	mRect.right = aRight;
	mRect.bottom = aBottom;
}

void PGDialog::setFocus(PGDialogItem* theItem)
{
	if (theItem != mFocus && mFocus != nullptr)
	{
		const PGRect* aRect = mFocus->getRect();
		PGRect anInvalid = {aRect->left + mRect.left, aRect->top + mRect.top, aRect->right + mRect.left,
			aRect->bottom + mRect.top};
		invalidate(&anInvalid);
	}
	if (theItem == nullptr)
	{
		mFocus = mFirstItem;
		return;
	}
	mFocus = theItem;
}

void PGDialog::invalidate(const PGRect* theRect)
{
	InvalidateDialogRect(theRect);
}

} // namespace pg
