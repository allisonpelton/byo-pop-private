#include "WordBalloon.h"
#include "BookWormApp.h"
#include "Board.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"

using namespace Sexy;

// Widget::SetColors table (r, g, b, a): 0 balloon tint, 1 header, 2 text, 3 footer
static int gWordBalloonColors[4][4] = {{255,255,255,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}};  // 0x52f8f0

// The balloon's fade applied to one of its colours: alpha (int)(colour alpha * clamp(mAlpha, 0, 1)).
static Color FadedColor(const Color& theColor, double theAlpha)
{
	if (theAlpha < 0.0)
		theAlpha = 0.0;
	else if (theAlpha > 1.0)
		theAlpha = 1.0;
	Color aColor = theColor;
	aColor.mAlpha = (int)((double)aColor.mAlpha * theAlpha);
	return aColor;
}

// 0x4372a0
WordBalloon::WordBalloon(BookWormApp* theApp, Image* theBoxImage, Image* theTailImage, Point theTailPos)
{
	// mHeader, mText, mFooter empty; mImageInsets, mTextInsets zero (Insets())
	mTailPos = Point(0, 0);
	mPlainImage = NULL; // port: the original tests it in SetBoxImages before anything sets it (lex.md 11.7)
	Init(theApp);
	SetBoxImages(theBoxImage, theTailImage, theTailPos);
}

// 0x4373a0
WordBalloon::~WordBalloon()
{
}

// 0x436260
// Starts hidden and disabled, white tint and black texts, left-aligned blocks, fade step 0.1.
void WordBalloon::Init(BookWormApp* theApp)
{
	mApp = theApp;
	mHasAlpha = true;
	mDoFinger = true;
	mBoxImage = NULL;
	mTailImage = NULL;
	mHeaderFont = NULL;
	mTextFont = NULL;
	mFooterFont = NULL;
	mHeaderJustify = -1;
	mTextJustify = -1;
	mFooterJustify = -1;
	mAlpha = 0.0;
	mTargetAlpha = 0.0;
	mFadeSpeed = 0.1;
	mTimer = 0;
	mClicked = false;
	SetColors(gWordBalloonColors, 4);
	SetVisible(false);
	SetDisabled(true);
}

// 0x436310
// Note the argument order: text, header, footer (only Lex::Update calls it, with 0, 0, 0).
void WordBalloon::SetJustification(int theTextJustify, int theHeaderJustify, int theFooterJustify)
{
	mTextJustify = theTextJustify;
	mHeaderJustify = theHeaderJustify;
	mFooterJustify = theFooterJustify;
}

// 0x436360 slot 49
// Fades in and stays theDuration updates (-1: until Show(0) or a click); Show(0) fades out and hides.
void WordBalloon::Show(int theDuration)
{
	SetVisible(true);
	SetDisabled(false);
	mTimer = theDuration;
	mClicked = false;
	mTargetAlpha = (theDuration != 0) ? 1.0 : 0.0;
}

// 0x4363d0 slot 16
// Only while a board with a Lex exists.
void WordBalloon::Draw(Graphics* g)
{
	Board* aBoard = mApp->mBoard;
	if ((aBoard != NULL) && (aBoard->mLex != NULL) && (mAlpha > 0.0))
	{
		if (mPlainImage != NULL)
			DrawPlain(g);
		else
			DrawBox(g);
	}
}

// 0x436420 slot 27
// A click ends the message: one more update, then the fade-out (no Widget::MouseDown call).
void WordBalloon::MouseDown(int x, int y, int theClickCount)
{
	Close();
	if ((!mLookupURL.empty()) && (theClickCount > 0))
		mApp->OpenURL(mLookupURL);  // port(mod): the word in an online dictionary (a left click; a right one only closes)
}

// port(mod): ends the message as a click on it does (the click's body; mod "close-balloons", Board::MouseDown)
void WordBalloon::Close()
{
	mTimer = 1;
	mClicked = true;
}

// 0x436440
// The thought balloon: one image drawn whole, text inside theInsets; resized to the image at its position.
void WordBalloon::SetPlainImage(Image* theImage, Insets theInsets, bool doResize)
{
	if ((mBoxImage != NULL) || (mTailImage != NULL))
		SetBoxImages(NULL, NULL, Point(0, 0));
	mImageInsets = theInsets;
	mPlainImage = theImage;
	if (doResize && (theImage != NULL))
		Resize(mX, mY, mPlainImage->GetWidth(), theImage->GetHeight());
}

// 0x4364d0
// The speech balloon: a 9-slice box with a tail below it. The text area skips a third of the box image
// at the top and the bottom (9 and 9 + 20 pixels with backdrop_wb_text and the talk tail).
void WordBalloon::SetBoxImages(Image* theBoxImage, Image* theTailImage, Point theTailPos)
{
	if (mPlainImage != NULL)
		SetPlainImage(NULL, Insets(0, 0, 0, 0), false);
	mBoxImage = theBoxImage;
	mTailImage = theTailImage;
	mTailPos = theTailPos;
	mImageInsets = Insets(0, 0, 0, 0);
	if (mBoxImage != NULL)
	{
		mImageInsets.mTop += (int)(((double)mBoxImage->GetHeight() + 0.5) * 0.33);
		mImageInsets.mBottom += (int)(((double)mBoxImage->GetHeight() + 0.5) * 0.33);
	}
	if (mTailImage != NULL)
		mImageInsets.mBottom += mTailImage->GetHeight();
}

// 0x4365d0 slot 50
// Height of a word-wrapped block (0 for no text or no font).
int WordBalloon::GetTextHeight(int theWidth, const SexyString& theText, Font* theFont, int theJustify,
	int theLineSpacing)
{
	if ((theText.length() == 0) || (theFont == NULL))
		return 0;

	Graphics aGraphics;
	aGraphics.SetFont(theFont);
	if (theLineSpacing == -1)
		theLineSpacing = theFont->GetLineSpacing();
	// port: the original runs the 2004 WriteWordWrapped (Widget slot 37) with theJustify into a Graphics
	// without an image; the justification does not change the height, so this measures without drawing.
	(void)theJustify;
	return GetWordWrappedHeight2004(&aGraphics, theWidth, theText, theLineSpacing);
}

// 0x436680 slot 51
// Text blocks (those with text and a font) plus both insets; 0 for a width <= 0.
int WordBalloon::GetPreferredHeight(int theWidth)
{
	int aHeight = 0;
	if (theWidth > 0)
	{
		int anInnerWidth = theWidth - (mTextInsets.mRight + mTextInsets.mLeft + mImageInsets.mRight + mImageInsets.mLeft);
		if ((mHeader.length() != 0) && (mHeaderFont != NULL))
			aHeight = GetTextHeight(anInnerWidth, mHeader, mHeaderFont, mHeaderJustify, mHeaderFont->GetLineSpacing());
		if ((mText.length() != 0) && (mTextFont != NULL))
			aHeight += GetTextHeight(anInnerWidth, mText, mTextFont, mTextJustify, mTextFont->GetLineSpacing());
		if ((mFooter.length() != 0) && (mFooterFont != NULL))
			aHeight += GetTextHeight(anInnerWidth, mFooter, mFooterFont, mFooterJustify, mFooterFont->GetLineSpacing());
		aHeight += mTextInsets.mBottom + mTextInsets.mTop + mImageInsets.mBottom + mImageInsets.mTop;
	}
	return aHeight;
}

// port: the block loop of DrawPlain (0x4367a0) and DrawBox (0x436c40). Header, text, footer in that
// order, each with its colour (1, 2, 3) faded, its font and its justification, one under the other.
void WordBalloon::DrawTextBlocks(Graphics* g, Rect theRect)
{
	if ((mHeader.length() != 0) && (mHeaderFont != NULL))
	{
		g->SetColor(FadedColor(GetColor(1), mAlpha));
		g->SetFont(mHeaderFont);
		theRect.mY += WriteWordWrapped2004(g, theRect, mHeader, mHeaderFont->GetLineSpacing(), mHeaderJustify);
	}
	if ((mText.length() != 0) && (mTextFont != NULL))
	{
		g->SetColor(FadedColor(GetColor(2), mAlpha));
		g->SetFont(mTextFont);
		theRect.mY += WriteWordWrapped2004(g, theRect, mText, mTextFont->GetLineSpacing(), mTextJustify);
	}
	if ((mFooter.length() != 0) && (mFooterFont != NULL))
	{
		g->SetColor(FadedColor(GetColor(3), mAlpha));
		g->SetFont(mFooterFont);
		theRect.mY += WriteWordWrapped2004(g, theRect, mFooter, mFooterFont->GetLineSpacing(), mFooterJustify);
	}
}

// 0x4367a0 slot 47
// The plain image tinted with colour 0, the text centred vertically in the area inside the insets. The
// measuring pass checks only the strings (a block with text but no font would crash; never happens).
void WordBalloon::DrawPlain(Graphics* g)
{
	g->SetColorizeImages(true);
	g->SetColor(FadedColor(GetColor(0), mAlpha));
	g->DrawImage(mPlainImage, 0, 0);

	int aX = mImageInsets.mLeft + mTextInsets.mLeft;
	int aY = mImageInsets.mTop + mTextInsets.mTop;
	int aWidth = mWidth - mTextInsets.mRight - mImageInsets.mRight - mImageInsets.mLeft - mTextInsets.mLeft;
	int aHeight = mHeight - mTextInsets.mBottom - mImageInsets.mBottom - mImageInsets.mTop - mTextInsets.mTop;

	int aTextHeight = 0;
	if (mHeader.length() != 0)
		aTextHeight = GetTextHeight(aWidth, mHeader, mHeaderFont, mHeaderJustify, mHeaderFont->GetLineSpacing());
	if (mText.length() != 0)
		aTextHeight += GetTextHeight(aWidth, mText, mTextFont, mTextJustify, mTextFont->GetLineSpacing());
	if (mFooter.length() != 0)
		aTextHeight += GetTextHeight(aWidth, mFooter, mFooterFont, mFooterJustify, mFooterFont->GetLineSpacing());

	DrawTextBlocks(g, Rect(aX, (aHeight - aTextHeight) / 2 + aY, aWidth, aTextHeight));
	g->SetColorizeImages(false);
}

// 0x436c40 slot 48
// The box tiled from the 9-slice image over the widget minus the tail's height, the left half of the
// tail image under it at mTailPos (tinted with colour 0 only when a box was drawn), the text from the top.
void WordBalloon::DrawBox(Graphics* g)
{
	g->SetColorizeImages(true);
	Color aBackColor = FadedColor(GetColor(0), mAlpha);
	int aBoxHeight = mHeight;
	if (mTailImage != NULL)
		aBoxHeight -= mTailImage->GetHeight();
	if (mBoxImage != NULL)
	{
		g->SetColor(aBackColor);
		g->DrawImageBox(Rect(0, 0, mWidth, aBoxHeight), mBoxImage);
	}
	if (mTailImage != NULL)
		g->DrawImage(mTailImage, mTailPos.mX, mTailPos.mY + aBoxHeight,
			Rect(0, 0, mTailImage->GetWidth() / 2, mTailImage->GetHeight()));

	int aX = mImageInsets.mLeft + mTextInsets.mLeft;
	int aY = mImageInsets.mTop + mTextInsets.mTop;
	int aWidth = mWidth - mTextInsets.mRight - mImageInsets.mRight - mImageInsets.mLeft - mTextInsets.mLeft;
	int aHeight = mHeight - mTextInsets.mBottom - mImageInsets.mBottom - mImageInsets.mTop - mTextInsets.mTop;
	DrawTextBlocks(g, Rect(aX, aY, aWidth, aHeight));
	g->SetColorizeImages(false);
}

// 0x437080 slot 17
// The countdown (stopped while the level summary is up or the board is paused; held at 50 while the
// mouse rests on the balloon and it was not clicked), the fade by mFadeSpeed per update (out only once the
// countdown is 0), and hiding once fully faded. Nothing while disabled or without a board with a Lex.
void WordBalloon::Update()
{
	Widget::Update();
	if (mDisabled)
		return;
	Board* aBoard = mApp->mBoard;
	if ((aBoard == NULL) || (aBoard->mLex == NULL))
		return;

	if ((mTimer > 0) && (!aBoard->mSummaryShown) && (aBoard->mPauseCount <= 0))
	{
		mTimer = (mTimer - 1 < 0) ? 0 : mTimer - 1;
		if (mIsOver && !mClicked)
		{
			if (mTimer < 50)
				mTimer = 50;
		}
		if (mTimer == 0)
			mTargetAlpha = 0.0;
	}

	if (mTargetAlpha > mAlpha)
	{
		int aSign = (mTargetAlpha - mAlpha >= 0.0) ? 1 : -1;
		double anAlpha = (double)aSign * mFadeSpeed + mAlpha;
		if (mTargetAlpha <= anAlpha)
			mAlpha = mTargetAlpha;
		else
			mAlpha = anAlpha;
		MarkDirty();
	}
	else if ((mTargetAlpha < mAlpha) && (mTimer == 0))
	{
		int aSign = (mTargetAlpha - mAlpha >= 0.0) ? 1 : -1;
		double anAlpha = (double)aSign * mFadeSpeed + mAlpha;
		if (anAlpha <= mTargetAlpha)
			mAlpha = mTargetAlpha;
		else
			mAlpha = anAlpha;
		MarkDirty();
	}

	if (mVisible && (mTargetAlpha == 0.0) && (mAlpha == 0.0) && (mTimer == 0))
	{
		SetDisabled(true);
		SetVisible(false);
	}
}

// 0x437430
void WordBalloon::ClearHeader()
{
	mHeader.erase(mHeader.begin(), mHeader.end());
	mHeaderFont = NULL;
}

// 0x437480
void WordBalloon::ClearFooter()
{
	mFooter.erase(mFooter.begin(), mFooter.end());
	mFooterFont = NULL;
}

// port(mod): the original only ever clears the third block; Lex::SayReaction puts the definition there
void WordBalloon::SetFooter(const SexyString& theText, Font* theFont, int theJustify)
{
	mFooter = theText;
	mFooterFont = theFont;
	mFooterJustify = theJustify;
}

// 0x4374e0
void WordBalloon::SetHeader(const SexyString& theText, Font* theFont, int theJustify)
{
	mHeader = theText;
	mHeaderFont = theFont;
	mHeaderJustify = theJustify;
}

// 0x437510
void WordBalloon::SetText(const SexyString& theText, Font* theFont, int theJustify)
{
	mText = theText;
	mTextFont = theFont;
	mTextJustify = theJustify;
	// port(mod): every message sets its text; only a definition is a link (Lex::SayDefinition sets it after)
	mLookupURL.clear();
	mDoFinger = false;
}
