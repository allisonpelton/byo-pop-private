// Board: the tally of a bonus round (0x443480). Its lines come one by one as m1bb0 counts up: the value of the field, the
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
	g->SetColor(Color(0xff, 0, 0, m1b8c));
	std::string aTitle = GT(0x005647a8);
	int aLeft = 0x182 - aFont->StringWidth(aTitle) / 2;
	int aBaseY = 0xcd;

	if (m1bb0 > 0)
	{
		g->DrawString(aTitle, aLeft, 0xcd);
		if (m1bb0 == 1 && m1b94 == 0)
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
	if (m1b98 == m1b94 && m1b94 == 1)
		aLabel = GT(0x0056476c);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (m1b94 > 0)
	{
		if (m1bb0 >= 2)
		{
			std::string aCount = StrFormat(GTC(0x00564768), m1b94);
			g->DrawString(aLabel, aFont->StringWidth(aCount) + aLeft, aRowY + 2);
		}
	}
	else if (m1bb0 >= 2)
	{
		std::string aCleared = GT(0x00564754);
		g->DrawString(aCleared, aLeft - aFont->StringWidth(aCleared) / 2 + 0xc1, aRowY + 2);
	}

	if (m1bb0 >= 3 && m1b94 > 0)
	{
		g->DrawString(StrFormat("%d", m1b98), aLeft, aRowY + 2);
		g->SetColor(Color(0xff, 0xff, 0));
		std::string aPenalty = StrFormat("-%s", FormatNumber(m1ba0).c_str());
		if (m1b94 > 0)
		{
			// (the penalty is lined up by the width of another number, m1b9c, as in the original)
			std::string aAligned = StrFormat("-%s", FormatNumber(m1b9c).c_str());
			g->DrawString(aPenalty, aLeft - aFont->StringWidth(aAligned) + 0x183, aRowY + 2);
		}
	}

	// the bonus for a clean field
	aFont = GetFontById(FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	int aRow2Y = aRowY + aFont->GetAscent() + 0xa;
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (m1bb0 >= 4)
	{
		if (m1b94 == 0)
			g->DrawString(GT(0x00564740), aLeft, aRow2Y);
		else
			g->DrawString(GT(0x00564728), aLeft, aRow2Y);
	}
	if (m1bb0 >= 5 && m1b94 == 0)
	{
		g->SetColor(Color(0xff, 0xff, 0));
		// (this one is lined up by the width of m1ba4, but shows m1ba8)
		std::string aAligned = StrFormat("%s", FormatNumber(m1ba4).c_str());
		std::string aBonus = FormatNumber(m1ba8);
		g->DrawString(aBonus, aLeft - aFont->StringWidth(aAligned) + 0x183, aRow2Y + 2);
	}

	// the total, under a line
	aFont = GetFontById(FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	int aRow3Y = aRow2Y + aFont->GetAscent() * 2;
	if (m1bb0 >= 6)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		aFont = GetFontById(FONT_HIGH_SCORE_ID);
		g->FillRect(Rect(aLeft, aRow3Y, 0x183, 1));
		int aRow4Y = aRow3Y + aFont->GetAscent() * 2;
		g->SetFont(aFont);
		g->SetColor(Color(0, 0xff, 0));
		// (lined up by the width of another sum, m1ba4 - m1b9c + mClearBonusWorth, as in the original)
		std::string aTotal = GT(0x00564718) + FormatNumber(m1bac);
		std::string aAligned = GT(0x00564718) + FormatNumber(m1ba4 - m1b9c + mClearBonusWorth);
		g->DrawString(aTotal, aLeft - aFont->StringWidth(aAligned) + 0x183, aRow4Y);
	}
}
