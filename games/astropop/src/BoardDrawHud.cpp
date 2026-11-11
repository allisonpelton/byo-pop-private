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
	if (mWarpFlight || mAtWarpGate)
	{
		if (mLevelUpScanHeight > 0 && mApp->Is3DAccelerated())
		{
			g->SetColor(Color(100, 0xff, 100, 0x32));
			g->FillRect(0x9b, mLevelUpScanY, 0x1d1, mLevelUpScanHeight);
		}
		g->DrawImage(IMAGE_HUD_UI, 0x7d, mLevelUpPanelY);
	}
	else if (mLevelUpPanelY != -mLevelUpPanelHeight)
		g->DrawImage(IMAGE_HUD_UI, 0x7d, mLevelUpPanelY);

	if ((!mWarpFlight && !mAtWarpGate) || mLevelUpPanelY != 0)
		return;

	g->DrawImage(IMAGE_STATS_TEXT, 0xc6, 0x1e);

	int aY = 0x5a;
	Font* aFont = GetFontById(FONT_DIALOG_BOX_BTN_ID);
	g->SetFont(aFont);
	bool aLastIsOrange = m1b2c == 1;                // the last line of the results is in another colour
	int aSpacing = aLastIsOrange ? 0 : 8;
	unsigned int aLineCount = (unsigned int) mLevelUpLines.size();

	// the lines that are typed out already: the label up to the colon, then the value after it
	for (int aLine = 0; aLine < mLevelUpLineIndex; aLine++)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		if (aLine > 0)
			aY += aFont->mHeight + aSpacing;

		const std::string& aText = mLevelUpLines[aLine];
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
	if (mLevelUpLineIndex > 0)
		aY += aFont->mHeight + aSpacing;

	// the line that is being typed
	std::string aPartial;
	if (mLevelUpLineIndex < (int) aLineCount)
	{
		aPartial = mLevelUpLines[mLevelUpLineIndex].substr(0, mLevelUpLetterIndex + 1);
		int aColon = (int) aPartial.find(':', 0);
		g->SetColor(Color(0xff, 0xff, 0xff));
		if (aColon < 0)
			g->DrawString(aPartial, 0xb2, aY);
		else
		{
			std::string aLabel = aPartial.substr(0, aColon + 1);
			std::string aValue = aPartial.substr(aColon + 1, aPartial.length() - aColon - 1);
			g->DrawString(aLabel, 0xb2, aY);
			if (aLastIsOrange && aLineCount - 1U <= (unsigned int) mLevelUpLineIndex)
				g->SetColor(Color(0xeb, 0xa4, 0));
			else
				g->SetColor(Color(0xff, 0xff, 0));
			g->DrawString(aValue, aFont->StringWidth(aLabel) + 0xb2, aY);
		}
	}

	// the cursor after the typed letters
	if (mLevelUpCursorShown)
	{
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->FillRect(aFont->StringWidth(aPartial + " ") + 0xb2, aY - aFont->mHeight + 4, 0xf, aFont->mHeight - 5);
	}

	// the rank, with its picture
	g->DrawImage(mRankImage, 0x1c9, 0xf);
	std::string aRank = mApp->GetRankName(mScore, NULL);
	if (!mNewRankAnim.mStopped)
	{
		// a new rank: its name flashes red with the frame of the animation
		g->SetColor(Color(0xff, 0, 0, ((mNewRankAnim.mNumFrames == 1) ? 0 : mNewRankAnim.mFrame) * 4));
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
