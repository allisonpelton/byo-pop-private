#include "Compat2004.h"
#include "DialogButton.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "Image.h"
#include "Graphics.h"
#include "Font.h"

using namespace Sexy;

// MSVC 7.0 CRT rand.c (_holdrand of the main thread)
static unsigned int gCrtHoldRand = 1;

void Sexy::CrtSRand(unsigned int theSeed)
{
	gCrtHoldRand = theSeed;
}

unsigned int Sexy::CrtRandState()
{
	return gCrtHoldRand;
}

int Sexy::CrtRand()
{
	gCrtHoldRand = gCrtHoldRand * 214013u + 2531011u;
	return (int) ((gCrtHoldRand >> 16) & 0x7fff);
}

void Sexy::SeedMTRand2004(MTRand& theRand, unsigned long theSeed)
{
	if (theSeed == 0)
		theSeed = 4357;

	uint32_t aState[MTRAND_N];
	aState[0] = (uint32_t) theSeed;
	for (int i = 1; i < MTRAND_N; i++)
		aState[i] = aState[i - 1] * 69069u;
	theRand.SetState(aState, MTRAND_N);
}

// 0x43f3b0 (the 2004 Dialog ctor's button part): the port's Dialog ctor made the buttons with ids
// ID_YES/ID_NO and GetString labels; give them the 2004 ids and the constant labels.
Dialog2004::Dialog2004(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
	const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter,
	int theButtonMode) :
	Dialog(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines,
		theDialogFooter, theButtonMode)
{
	if (mYesButton != NULL)
		mYesButton->mId = mId + ID_YES_2004;
	if (mNoButton != NULL)
		mNoButton->mId = mId + ID_NO_2004;
	if (theButtonMode == BUTTONS_YES_NO)
	{
		mYesButton->mLabel = DIALOG_YES_STRING;
		mNoButton->mLabel = DIALOG_NO_STRING;
	}
	else if (theButtonMode == BUTTONS_OK_CANCEL)
	{
		mYesButton->mLabel = DIALOG_OK_STRING;
		mNoButton->mLabel = DIALOG_CANCEL_STRING;
	}
}

// 0x4386e0
void Dialog2004::ButtonPress(int theId)
{
	if ((theId == mId + ID_YES_2004) || (theId == mId + ID_NO_2004))
		mWidgetManager->mApp->ButtonPress(theId);
}

// 0x438720
void Dialog2004::ButtonDepress(int theId)
{
	if ((theId == mId + ID_YES_2004) || (theId == mId + ID_NO_2004))
		mWidgetManager->mApp->ButtonDepress(theId);
}

// 0x438760
void Dialog2004::ButtonDownTick(int theId)
{
	if ((theId == mId + ID_YES_2004) || (theId == mId + ID_NO_2004))
		mWidgetManager->mApp->ButtonDownTick(theId);
}

// 0x43d950. The 2004 Draw creates SysFont fallbacks when no header/lines font is set; the port has no
// SysFont, so a game must set both (BookWorm always does).
void Dialog2004::Draw(Graphics* g)
{
	g->DrawImageBox(Rect(mBackgroundInsets.mLeft, mBackgroundInsets.mTop,
		mWidth - mBackgroundInsets.mLeft - mBackgroundInsets.mRight,
		mHeight - mBackgroundInsets.mTop - mBackgroundInsets.mBottom), mComponentImage);

	int aCurY = mContentInsets.mTop + mBackgroundInsets.mTop;
	if (mDialogHeader.length() > 0)
	{
		aCurY += mHeaderFont->GetAscent() - mHeaderFont->GetAscentPadding();
		g->SetFont(mHeaderFont);
		g->SetColor(mColors[COLOR_HEADER]);
		WriteCenteredLine(g, aCurY, mDialogHeader);
		aCurY += mHeaderFont->GetHeight() - mHeaderFont->GetAscent() + mSpaceAfterHeader;
	}

	g->SetFont(mLinesFont);
	g->SetColor(mColors[COLOR_LINES]);
	Rect aRect(mContentInsets.mLeft + 2 + mBackgroundInsets.mLeft, aCurY,
		mWidth - mContentInsets.mRight - mContentInsets.mLeft - mBackgroundInsets.mLeft - mBackgroundInsets.mRight - 4, 0);
	int aLinesHeight = WriteWordWrapped2004(g, aRect, mDialogLines, mLinesFont->GetLineSpacing() + mLineSpacingOffset,
		mTextAlign);

	if ((mDialogFooter.length() != 0) && (mButtonMode != BUTTONS_FOOTER))
	{
		int aY = aCurY + aLinesHeight + 8 + mHeaderFont->GetLineSpacing();
		g->SetFont(mHeaderFont);
		g->SetColor(mColors[COLOR_FOOTER]);
		WriteCenteredLine(g, aY, mDialogFooter);
	}
}

// 0x438ec0 (the button height is the button image's: the port's mButtonHeight)
int Dialog2004::GetPreferredHeight(int theWidth)
{
	int aHeight = mContentInsets.mTop + mContentInsets.mBottom + mBackgroundInsets.mTop + mBackgroundInsets.mBottom;
	bool needSpace = mDialogHeader.length() > 0;
	if (needSpace)
		aHeight += mHeaderFont->GetHeight() - mHeaderFont->GetAscentPadding();
	if (mDialogLines.length() > 0)
	{
		if (needSpace)
			aHeight += mSpaceAfterHeader;
		Graphics aGraphics;
		aGraphics.SetFont(mLinesFont);
		aHeight += GetWordWrappedHeight2004(&aGraphics,
			theWidth - mContentInsets.mRight - mContentInsets.mLeft - mBackgroundInsets.mRight - mBackgroundInsets.mLeft - 4,
			mDialogLines, mLinesFont->GetLineSpacing() + mLineSpacingOffset);
		needSpace = true;
	}
	if ((mDialogFooter.length() != 0) && (mButtonMode != BUTTONS_FOOTER))
	{
		if (needSpace)
			aHeight += 8;
		aHeight += mHeaderFont->GetLineSpacing();
		needSpace = true;
	}
	if (mYesButton != NULL)
	{
		if (needSpace)
			aHeight += 8;
		aHeight += 8 + mButtonHeight;
	}
	return aHeight;
}

Slider2004::Slider2004(Image* theTrackImage, Image* theThumbImage, int theId, SliderListener* theListener) :
	Slider(theTrackImage, theThumbImage, theId, theListener)
{
}

// 0x437b80
void Slider2004::SetValue(double theValue)
{
	mVal = theValue;
	MarkDirtyFull();
}

// 0x437ba0: only a press on the thumb starts a drag
void Slider2004::MouseDown(int x, int y, int theClickCount)
{
	int aThumbX = (int) (mVal * (mWidth - mThumbImage->GetWidth()));

	if ((x >= aThumbX) && (x < aThumbX + mThumbImage->GetWidth()))
	{
		mWidgetManager->mApp->SetCursor(CURSOR_DRAGGING);
		mDragging = true;
		mRelX = x - aThumbX;
	}
}

DWORD Sexy::GameTickCount()
{
	return (gSexyAppBase != NULL) ? gSexyAppBase->GetClockTime() : GetTickCount();
}

DWORD Sexy::GetLastMouseDownTick(WidgetManager* theWidgetManager)
{
	return (theWidgetManager != NULL) ? theWidgetManager->mLastMouseDownTick : 0;
}

// 0x43b420
static int WriteString2004Impl(Graphics* g, const SexyString& theString, int theX, int theY, int theWidth,
	int theJustification, bool drawString, int theOffset, int theLength)
{
	Font* aFont = g->GetFont();
	if (drawString)
	{
		if (theJustification == 0)
			theX += (theWidth - WriteString2004Impl(g, theString, theX, theY, theWidth, -1, false, theOffset, theLength)) / 2;
		else if (theJustification == 1)
			theX += theWidth - WriteString2004Impl(g, theString, theX, theY, theWidth, -1, false, theOffset, theLength);
	}

	int anEnd = (int) theString.length();
	if ((theLength >= 0) && (theOffset + theLength <= anEnd))
		anEnd = theOffset + theLength;

	SexyString aString;
	int aXOffset = 0;
	for (; theOffset < anEnd; theOffset++)
	{
		if (theString[theOffset] == '^')  // the 2004 Widget::mWriteColoredString: on (BookWorm never turns it off)
		{
			if (theOffset > anEnd - 8)
				break;
			if (theString[theOffset + 1] == '^')
			{
				aString += '^';
				theOffset++;
			}
			else
			{
				unsigned int aColor = 0;
				for (int i = 0; i < 6; i++)
				{
					char c = theString[theOffset + 1 + i];
					int aDigit = 0;
					if ((c >= '0') && (c <= '9'))
						aDigit = c - '0';
					else if ((c >= 'A') && (c <= 'F'))
						aDigit = c - 'A' + 10;
					else if ((c >= 'a') && (c <= 'f'))
						aDigit = c - 'a' + 10;
					aColor += aDigit << (20 - 4 * i);
				}
				if (drawString)
				{
					g->DrawString(aString, theX + aXOffset, theY);
					g->SetColor(Color((aColor >> 16) & 0xFF, (aColor >> 8) & 0xFF, aColor & 0xFF, g->GetColor().mAlpha));
				}
				theOffset += 7;
				aXOffset += aFont->StringWidth(aString);
				aString = "";
			}
		}
		else
		{
			aString += theString[theOffset];
		}
	}
	if (drawString)
		g->DrawString(aString, theX + aXOffset, theY);
	return aXOffset + aFont->StringWidth(aString);
}

int Sexy::WriteString2004(Graphics* g, const SexyString& theString, int theX, int theY, int theWidth,
	int theJustification, bool drawString, int theOffset, int theLength)
{
	return WriteString2004Impl(g, theString, theX, theY, theWidth, theJustification, drawString, theOffset, theLength);
}

// 0x439f70. drawString false only measures (GetWordWrappedHeight2004; the original draws into a
// Graphics on no image, which gives the same height).
static int WriteWordWrapped2004Impl(Graphics* g, const Rect& theRect, const SexyString& theLine, int theLineSpacing,
	int theJustification, bool drawString)
{
	Color anOrigColor = g->GetColor();
	Font* aFont = g->GetFont();
	int aYOffset = aFont->GetAscent() - aFont->GetAscentPadding();
	if (theLineSpacing == -1)
		theLineSpacing = aFont->GetLineSpacing();

	int aLength = (int) theLine.length();
	int aCurPos = 0;
	int aLineStartPos = 0;
	int aCurWidth = 0;
	int aSpacePos = -1;
	SexyChar aPrevChar = 0;
	while (aCurPos < aLength)
	{
		SexyChar aChar = theLine[aCurPos];
		int aNextPos = aCurPos;
		if ((aChar == '^') && (aCurPos + 1 < aLength) && (theLine[aCurPos + 1] != '^'))
		{
			aCurPos += 8;  // a colour code: no width
			continue;
		}
		if (aChar == '^')
		{
			// "^^" (or a '^' at the end): measured as one '^', both characters consumed
			if (aCurPos + 1 < aLength)
				aNextPos = aCurPos + 1;
		}
		else if (aChar == ' ')
		{
			aSpacePos = aCurPos;
		}
		else if (aChar == '\n')
		{
			aCurWidth = theRect.mWidth + 1;  // forces the wrap here
			aNextPos = aCurPos + 1;
			aSpacePos = aCurPos;
		}

		aCurWidth += aFont->CharWidthKern(aChar, aPrevChar);
		aPrevChar = aChar;
		if (aCurWidth > theRect.mWidth)
		{
			if (aSpacePos == -1)
			{
				if (aNextPos < aLineStartPos + 1)
					aNextPos++;  // at least one character per line
				WriteString2004Impl(g, theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, theJustification,
					drawString, aLineStartPos, aNextPos - aLineStartPos);
			}
			else
			{
				WriteString2004Impl(g, theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, theJustification,
					drawString, aLineStartPos, aSpacePos - aLineStartPos);
				aNextPos = aSpacePos + 1;  // 2004: further spaces are not skipped
			}
			aYOffset += theLineSpacing;
			aSpacePos = -1;
			aCurWidth = 0;
			aPrevChar = 0;
			aLineStartPos = aNextPos;
		}
		else
		{
			aNextPos++;
		}
		aCurPos = aNextPos;
	}

	if (aLineStartPos < aLength)
	{
		WriteString2004Impl(g, theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, theJustification,
			drawString, aLineStartPos, aLength - aLineStartPos);
		aYOffset += theLineSpacing;
	}
	g->SetColor(anOrigColor);
	return aFont->GetDescent() + aYOffset - theLineSpacing;
}

int Sexy::WriteWordWrapped2004(Graphics* g, const Rect& theRect, const SexyString& theLine, int theLineSpacing,
	int theJustification)
{
	return WriteWordWrapped2004Impl(g, theRect, theLine, theLineSpacing, theJustification, true);
}

// 0x4389f0
int Sexy::GetWordWrappedHeight2004(Graphics* g, int theWidth, const SexyString& theLine, int theLineSpacing)
{
	Graphics aGraphics;
	aGraphics.SetFont(g->GetFont());
	return WriteWordWrapped2004Impl(&aGraphics, Rect(0, 0, theWidth, 0), theLine, theLineSpacing, -1, false);
}
