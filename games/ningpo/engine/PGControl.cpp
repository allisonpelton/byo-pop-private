// PGControl (see PGControl.h).

#include "PGControl.h"
#include "HDisplay.h"
#include "PGApp.h"
#include "PGSprite.h"

namespace pg
{

bool ClipToRect(PGRect* theRect, PGRect* theSrc, const PGRect* theClip)
{
	if (theClip->right < theRect->left || theRect->right < theClip->left || theClip->bottom < theRect->top ||
		theRect->bottom < theClip->top)
		return false;
	if (theRect->left < theClip->left)
	{
		theSrc->left += theClip->left - theRect->left;
		theRect->left = theClip->left;
	}
	if (theRect->top < theClip->top)
	{
		theSrc->top += theClip->top - theRect->top;
		theRect->top = theClip->top;
	}
	if (theClip->right < theRect->right)
	{
		theSrc->right += theClip->right - theRect->right;
		theRect->right = theClip->right;
	}
	if (theClip->bottom < theRect->bottom)
	{
		theSrc->bottom += theClip->bottom - theRect->bottom;
		theRect->bottom = theClip->bottom;
	}
	return true;
}

PGControl::PGControl()
{
	mRefCount = 0;
	mOwner = nullptr;
	mLit = nullptr;
	mNormal = nullptr;
	mDisabled = nullptr;
	mHighlighted = false;
	mShownUnderDialogs = false;
	mEnabled = true;
	mRect.left = -1000;
	mRect.top = 0;
	mRect.right = 0;
	mRect.bottom = -1000;
}

void PGControl::setHighlight(bool theHighlighted)
{
	if (mHighlighted != theHighlighted)
	{
		mHighlighted = theHighlighted;
		if (mOwner != nullptr)
			mOwner->invalidateRect(&mRect);
	}
}

// 0x4110a0
void PGControl::draw(const PGRect* theClip)
{
	PGSprite* aSprite = mNormal;
	if (aSprite == nullptr && (aSprite = mLit) == nullptr)
	{
		// the original noted it once (0x4c1d50)
		return;
	}
	PGRect aSrc;
	aSprite->getRect(&aSrc);
	PGRect aDest = mRect;
	if (!ClipToRect(&aDest, &aSrc, theClip))
		return;
	if (!isEnabled())
	{
		if (mDisabled != nullptr)
			mDisabled->blit(&aDest, &aSrc);
	}
	else
	{
		aSprite = isHighlighted() ? mLit : mNormal;
		if (aSprite != nullptr)
			aSprite->blitEx(&aDest, &aSrc, BLEND_ALPHA);
	}
}

// 0x411240
bool PGControl::handleEvent(const PGEvent* theEvent)
{
	if (theEvent->mType == EV_MOUSE_MOVE)
	{
		const PGRect* aRect = getRect();
		if (aRect->left <= theEvent->mX && theEvent->mX <= aRect->right && aRect->top <= theEvent->mY &&
			theEvent->mY <= aRect->bottom)
		{
			setHighlight(true);
			return false;
		}
		setHighlight(false);
	}
	return false;
}

void PGControl::setSprites(PGSprite* theNormal, PGSprite* theLit, PGSprite* theDisabled)
{
	if (mNormal != theNormal || mLit != theLit || mDisabled != theDisabled)
	{
		mNormal = theNormal;
		mLit = theLit;
		mDisabled = theDisabled;
		updateRect();
	}
}

void PGControl::setPosition(const PGPoint* thePosition)
{
	mRect.left = thePosition->x;
	mRect.top = thePosition->y;
	updateRect();
}

void PGControl::updateRect()
{
	int aWidth = 0;
	int aHeight = 0;
	if (mNormal != nullptr)
	{
		aWidth = mNormal->width();
		aHeight = mNormal->height();
	}
	if (mLit != nullptr)
	{
		aWidth = mLit->width();
		aHeight = mLit->height();
	}
	if (mDisabled != nullptr)
	{
		aWidth = mDisabled->width();
		aHeight = mDisabled->height();
	}
	// the original compares left and bottom with -10000 (the constructor sets -1000), so this always happens
	if (mRect.left != -10000)
		mRect.right = mRect.left + aWidth;
	if (mRect.bottom != -10000)
		mRect.bottom = mRect.top + aHeight;
	if (mOwner != nullptr)
		mOwner->invalidateRect(&mRect);
}

bool PGControl::isClick(const PGEvent* theEvent)
{
	if (theEvent->mType != EV_DOWN || theEvent->mCode != BUTTON_LEFT)
		return false;
	const PGRect* aRect = getRect();
	return aRect->left <= theEvent->mX && theEvent->mX <= aRect->right && aRect->top <= theEvent->mY &&
		theEvent->mY <= aRect->bottom;
}

} // namespace pg
