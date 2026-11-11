// Board: the panel of a stage's hints (0x43b5c0) and the wave of the banner's picture (0x433fa0).

#include "Board.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "SexyAppBase.h"
#include <math.h>

using namespace Sexy;

// 0x43b5c0: the panel of the hints of the stage (the strip across the screen, the two pictures that fade in, the title
// and the text)
void Board::DrawStageBg(Graphics* g)
{
	if (mDemoPlaying)
		return;

	unsigned int aCount = (unsigned int) mStageHintPanels.size();
	if ((unsigned int) mField193C >= aCount && !m90a)
		return;

	unsigned int anIndex = m90a ? 1U : (unsigned int) mField193C;
	StageBg* aBg = mStageHintPanels[anIndex];
	if (aBg->mFadedOut)
		return;

	g->SetColorizeImages(true);
	g->SetColor(Color(aBg->mColor2.mRed, aBg->mColor2.mGreen, aBg->mColor2.mBlue, aBg->mPanelAlpha));
	g->DrawImage(IMAGE_TH_STRIP, 0, 0xb4, Rect(0, 0, IMAGE_TH_STRIP->mWidth, 0x35));

	int anAlpha = aBg->mPicAlpha;
	if (anAlpha > 0)
	{
		if (aBg->mPanelAlphaStep < 0 && aBg->mPanelAlpha <= anAlpha)
			anAlpha = aBg->mPanelAlpha;
		int aHeight = aBg->mPic->mHeight;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		DrawCel(g, aBg->mPic, aBg->mX, 0x1a - aHeight / 2 + 0xb4, (aBg->mPicAnim.mNumFrames == 1) ? 0 : aBg->mPicAnim.mFrame);
	}

	if (aBg->mOverPic != NULL && aBg->mOverPicAlpha > 0)
	{
		anAlpha = aBg->mOverPicAlpha;
		if (aBg->mPanelAlphaStep < 0 && aBg->mPanelAlpha <= anAlpha)
			anAlpha = aBg->mPanelAlpha;
		int aCelWidth = aBg->mOverPic->mWidth / aBg->mOverPic->mNumCols;
		int aPicY = 0x1a - aBg->mOverPic->mHeight / 2;
		int aPicX = aBg->mPic->mWidth / 2 + aBg->mX - aCelWidth / 2;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		DrawCel(g, aBg->mOverPic, aPicX, aPicY + 0xb4, (aBg->mOverPicAnim.mNumFrames == 1) ? 0 : aBg->mOverPicAnim.mFrame);
	}

	g->SetColorizeImages(false);

	g->SetFont(GetFontById(FONT_NEURO30_ID));
	g->SetColor(Color(aBg->mColor1.mRed, aBg->mColor1.mGreen, aBg->mColor1.mBlue, aBg->mPanelAlpha));
	Font* aFont = g->GetFont();
	int aTitleY = (-(aBg->mTitleHeight / 2) - (aFont->GetHeight() - aFont->GetAscent()) / 2) + 0xd4;
	WriteWordWrapped(g, Rect(0x96, aTitleY, 0x96, 0x3c), aBg->mTitle, -1, 0);

	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
	g->SetColor(Color(0xff, 0xff, 0xff, aBg->mPanelAlpha));
	aFont = g->GetFont();
	int aTextY = (-(aBg->mTextHeight / 2) - aFont->GetAscent() / 2) + 0xd4;
	WriteWordWrapped(g, Rect(300, aTextY, 0xd2, 0x3c), aBg->mText, -1, 0);
}

// 0x433fa0: the picture of the banner (mBannerTextImage) goes into the taller one (mBannerWaveImage) with each column moved down by a wave (it
// runs with the banner's time, m944), each row in a colour of the rainbow, and the whole fading in and out.
void Board::AnimateBanner()
{
	ulong* aSrcBits = mBannerTextImage->GetBits();
	ulong* aDstBits = mBannerWaveImage->GetBits();
	int aWidth = mBannerTextImage->GetWidth();
	int aSrcHeight = mBannerTextImage->GetHeight();
	int aDstHeight = mBannerWaveImage->GetHeight();
	memset(aDstBits, 0, (size_t) aDstHeight * aWidth * 4);

	// the fade at the two ends of the banner's time
	int aLeft = m1b80 - m1b84;
	int anAlpha = 0xff;
	if (aLeft < 0x4b)
		anAlpha = (aLeft * 0xff) / 0x4b;
	else if (m1b84 < 0x4b)
		anAlpha = (m1b84 * 0xff) / 0x4b;

	// a colour for every row of the taller picture, going round the colour wheel every 64 rows
	unsigned int* aRowColors = new unsigned int[aDstHeight];
	for (int aRow = 0; aRow < aDstHeight; aRow++)
		aRowColors[aRow] = (unsigned int) mApp->HSLToRGB((aRow * 4) % 256, 0xff, 0x94);

	for (int aCol = 0; aCol < aWidth; aCol++)
	{
		// (x87 at 24 bits: the column's phase, the sum with the wave's offset, the sine plus one and the product are floats; FSIN's own result is not rounded)
		float aPhase = (float) ((double) aCol * 0.024) + m944;
		float aWave = (float) (sin((double) aPhase) + 1.0);
		int aShift = (int) (aWave * 24.0f);
		for (int aRow = 0; aRow < aSrcHeight; aRow++)
		{
			unsigned int aPixel = (unsigned int) aSrcBits[aRow * aWidth + aCol];
			unsigned int aColor = aRowColors[aShift + aRow];
			unsigned int aRed = ((aColor >> 16) & 0xff) * (aPixel & 0xff0000) & 0xff0000ff;
			unsigned int aGreen = ((aColor >> 8) & 0xff) * (aPixel & 0xff00) & 0xff0000;
			unsigned int aBlue = (aColor & 0xff) * (aPixel & 0xff) & 0xff00;
			unsigned int aAlpha = ((aPixel >> 8) & 0xff0000) * (unsigned int) anAlpha & 0xff000000;
			aDstBits[(aShift + aRow) * aWidth + aCol] = ((aRed | aGreen | aBlue) >> 8) | aAlpha;
		}
	}
	delete[] aRowColors;
	mBannerWaveImage->BitsChanged();
}
