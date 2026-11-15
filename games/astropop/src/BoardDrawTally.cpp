// Board: the tally of a bonus round (0x443480). Its lines come one by one as mTallyState counts up: the value of the field, the
// bricks that are left and the penalty for them, the bonus for a clean field and the total.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x443480
void Board::DrawBonusStats(Graphics* g)
{
	Font* aFont = GetFontById(FONT_NEURO30_ID);
	g->SetFont(aFont);
	g->SetColor(Color(0xff, 0, 0, mTallyTitleAlpha));
	std::string aTitle = GT(0x005647a8);
	int aLeft = 0x182 - aFont->StringWidth(aTitle) / 2;
	int aBaseY = 0xcd;

	if (mTallyState > 0)
	{
		g->DrawString(aTitle, aLeft, 0xcd);
		if (mTallyState == 1 && mTallyBricksLeft == 0)
			g->DrawImage(IMAGE_PERFECT, 0x182 - IMAGE_PERFECT->mWidth / 2, aFont->GetHeight() * 2 + 0xb9);

		aBaseY = 0xda;
		g->SetColor(Color(0xff, 0xff, 0));
		aFont = GetFontById(FONT_HIGH_SCORE_ID);
		g->SetFont(aFont);
		g->DrawString(GT(0x00564798), aLeft, aFont->GetHeight() + 0xda);
		std::string aWorth = StrFormat("%s", FormatNumber(mClearBonusWorth).c_str());
		g->DrawString(aWorth, aLeft - aFont->StringWidth(aWorth) + 0x183, aFont->GetHeight() + 0xda);
	}

	// the bricks that are left (or none)
	int aRowY = aBaseY + aFont->GetAscent() * 2 + 0xa;
	aFont = GetFontById(FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	std::string aLabel = GT(0x00564780);
	if (mTallyBricksCounted == mTallyBricksLeft && mTallyBricksLeft == 1)
		aLabel = GT(0x0056476c);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mTallyBricksLeft > 0)
	{
		if (mTallyState >= 2)
		{
			std::string aCount = StrFormat(GTC(0x00564768), mTallyBricksLeft);
			g->DrawString(aLabel, aFont->StringWidth(aCount) + aLeft, aRowY + 2);
		}
	}
	else if (mTallyState >= 2)
	{
		std::string aCleared = GT(0x00564754);
		g->DrawString(aCleared, aLeft - aFont->StringWidth(aCleared) / 2 + 0xc1, aRowY + 2);
	}

	if (mTallyState >= 3 && mTallyBricksLeft > 0)
	{
		g->DrawString(StrFormat("%d", mTallyBricksCounted), aLeft, aRowY + 2);
		g->SetColor(Color(0xff, 0xff, 0));
		std::string aPenalty = StrFormat("-%s", FormatNumber(mTallyPenaltyShown).c_str());
		if (mTallyBricksLeft > 0)
		{
			// (the penalty is lined up by the width of another number, mTallyPenalty, as in the original)
			std::string aAligned = StrFormat("-%s", FormatNumber(mTallyPenalty).c_str());
			g->DrawString(aPenalty, aLeft - aFont->StringWidth(aAligned) + 0x183, aRowY + 2);
		}
	}

	// the bonus for a clean field
	aFont = GetFontById(FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	int aRow2Y = aRowY + aFont->GetAscent() + 0xa;
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (mTallyState >= 4)
	{
		if (mTallyBricksLeft == 0)
			g->DrawString(GT(0x00564740), aLeft, aRow2Y);
		else
			g->DrawString(GT(0x00564728), aLeft, aRow2Y);
	}
	if (mTallyState >= 5 && mTallyBricksLeft == 0)
	{
		g->SetColor(Color(0xff, 0xff, 0));
		// (this one is lined up by the width of mTallyCleanBonus, but shows mTallyCleanBonusShown)
		std::string aAligned = StrFormat("%s", FormatNumber(mTallyCleanBonus).c_str());
		std::string aBonus = FormatNumber(mTallyCleanBonusShown);
		g->DrawString(aBonus, aLeft - aFont->StringWidth(aAligned) + 0x183, aRow2Y + 2);
	}

	// the total, under a line
	aFont = GetFontById(FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	int aRow3Y = aRow2Y + aFont->GetAscent() * 2;
	if (mTallyState >= 6)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		aFont = GetFontById(FONT_HIGH_SCORE_ID);
		g->FillRect(Rect(aLeft, aRow3Y, 0x183, 1));
		int aRow4Y = aRow3Y + aFont->GetAscent() * 2;
		g->SetFont(aFont);
		g->SetColor(Color(0, 0xff, 0));
		// (lined up by the width of another sum, mTallyCleanBonus - mTallyPenalty + mClearBonusWorth, as in the original)
		std::string aTotal = GT(0x00564718) + FormatNumber(mTallyPaid);
		std::string aAligned = GT(0x00564718) + FormatNumber(mTallyCleanBonus - mTallyPenalty + mClearBonusWorth);
		g->DrawString(aTotal, aLeft - aFont->StringWidth(aAligned) + 0x183, aRow4Y);
	}
}
