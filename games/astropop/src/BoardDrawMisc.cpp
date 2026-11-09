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
	if (m8dc)
		return;

	unsigned int aCount = (unsigned int) m220.size();
	if ((unsigned int) mField193C >= aCount && !m90a)
		return;

	unsigned int anIndex = m90a ? 1U : (unsigned int) mField193C;
	StageBg* aBg = m220[anIndex];
	if (aBg->mf0)
		return;

	g->SetColorizeImages(true);
	g->SetColor(Color(aBg->mColor2.mRed, aBg->mColor2.mGreen, aBg->mColor2.mBlue, aBg->m114));
	g->DrawImage(IMAGE_TH_STRIP, 0, 0xb4, Rect(0, 0, IMAGE_TH_STRIP->mWidth, 0x35));

	int anAlpha = aBg->m100;
	if (anAlpha > 0)
	{
		if (aBg->m118 < 0 && aBg->m114 <= anAlpha)
			anAlpha = aBg->m114;
		int aHeight = aBg->mPic->mHeight;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		DrawCel(g, aBg->mPic, aBg->mX, 0x1a - aHeight / 2 + 0xb4, (aBg->m70.mNumFrames == 1) ? 0 : aBg->m70.mFrame);
	}

	if (aBg->m5c != NULL && aBg->m104 > 0)
	{
		anAlpha = aBg->m104;
		if (aBg->m118 < 0 && aBg->m114 <= anAlpha)
			anAlpha = aBg->m114;
		int aCelWidth = aBg->m5c->mWidth / aBg->m5c->mNumCols;
		int aPicY = 0x1a - aBg->m5c->mHeight / 2;
		int aPicX = aBg->mPic->mWidth / 2 + aBg->mX - aCelWidth / 2;
		g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
		DrawCel(g, aBg->m5c, aPicX, aPicY + 0xb4, (aBg->mb0.mNumFrames == 1) ? 0 : aBg->mb0.mFrame);
	}

	g->SetColorizeImages(false);

	g->SetFont(GetFontById(FONT_NEURO30_ID));
	g->SetColor(Color(aBg->mColor1.mRed, aBg->mColor1.mGreen, aBg->mColor1.mBlue, aBg->m114));
	Font* aFont = g->GetFont();
	int aTitleY = (-(aBg->m120 / 2) - (aFont->GetHeight() - aFont->GetAscent()) / 2) + 0xd4;
	WriteWordWrapped(g, Rect(0x96, aTitleY, 0x96, 0x3c), aBg->mTitle, -1, 0);

	g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
	g->SetColor(Color(0xff, 0xff, 0xff, aBg->m114));
	aFont = g->GetFont();
	int aTextY = (-(aBg->m124 / 2) - aFont->GetAscent() / 2) + 0xd4;
	WriteWordWrapped(g, Rect(300, aTextY, 0xd2, 0x3c), aBg->mText, -1, 0);
}

// 0x433fa0: the picture of the banner (m304) goes into the taller one (m308) with each column moved down by a wave (it
// runs with the banner's time, m944), each row in a colour of the rainbow, and the whole fading in and out.
void Board::AnimateBanner()
{
	ulong* aSrcBits = m304->GetBits();
	ulong* aDstBits = m308->GetBits();
	int aWidth = m304->GetWidth();
	int aSrcHeight = m304->GetHeight();
	int aDstHeight = m308->GetHeight();
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
		int aShift = (int) ((sin((double) aCol * 0.024 + (double) m944) + 1.0) * 24.0);
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
	m308->BitsChanged();
}
