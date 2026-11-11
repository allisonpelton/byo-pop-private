// BoardOverlay: the widget over the field (the Board keeps it at +0x660): it fades the field to a colour (the white of a warp,
// the dark of the end of a game), darkens it for the pause, and draws the panels that stop the play: a tutorial hint, the
// weapon's own hint and the warning of an emergency (a ship about to crash).

#include "Board.h"
#include "AstroPopApp.h"
#include "Checkbox.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "Widget.h"

using namespace Sexy;

// 0x432930: the colour the overlay shows comes up to the colour it is told to fill with, and the widget is redrawn. (The
// original's test needs all four parts of the two colours to differ, not just one of them.)
void BoardOverlay::Update()
{
	Widget::Update();
	if (mColor58.mRed != mFill.mRed && mColor58.mGreen != mFill.mGreen && mColor58.mBlue != mFill.mBlue &&
		mColor58.mAlpha != mFill.mAlpha && mBoard->m1924 < 1)
	{
		mColor58 = mFill;
		MarkDirty();
	}
}

// 0x43fc10
void BoardOverlay::Draw(Graphics* g)
{
	Board* aBoard = mBoard;

	// a hint of the tutorial is showing: the first of the Board's hints is lit and the pause timer has run out
	bool aHintShown = !aBoard->mHintSpots.empty() && aBoard->mHintSpots[0]->mActive && !(-aBoard->mLevelUpPanelHeight < aBoard->mLevelUpPanelY);

	if (aBoard->m1924 < 1 && !aHintShown && !mFlag78)
	{
		g->SetColor(mFill);
		g->FillRect(0, 0, mWidth, mHeight);
		if (mFill.mAlpha < 0)
			mFill.mAlpha = 0;
	}

	// the flash of lightning of the freeze (3D only)
	if (gApp->Is3DAccelerated() && aBoard->m19f0 / 4 > 0 && (aBoard->mWarpFlight || aBoard->mAtWarpGate || aBoard->mWarpGatePicked))
	{
		g->SetColor(Color(200, 200, 200, aBoard->m19f0 / 4));
		g->FillRect(0, 0, mWidth, mHeight);
	}

	int aPause = aBoard->m1924;
	if (aPause < 1)
	{
		if (!aHintShown)
			return;
	}
	else
	{
		if (!aHintShown && !mFlag78 && !aBoard->mWeaponHintShown)
		{
			// the pause: the field darkens and says so
			if (aBoard->m1928 > 0)
			{
				int aShade = aBoard->m1928 / 2;
				if (aShade > 0xe6)
					aShade = 0xe6;
				g->SetColor(Color(0, 0, 0, aShade));
				g->FillRect(0, 0, mWidth, mHeight);
			}
			if (!aBoard->mPauseTextWanted)
				return;
			if (aBoard->mScreenWiping)
				return;
			if (gApp->mOpenDialogs != 0)
				return;
			g->SetFont(GetFontById(0xb1));
			g->SetColor(Color(0xff, 0xff, 0xff));
			WriteString(g, GT(0x564510), 0, mHeight / 2 - FONT_POINTS->mHeight / 2, mWidth, 0, true, 0, -1);
			WriteString(g, GT(0x5644ec), 0, (mHeight / 2 - FONT_POINTS->mHeight / 2) + FONT_POINTS->mHeight, mWidth, 0, true, 0, -1);
			return;
		}

		if (!mFlag78 && aHintShown && !aBoard->mWeaponHintShown)
			DrawHint(g);
		else if (!mFlag78 && aBoard->mWeaponHintShown)
			DrawWeaponHint(g);
		else if (mFlag78)
			DrawEmergency(g);
	}

	// a hint over the streaks that fade out (the end of a stage)
	if (aHintShown && aBoard->mStreaksFadingOut)
	{
		g->SetColor(mFill);
		g->FillRect(0, 0, mWidth, mHeight);
		if (mFill.mAlpha < 0)
			mFill.mAlpha = 0;
	}
}

// the part of 0x43fc10 that draws a hint of the tutorial: the field darkened but for the ring around the brick it is about,
// the arrow towards it, and the panel with its title, text and picture
void BoardOverlay::DrawHint(Graphics* g)
{
	Board* aBoard = mBoard;
	HintSpot* aSpot = aBoard->mHintSpots[0];
	StageBg* aBg = aSpot->mBg;

	g->SetColor(Color(0, 0, 0, 0xc0));
	for (size_t i = 0; i < aSpot->mHole.size(); i++)
		g->FillRect(aSpot->mHole[i]);

	if (aSpot->mRingX >= 0)
		g->DrawImage(IMAGE_TH_RING, aSpot->mRingX, aSpot->mRingY);

	g->SetColor(Color(aBg->mColor2.mRed, aBg->mColor2.mGreen, aBg->mColor2.mBlue));
	g->SetColorizeImages(true);
	if (aSpot->mArrowX >= 0)
		g->DrawImage(IMAGE_TH_ARROWHEAD, aSpot->mArrowX, aSpot->mArrowY);

	g->SetColor(Color(aBg->mColor2.mRed, aBg->mColor2.mGreen, aBg->mColor2.mBlue, 0x80));
	if (aSpot->mArrowTop > 0)
		g->FillRect(aSpot->mArrowX + 0x14, IMAGE_TH_ARROWHEAD->mHeight + aSpot->mArrowY, IMAGE_TH_ARROWHEAD->mWidth - 0x28, aSpot->mArrowTop);

	g->SetColor(Color(aBg->mColor2.mRed, aBg->mColor2.mGreen, aBg->mColor2.mBlue, aBg->mPanelAlpha));
	g->DrawImage(IMAGE_TH_STRIP, 0, 0xb4);

	int anAlpha = aBg->mPicAlpha;
	if (anAlpha > 0)
	{
		if (aBg->mPanelAlphaStep < 0 && aBg->mPanelAlpha <= anAlpha)
			anAlpha = aBg->mPanelAlpha;
		int aPicHeight = aBg->mPic->mHeight;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		int aCel = (aBg->mPicAnim.mNumFrames == 1) ? 0 : aBg->mPicAnim.mFrame;
		DrawCel(g, aBg->mPic, aBg->mX, 0xd8 - aPicHeight / 2, aCel);
	}

	Image* aPicture = aBg->mOverPic;
	if (aPicture != NULL && aBg->mOverPicAlpha > 0)
	{
		int aPictureAlpha = aBg->mOverPicAlpha;
		if (aBg->mPanelAlphaStep < 0 && aBg->mPanelAlpha <= aPictureAlpha)
			aPictureAlpha = aBg->mPanelAlpha;
		int aPictureTop = 0x24 - aPicture->mHeight / 2;
		int aCelWidth = aPicture->mWidth / aPicture->mNumCols;
		g->SetColor(Color(0xff, 0xff, 0xff, aPictureAlpha));
		int aCel = (aBg->mOverPicAnim.mNumFrames == 1) ? 0 : aBg->mOverPicAnim.mFrame;
		DrawCel(g, aPicture, (aBg->mPic->mWidth / 2 + aBg->mX) - aCelWidth / 2, aPictureTop + 0xb4, aCel);
	}
	g->SetColorizeImages(false);

	// the title in the first colour, the text in white
	Font* aTitleFont = GetFontById(0x121);
	g->SetFont(aTitleFont);
	g->SetColor(Color(aBg->mColor1.mRed, aBg->mColor1.mGreen, aBg->mColor1.mBlue, aBg->mPanelAlpha));
	int aTitleAscent = aTitleFont->GetAscent();
	int aTitleHeight = aTitleFont->GetHeight();
	WriteWordWrapped(g, Rect(aBg->mTitleX, (-(aBg->mTitleHeight / 2) - (aTitleHeight - aTitleAscent) / 2) + 0xe0, aBg->mTitleWidth, 0x3c), aBg->mTitle, -1, 0);

	Font* aTextFont = GetFontById(0xb1);
	g->SetFont(aTextFont);
	g->SetColor(Color(0xff, 0xff, 0xff, aBg->mPanelAlpha));
	int aTextAscent = aTextFont->GetAscent();
	WriteWordWrapped(g, Rect(aBg->mTextX, (-(aBg->mTextHeight / 2) - aTextAscent / 2) + 0xe0, 0xd2, 0x3c), aBg->mText, -1, 0);

	// the label of the check box that turns the hints off
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->SetFont(GetFontById(0xb1));
	Checkbox* aCheck = aBoard->mHintOffCheckbox;
	g->DrawString(GT(0x5644d8), IMAGE_TH_CHECKBOX->mWidth / 2 + 10 + aCheck->mX, aTextAscent + aCheck->mY);
}

// the part of 0x43fc10 that draws the weapon's hint (the first time its meter is full): the field darkened round a panel
// that says what the weapon does, with the two pictures of the pilot's weapon
void BoardOverlay::DrawWeaponHint(Graphics* g)
{
	Board* aBoard = mBoard;

	g->SetColor(Color(0, 0, 0, 0xc0));
	g->FillRect(0, 0, IMAGE_TH_RECTANGLE->mWidth, 0xc4);
	g->FillRect(0, IMAGE_TH_RECTANGLE->mHeight + 0xc4, IMAGE_TH_RECTANGLE->mWidth, 0x11c - IMAGE_TH_RECTANGLE->mHeight);
	g->FillRect(IMAGE_TH_RECTANGLE->mWidth, 0, 0x280 - IMAGE_TH_RECTANGLE->mWidth, 0x1e0);
	g->DrawImage(IMAGE_TH_RECTANGLE, 0, 0xc4);

	g->SetColor(Color(0xaf, 0, 0xaa, 0x80));
	g->FillRect(0, 0x20, 0x280, 0x7a);
	g->SetColor(Color(0xaf, 0, 0xaa, 0x5c));
	g->FillRect(0, 0x9a, 0x280, 0x1b);
	int anArrowRight = IMAGE_TH_LEFT_ARROWHEAD->mWidth + 0x91;
	g->SetColor(Color(0xaf, 0, 0xaa, 0x5c));
	g->FillRect(0x102, 0xb5, 0x32, IMAGE_TH_LEFT_ARROWHEAD->mHeight - 6);
	g->FillRect(anArrowRight, 0xd7, 0x102 - anArrowRight, IMAGE_TH_LEFT_ARROWHEAD->mHeight - 0x28);
	g->SetColorizeImages(true);
	g->SetColor(Color(0xaf, 0, 0xaa));
	g->DrawImage(IMAGE_TH_LEFT_ARROWHEAD, 0x91, 0xc3);
	g->SetColorizeImages(false);

	// the title, centred on the middle of the panel by the height of its text as measured first
	g->SetColor(Color(0xff, 0, 0xff));
	Font* aTitleFont = GetFontById(0x121);
	g->SetFont(aTitleFont);
	Graphics aMeasure;
	aMeasure.SetFont(aTitleFont);
	int aTitleLines = WriteWordWrapped(&aMeasure, Rect(0x8c, 0x20, 0x96, 0x7a), GT(0x5644c8), -1, 0);
	int aTitleHeight = aTitleFont->GetHeight();
	int aTitleAscent = aTitleFont->GetAscent();
	WriteWordWrapped(g, Rect(0x8c, (-((aTitleHeight - aTitleAscent) / 2) - aTitleLines / 2) + 99, 0x96, 0x7a), GT(0x5644bc), -1, 0);

	// the text of the pilot's weapon and its two pictures
	std::string aText;
	Image* aPicture1 = NULL;
	Image* aPicture2 = NULL;
	switch (aBoard->mMode)
	{
	case 0:
		aText = GT(0x564468);
		aPicture1 = IMAGE_TH_SUPA_VECTOR1;
		aPicture2 = IMAGE_TH_SUPA_VECTOR2;
		break;
	case 1:
		aText = GT(0x564418);
		aPicture1 = IMAGE_TH_SUPA_SPROCKET1;
		aPicture2 = IMAGE_TH_SUPA_SPROCKET2;
		break;
	case 2:
		aText = GT(0x5643d0);
		aPicture1 = IMAGE_TH_SUPA_TURBOT1;
		aPicture2 = IMAGE_TH_SUPA_TURBOT2;
		break;
	case 3:
		aText = GT(0x564368);
		aPicture1 = IMAGE_TH_SUPA_VIXX1;
		aPicture2 = IMAGE_TH_SUPA_VIXX2;
		break;
	}
	Font* aTextFont = GetFontById(0xb1);
	aMeasure.SetFont(aTextFont);
	int aTextLines = WriteWordWrapped(&aMeasure, Rect(300, 0x20, 0xd2, 0x7a), aText, -1, 0);
	int aTextAscent = aTextFont->GetAscent();
	g->SetFont(aTextFont);
	WriteWordWrapped(g, Rect(300, (-(aTextLines / 2) - aTextAscent / 2) + 99, 0xd2, 0x7a), aText, -1, 0);

	g->SetColorizeImages(true);
	if (aBoard->mWeaponHintPulse.mA > 0 && aPicture1 != NULL)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, aBoard->mWeaponHintPulse.mA));
		g->DrawImage(aPicture1, 0x210, 0x5d - aPicture1->mHeight / 2);
	}
	if (aBoard->mWeaponHintPulse.mB > 0 && aPicture2 != NULL)
	{
		g->SetColor(Color(0xff, 0xff, 0xff, aBoard->mWeaponHintPulse.mB));
		g->DrawImage(aPicture2, 0x210, 0x5d - aPicture2->mHeight / 2);
	}
	g->SetColorizeImages(false);
}

// the part of 0x43fc10 that draws the emergency (a ship about to crash with the weapon full): a red panel and the words
void BoardOverlay::DrawEmergency(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 0xc0));
	g->FillRect(0, 0, IMAGE_TH_RECTANGLE->mWidth - 6, 0xbf);
	g->FillRect(0, IMAGE_SUPA_RING->mHeight + 0xbf, IMAGE_SUPA_RING->mWidth - 6, 0x121 - IMAGE_SUPA_RING->mHeight);
	g->FillRect(IMAGE_SUPA_RING->mWidth - 6, 0, 0x286 - IMAGE_SUPA_RING->mWidth, 0x1e0);
	g->DrawImage(IMAGE_SUPA_RING, -6, 0xbf);

	Color aRed(0xff, 0x14, 10);
	Color aDarkRed(100, 0xf, 0);
	Color aBlackRed(0x3c, 0, 0);
	g->SetColor(aRed);
	g->FillRect(200, 0x82, 0x181, 0x28);
	g->SetColor(aBlackRed);
	g->FillRect(200, 0x141, 0x181, 0x28);
	g->SetColor(aDarkRed);
	g->FillRect(200, 0xad, 0x181, 0x91);
	int anArrowHeight = IMAGE_TH_LEFT_ARROWHEAD->mHeight;
	int anArrowRight = IMAGE_TH_LEFT_ARROWHEAD->mWidth + 0x82;
	g->SetColor(Color(0x5c, 7, 4));
	g->FillRect(anArrowRight, anArrowHeight / 2 + 0xbe, 200 - anArrowRight, 0x14);
	g->SetColorizeImages(true);
	g->SetColor(aRed);
	g->DrawImage(IMAGE_TH_LEFT_ARROWHEAD, 0x82, 200);
	g->SetColorizeImages(false);

	g->SetColor(Color(0xff, 0xe6, 0));
	Font* aTitleFont = GetFontById(0x121);
	g->SetFont(aTitleFont);
	WriteString(g, GT(0x56435c), 200, 0xab - aTitleFont->GetHeight() / 2, 0x181, 0, true, 0, -1);

	Font* aTextFont = GetFontById(0xb1);
	g->SetColor(Color(0xff, 0xff, 0));
	g->SetFont(aTextFont);
	int anAscent = aTextFont->GetAscent();
	WriteString(g, GT(0x56433c), 200, anAscent + 0xc1, 0x181, 0, true, 0, -1);

	g->SetColor(Color(0xff, 0xff, 0xff));
	WriteWordWrapped(g, Rect(0xf0, anAscent + 0xd5, 0x13b, 400), GT(0x5642f8), -1, 0);
	WriteString(g, GT(0x5642c8), 200, 0x121, 0x181, 0, true, 0, -1);

	g->SetColor(Color(0x9b, 0, 0));
	int aTextHeight = aTextFont->GetHeight();
	WriteWordWrapped(g, Rect(0xf0, 0x14d - aTextHeight / 2, 0x13b, 0x28), GT(0x564298), -1, 0);
}
