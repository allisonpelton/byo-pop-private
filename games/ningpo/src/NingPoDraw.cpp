// The game's screen (see NingPo.h): render (0x401c80) and what it draws with.
//
// The original clips by hand nearly everywhere it draws, inline, in one of two shapes: a test that the rectangle
// meets the clip (touching counts) followed by the intersection, with the sprite's source rectangle cut by as much
// (ClipBlitRect below, also inline in render), or each edge clamped into the clip (ClampDelta). The helpers here
// are those shapes, once.

#include "HDisplay.h"
#include "NingPo.h"

#include <cstdio>
#include <cstring>

namespace ningpo
{

// The blend mode of the faded sprites (the shadows, the "no match" sign): the source scaled by its pixels' alpha and
// the constant theAlpha (0..255), over the destination scaled by one minus the source's alpha
#define BLEND_CONSTANT_ALPHA(theAlpha) 2, 2, 0, (theAlpha), 2, 2, 1, 0xff, 0

// The test the game repeats inline before drawing into a rectangle: neither is empty and they meet (touching counts)
static bool RectsMeet(const PGRect* theRect, const PGRect* theClip)
{
	if (theRect->right == theRect->left || theRect->bottom == theRect->top || theClip->left == theClip->right ||
		theClip->bottom == theClip->top)
		return false;
	bool aMeetsX = (theClip->left <= theRect->left && theRect->left <= theClip->right) ||
		(theClip->left <= theRect->right && theRect->right <= theClip->right) ||
		(theRect->left <= theClip->left && theClip->right <= theRect->right);
	bool aMeetsY = (theClip->top <= theRect->top && theRect->top <= theClip->bottom) ||
		(theClip->top <= theRect->bottom && theRect->bottom <= theClip->bottom) ||
		(theRect->top <= theClip->top && theClip->bottom <= theRect->bottom);
	return aMeetsX && aMeetsY;
}

// A whole sprite at a point, its part inside theClip copied (ClipBlitRect's shape)
static void BlitSprite(PGSprite* theSprite, int theX, int theY, const PGRect* theClip)
{
	PGRect aRect;
	theSprite->getRect(&aRect);
	PGRect aSrc = aRect;
	aRect.left += theX;
	aRect.right += theX;
	aRect.top += theY;
	aRect.bottom += theY;
	PGRect aDest;
	if (ClipBlitRect(&aRect, theClip, &aDest, &aSrc))
		theSprite->blit(&aDest, &aSrc);
}

// The other shape: each edge of theDest clamped into theClip, theSrc cut by as much (a rectangle outside the clip
// becomes an empty one)
static void ClampToClip(PGRect* theDest, PGRect* theSrc, const PGRect* theClip)
{
	theSrc->left += ClampDelta(&theDest->left, theClip->left, theClip->right);
	theSrc->right += ClampDelta(&theDest->right, theClip->left, theClip->right);
	theSrc->top += ClampDelta(&theDest->top, theClip->top, theClip->bottom);
	theSrc->bottom += ClampDelta(&theDest->bottom, theClip->top, theClip->bottom);
}

// A whole sprite at a point, clamped into theClip (the title screen's previews)
static void BlitSpriteClamped(PGSprite* theSprite, int theX, int theY, const PGRect* theClip)
{
	PGRect aSrc;
	theSprite->getRect(&aSrc);
	PGRect aDest = {aSrc.left + theX, aSrc.top + theY, aSrc.right + theX, aSrc.bottom + theY};
	ClampToClip(&aDest, &aSrc, theClip);
	theSprite->blit(&aDest, &aSrc);
}

// 0x402ad0
bool ClipBlitRect(const PGRect* theRect, const PGRect* theClip, PGRect* theDest, PGRect* theSrc)
{
	if (theClip->right < theRect->left || theRect->right < theClip->left || theClip->bottom < theRect->top ||
		theRect->bottom < theClip->top)
		return false;
	if (theClip->left <= theRect->left && theRect->right <= theClip->right && theClip->top <= theRect->top &&
		theRect->bottom <= theClip->bottom)
	{
		*theDest = *theRect;
		return true;
	}
	if (theRect->left < theClip->left)
	{
		theDest->left = theClip->left;
		theSrc->left += theClip->left - theRect->left;
	}
	else
		theDest->left = theRect->left;
	if (theRect->top < theClip->top)
	{
		theDest->top = theClip->top;
		theSrc->top += theClip->top - theRect->top;
	}
	else
		theDest->top = theRect->top;
	if (theClip->right < theRect->right)
	{
		theDest->right = theClip->right;
		theSrc->right += theClip->right - theRect->right;
	}
	else
		theDest->right = theRect->right;
	if (theClip->bottom < theRect->bottom)
	{
		theDest->bottom = theClip->bottom;
		theSrc->bottom += theClip->bottom - theRect->bottom;
	}
	else
		theDest->bottom = theRect->bottom;
	return true;
}

// 0x402bf0
int ClampDelta(int* theValue, int theLow, int theHigh)
{
	int anOld = *theValue;
	if (anOld < theLow)
		*theValue = theLow;
	else if (theHigh < anOld)
		*theValue = theHigh;
	return *theValue - anOld;
}

// 0x401c80
void NingPo::render(const PGRect* theRect, bool theUnderDialog)
{
	if (mState == STATE_EXIT)
		return;
	if (RectsMeet(&mBoardRect, theRect))
	{
		if (mPaused)
		{
			// the paused screen, over the board, cut to the clip by hand
			PGRect aRect;
			getSprite(SPRITE_PAUSED)->getRect(&aRect);
			aRect.left += 0x7d;
			aRect.right += 0x7d;
			PGRect aDest = *theRect;
			if (aRect.left <= aDest.left)
			{
				if (aRect.right < aDest.left)
					aDest.left = aRect.right;
			}
			else
				aDest.left = aRect.left;
			if (aRect.left <= aDest.right)
			{
				if (aRect.right < aDest.right)
					aDest.right = aRect.right;
			}
			else
				aDest.right = aRect.left;
			ClampDelta(&aDest.top, aRect.top, aRect.bottom);
			ClampDelta(&aDest.bottom, aRect.top, aRect.bottom);
			PGRect aSrc = {aDest.left - 0x7d, aDest.top, aDest.right - 0x7d, aDest.bottom};
			if (aSrc.left < 0)
			{
				aDest.left -= aSrc.left;
				aSrc.left = 0;
			}
			if (aDest.top < 0)
				aSrc.top = 0;
			aDest.top = aSrc.top;
			getSprite(SPRITE_PAUSED)->blit(&aDest, &aSrc);
		}
		else if (mState == STATE_TITLE)
		{
			int aWidth;
			BlitSprite(getSprite(SPRITE_START), 0x78, 0, theRect);
			PGSprite* aSprite = getSprite(mBackground + SPRITE_PREVIEW_BACKGROUND);
			aWidth = aSprite->width();
			BlitSpriteClamped(aSprite, (mBoardRect.right - aWidth - mBoardRect.left) / 2 + 0x7e, 0x27, theRect);
			aSprite = getSprite(mTileset + SPRITE_PREVIEW_TILE);
			aWidth = aSprite->width();
			BlitSpriteClamped(aSprite, (mBoardRect.right - aWidth - mBoardRect.left) / 2 + 0x7d, 0x35, theRect);
			aSprite = getSprite(mLayout + SPRITE_PREVIEW_LAYOUT);
			aWidth = aSprite->width();
			BlitSpriteClamped(aSprite, (mBoardRect.right - aWidth - mBoardRect.left) / 2 + 0x7e, 0xcb, theRect);
			aSprite = getSprite(mLayout + SPRITE_TITLE_LAYOUT);
			aWidth = aSprite->width();
			BlitSpriteClamped(aSprite, (mBoardRect.right - aWidth - mBoardRect.left) / 2 + 0x7e, 0xa0, theRect);
		}
		else if (mState == STATE_WON || mState == STATE_GAME_OVER)
		{
			BlitSprite(&mSprites[SPRITE_BACKGROUND], 0x78, 0, theRect);
			BlitSprite(getSprite(mState == STATE_WON ? SPRITE_BOARD_COMPLETE : SPRITE_GAME_OVER), 0x7d, 0, theRect);
		}
		else
		{
			BlitSprite(&mSprites[SPRITE_BACKGROUND], 0x78, 0, theRect);
			// the hint's tiles pulse, on the system clock: (sin + 1) / 2 of half a degree a millisecond
			PyTime aNow = pg::HTimeGetTime();
			int anElapsed = (int)pg::PyTimeSub(aNow, mHintStart);
			float aPulse = (float)((pg::HSin((float)anElapsed * 0.5f) + 1.0) * 0.5);
			mBoard.draw(theRect, mHintTiles[0], mHintTiles[1], mHintTiles[2], mHintTiles[3], aPulse, mFade);
		}
	}
	BlitSprite(getSprite(SPRITE_LEFT), 0, 0, theRect);
	if (!theUnderDialog && !mPaused)
		drawPanel(theRect);
}

// 0x402c30: the panel's characters
static int DigitSprite(char theChar)
{
	switch (theChar)
	{
	case ',':
		return SPRITE_COMMA;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return SPRITE_DIGIT_0 + (theChar - '0');
	case ':':
		return SPRITE_COLON;
	default:
		return -1;
	}
}

// 0x402d30: right to left from theX (the last character's left edge); with theCommas a comma every three digits,
// a little lower and narrower
void NingPo::drawNumber(const char* theText, int theX, int theY, bool theCommas, const PGRect* theClip)
{
	PGSprite* aZero = &mSprites[SPRITE_DIGIT_0];
	int aWidth = aZero->width();
	int aHeight = aZero->height();
	int i = (int)strlen(theText) - 1;
	int aDigits = 0;
	bool aComma = false;
	do
	{
		if (i < 0)
			return;
		PGSprite* aSprite;
		int aStep;
		int aY;
		if (aComma && theCommas)
		{
			aSprite = getSprite(DigitSprite(','));
			aComma = false;
			if (aSprite != nullptr)
				aWidth = aSprite->width();
			aStep = (aWidth * 3) / 4;
			aY = theY + 3;
		}
		else
		{
			int anIndex = DigitSprite(theText[i]);
			aSprite = anIndex < 0 ? nullptr : getSprite(anIndex);
			aDigits++;
			i--;
			if (aSprite != nullptr)
				aWidth = aSprite->width();
			aStep = aWidth;
			aY = theY;
			if (aDigits % 3 == 0)
			{
				aComma = true;
				aStep = (aWidth * 3) / 4 + 1;
			}
		}
		if (aSprite != nullptr)
		{
			PGRect aRect = {theX, aY, aWidth + theX, aY + aHeight};
			PGRect aSrc;
			aSprite->getRect(&aSrc);
			PGRect aDest;
			if (ClipBlitRect(&aRect, theClip, &aDest, &aSrc))
				aSprite->blitEx(&aDest, &aSrc, BLEND_ALPHA);
		}
		theX = theX - aStep;
	} while (true);
}

// 0x402fd0: the tiles left, the moves and the clock
void NingPo::drawPanel(const PGRect* theClip)
{
	if (mState == STATE_TITLE || mState == STATE_EXIT)
		return;
	char aText[0x80];
	if (RectsMeet(&mTilesLeftRect, theClip))
	{
		pg::HSnprintf(aText, sizeof(aText), "%d", mBoard.mTilesLeft);
		drawNumber(aText, 0x262, 0x1c4, true, theClip);
	}
	if (RectsMeet(&mMovesRect, theClip))
	{
		pg::HSnprintf(aText, sizeof(aText), "%d", mBoard.mMoves);
		drawNumber(aText, 0x1b8, 0x1c4, true, theClip);
	}
	// the seconds played (an unsigned division of the 64-bit time), with the penalties
	uint64_t aPlayed = (uint64_t)(pg::PyTimeSub(mEventTime, mGameStart) - getTime());
	int aSeconds = (int)(uint32_t)(aPlayed / 1000) + mPenalty;
	if (mState == STATE_GAME_OVER || mState == STATE_WON)
		aSeconds = mFinalTime;
	int aMinutes = aSeconds / 0x3c;
	aSeconds = aSeconds % 0x3c;
	if (RectsMeet(&mClockRect, theClip))
	{
		pg::HSnprintf(aText, sizeof(aText), "%02d:%02d", aMinutes, aSeconds);
		drawNumber(aText, 0x10e, 0x1c4, false, theClip);
	}
	if (!isPaused())
		drawNoMatch(theClip);
}

// 0x403410: a tile with its shadows; theAlpha scales the face, theHighlight adds the screen under it (lighting the
// tile), theShadow is the shadows' alpha (none at 0)
void NingPo::drawTile(Tile* theTile, const PGRect* theClip, float theShadow, float theAlpha, float theHighlight)
{
	PGSprite* aSheet = getSprite(SPRITE_TILE_SHEET);
	PGRect aRect;
	theTile->getDirtyRect(&aRect);
	if (!RectsMeet(&aRect, theClip))
		return;
	theTile->getRect(&aRect);
	int aCellX = (theTile->mFace % 0xd) * 0x27;
	int aCellY = (theTile->mFace / 0xd) * 0x37;
	PGRect aSrc = {aCellX, aCellY, aCellX + 0x27, aCellY + 0x37};
	ClampToClip(&aRect, &aSrc, theClip);
	if (theAlpha == 1.0f && theHighlight == 0.0f)
		aSheet->blit(&aRect, &aSrc);
	else
		aSheet->blitEx(&aRect, &aSrc, 2, 2, 0, (int)(theAlpha * 255.0f), 1, 4, 0, (int)(theHighlight * 255.0f), 0);
	if (0.0f < theShadow)
	{
		drawTopShadow(theTile, theClip, theShadow);
		drawRightShadow(theTile, theClip, theShadow);
	}
}

// 0x403940: theDest (and theSrc) cut where a neighbour's face covers it, on the left or the right
static void TrimShadowX(PGRect* theDest, PGRect* theSrc, Tile* theTile)
{
	if (theTile != nullptr)
	{
		PGRect aFace;
		theTile->getFaceRect(&aFace);
		int aLeft = theDest->left;
		if (aLeft <= aFace.right && aFace.left <= aLeft)
		{
			theDest->left = aLeft + (aFace.right - aLeft);
			theSrc->left = theSrc->left + (aFace.right - aLeft);
		}
		if (theDest->left <= aFace.left && aFace.left <= theDest->right)
		{
			int aRight = theDest->right;
			theDest->right = aRight + (aFace.left - aRight);
			theSrc->right = theSrc->right + (aFace.left - aRight);
		}
	}
}

// 0x403c60: the same, at the top or the bottom
static void TrimShadowY(PGRect* theDest, PGRect* theSrc, Tile* theTile)
{
	if (theTile != nullptr)
	{
		PGRect aFace;
		theTile->getFaceRect(&aFace);
		int aTop = theDest->top;
		if (aTop <= aFace.bottom && aFace.top <= aTop)
		{
			theDest->top = aTop + (aFace.bottom - aTop);
			theSrc->top = theSrc->top + (aFace.bottom - aTop);
		}
		if (theDest->top <= aFace.top && aFace.top <= theDest->bottom)
		{
			int aBottom = theDest->bottom;
			theDest->bottom = aBottom + (aFace.top - aBottom);
			theSrc->bottom = theSrc->bottom + (aFace.top - aBottom);
		}
	}
}

// 0x403690: the shadow above the tile's face, when no tile is right above it on its level
void NingPo::drawTopShadow(Tile* theTile, const PGRect* theClip, float theShadow)
{
	Board* aBoard = &mBoard;
	if (aBoard->tileAtCell(theTile->mX, theTile->mY - 2, theTile->mZ, true) != nullptr)
		return;
	PGRect aFace;
	theTile->getFaceRect(&aFace);
	PGSprite* aShadow = &mSprites[SPRITE_SHADOW_TOP];
	PGRect aDest = {aFace.left, aFace.top - aShadow->height(), aShadow->width() + aFace.left, aFace.top};
	PGRect aSrc = {0, 0, aShadow->width(), aShadow->height()};
	Tile* aNeighbour = aBoard->tileAtCell(theTile->mX - 1, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowX(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowX(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 1, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowX(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowX(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY - 1, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowX(&aDest, &aSrc, aNeighbour);
	if (aDest.top < aDest.bottom)
	{
		ClampToClip(&aDest, &aSrc, theClip);
		invalidateRect(&aDest);
		getSprite(SPRITE_SHADOW_TOP)->blitEx(&aDest, &aSrc, BLEND_CONSTANT_ALPHA((int)(theShadow * 255.0f)));
	}
}

// 0x4039a0: the shadow right of the tile's face, when no tile is right of it on its level
void NingPo::drawRightShadow(Tile* theTile, const PGRect* theClip, float theShadow)
{
	Board* aBoard = &mBoard;
	if (aBoard->tileAtCell(theTile->mX + 2, theTile->mY, theTile->mZ, true) != nullptr)
		return;
	PGRect aFace;
	theTile->getFaceRect(&aFace);
	PGSprite* aShadow = &mSprites[SPRITE_SHADOW_RIGHT];
	// it starts where the top shadow does
	int aTop = aFace.top + (1 - mSprites[SPRITE_SHADOW_TOP].height());
	PGRect aDest = {aFace.right, aTop, aShadow->width() + aFace.right, aShadow->height() + aTop};
	PGRect aSrc = {0, 0, aShadow->width(), aShadow->height()};
	Tile* aNeighbour = aBoard->tileAtCell(theTile->mX + 1, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowY(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY - 2, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowY(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY - 1, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowY(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowY(&aDest, &aSrc, aNeighbour);
	aNeighbour = aBoard->tileAtCell(theTile->mX + 2, theTile->mY + 1, theTile->mZ, true);
	if (aNeighbour != nullptr)
		TrimShadowY(&aDest, &aSrc, aNeighbour);
	if (aDest.top < aDest.bottom)
	{
		ClampToClip(&aDest, &aSrc, theClip);
		invalidateRect(&aDest);
		getSprite(SPRITE_SHADOW_RIGHT)->blitEx(&aDest, &aSrc, BLEND_CONSTANT_ALPHA((int)(theShadow * 255.0f)));
	}
}

// 0x404de0: the outline of the tile under the mouse, green when it is free, red when not
void NingPo::drawOutline(Tile* theTile, const PGRect* theClip, bool theFree)
{
	PGSprite* aSprite = getSprite(theFree ? SPRITE_OUTLINE_FREE : SPRITE_OUTLINE_BLOCKED);
	PGRect aDest;
	theTile->getFaceRect(&aDest);
	PGRect aSrc = {0, 0, 0x21, 0x30};
	ClampToClip(&aDest, &aSrc, theClip);
	aSprite->blitEx(&aDest, &aSrc, BLEND_ALPHA);
}

// 0x405690: theSprite at theDest with a constant alpha, inside theClip
void NingPo::drawFaded(const PGRect* theClip, const PGRect* theDest, PGSprite* theSprite, float theAlpha)
{
	PGRect aSrc = {0, 0, theSprite->width(), theSprite->height()};
	if (!RectsMeet(theDest, theClip))
		return;
	PGRect aDest = *theDest;
	ClampToClip(&aDest, &aSrc, theClip);
	if (0.0f < theAlpha)
		theSprite->blitEx(&aDest, &aSrc, BLEND_CONSTANT_ALPHA((int)(theAlpha * 255.0f)));
}

// 0x405890: the "no match" sign, shown at once and faded out over its last 1.5 of 3 seconds
void NingPo::drawNoMatch(const PGRect* theClip)
{
	int64_t anElapsed = pg::PyTimeSub(mEventTime, mNoMatchTime);
	if (0 <= anElapsed && mNoMatchTime != 0)
	{
		float aFade;
		if (0x5db < anElapsed)
			aFade = (float)(anElapsed - 0x5dc) * 0.00066666666f;
		else
			aFade = 0.0f;
		aFade = 1.0f - aFade;
		drawFaded(theClip, &mNoMatchRect, getSprite(SPRITE_NOMATCH), aFade);
	}
}

} // namespace ningpo
