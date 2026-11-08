// Board: the panel that slides in over the field at the end of a game (0x440f70): the lines of the results type
// themselves out one letter at a time, and the rank with its picture comes up last.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x440f70
void Board::DrawHud(Graphics* g)
{
	if (m8ed || m8ef)
	{
		if (m7f4 > 0 && mApp->Is3DAccelerated())
		{
			g->SetColor(Color(100, 0xff, 100, 0x32));
			g->FillRect(0x9b, m7f0, 0x1d1, m7f4);
		}
		g->DrawImage(IMAGE_HUD_UI, 0x7d, m7e8);
	}
	else if (m7e8 != -m7ec)
		g->DrawImage(IMAGE_HUD_UI, 0x7d, m7e8);

	if ((!m8ed && !m8ef) || m7e8 != 0)
		return;

	g->DrawImage(IMAGE_STATS_TEXT, 0xc6, 0x1e);

	int aY = 0x5a;
	Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
	g->SetFont(aFont);
	bool aLastIsOrange = m1b2c == 1;                // the last line of the results is in another colour
	int aSpacing = aLastIsOrange ? 0 : 8;
	unsigned int aLineCount = (unsigned int) m7cc.size();

	// the lines that are typed out already: the label up to the colon, then the value after it
	for (int aLine = 0; aLine < m7e0; aLine++)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		if (aLine > 0)
			aY += aFont->mHeight + aSpacing;

		const std::string& aText = m7cc[aLine];
		size_t aColon = aText.find(':', 0);
		std::string aLabel = (aColon == std::string::npos) ? aText : aText.substr(0, aColon + 1);
		g->DrawString(aLabel, 0xb2, aY);
		if (aColon != std::string::npos)
		{
			std::string aValue = aText.substr(aColon + 1, aText.length() - aColon - 1);
			if (aLastIsOrange && (unsigned int) aLine >= aLineCount - 1U)
				g->SetColor(Color(0xeb, 0xa4, 0));
			else
				g->SetColor(Color(0xff, 0xff, 0));
			g->DrawString(aValue, aFont->StringWidth(aLabel) + 0xb2, aY);
		}
	}
	if (m7e0 > 0)
		aY += aFont->mHeight + aSpacing;

	// the line that is being typed
	std::string aPartial;
	if (m7e0 < (int) aLineCount)
	{
		aPartial = m7cc[m7e0].substr(0, m7e4 + 1);
		int aColon = (int) aPartial.find(':', 0);
		g->SetColor(Color(0xff, 0xff, 0xff));
		if (aColon < 0)
			g->DrawString(aPartial, 0xb2, aY);
		else
		{
			std::string aLabel = aPartial.substr(0, aColon + 1);
			std::string aValue = aPartial.substr(aColon + 1, aPartial.length() - aColon - 1);
			g->DrawString(aLabel, 0xb2, aY);
			if (aLastIsOrange && aLineCount - 1U <= (unsigned int) m7e0)
				g->SetColor(Color(0xeb, 0xa4, 0));
			else
				g->SetColor(Color(0xff, 0xff, 0));
			g->DrawString(aValue, aFont->StringWidth(aLabel) + 0xb2, aY);
		}
	}

	// the cursor after the typed letters
	if (m804)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->FillRect(aFont->StringWidth(aPartial + " ") + 0xb2, aY - aFont->mHeight + 4, 0xf, aFont->mHeight - 5);
	}

	// the rank, with its picture
	g->DrawImage(m30c, 0x1c9, 0xf);
	std::string aRank = mApp->GetRankName(mScore, NULL);
	if (!mAnim5e0.mStopped)
	{
		// a new rank: its name flashes red with the frame of the animation
		g->SetColor(Color(0xff, 0, 0, ((mAnim5e0.mNumFrames == 1) ? 0 : mAnim5e0.mFrame) * 4));
		std::string aText = GT(0x00564518);
		Rect aRect(0x1d5, g->GetFont()->GetHeight() + 0x87, 0x79, 0x88);
		WriteWordWrapped(g, aRect, aText, -1, 0);
	}
	else
	{
		std::string aText = GT(0x00564524) + aRank;
		Rect aRect(0x1c7, g->GetFont()->GetHeight() + 0x87, 0x96, 0x88);
		WriteWordWrapped(g, aRect, aText, -1, 0);
	}
}
