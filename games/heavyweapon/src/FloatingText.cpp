#include "FloatingText.h"
#include "Board.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x42c000
FloatingText::FloatingText(Board* theBoard, const std::string& theText, int x, int y, bool isRainbow,
						   const Color& theColor)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mText = theText;
	mY = y;
	mAlpha = 255;
	mIsRainbow = isRainbow;
	mHue = 0;

	mWidth = mApp->mFonts[FONT_KEYPUNCH16]->StringWidth(theText);
	mX = x - mWidth / 2;
	if (mX + mWidth > 320)
		mX = 320 - mWidth;
	if (mX < -320)
		mX = -320;

	if (theColor == Color(0))
		mColor = Color(255, 255, 255);
	else
		mColor = theColor;

	theBoard->mFloatingTexts.Append(this);
}

// 0x42bf90
void FloatingText::Draw(Graphics* g)
{
	g->SetFont(mApp->mFonts[FONT_KEYPUNCH16]);
	g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, mAlpha));
	g->DrawString(mText, mX, mAlpha / 2 - 128 + mY);
}
