// Board: the screen between two levels (0x442c70): the slow gate (the next level) and, when a checkpoint was passed, the
// fast gate (the warp to a later one), each with its planet, its title and what the mission is for.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// the texts under the gates are in the colour codes of WriteWordWrapped
#define kClickForNext GTC(0x005646f0)
#define kForBonusRound GTC(0x005646b8)
#define kForUpgrade GTC(0x0056467c)
#define kForPilot GTC(0x00564640)

// 0x442c70
void Board::DrawWarpScreen(Graphics* g)
{
	if ((m1b2c == 2 || m1b2c == 4 || m1b2c == 3) && !m91a)
		return;

	if (m19e0 < 0xff)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, m19e0));
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	int aGateCel = (mAnim460.mNumFrames == 1) ? 0 : mAnim460.mFrame;
	DrawCel(g, IMAGE_SLOW_WARP_GATE, m1a70, 0xe1, aGateCel);
	if (m1a08 > 0)
		DrawCel(g, IMAGE_FAST_WARP_GATE, m1a74, 0xe1, aGateCel);
	if (m19e0 == 0xff)
		g->SetColorizeImages(true);
	if (m19e8 > 0)
	{
		g->SetColor(Color(m19e8, m19e8, m19e8, m19e8));
		DrawCel(g, IMAGE_SLOW_WARP_GATE, m1a70, 0xe1, aGateCel);
	}
	if (m1a08 > 0 && m19ec > 0)
	{
		g->SetColor(Color(m19ec, m19ec, m19ec, m19ec));
		DrawCel(g, IMAGE_FAST_WARP_GATE, m1a74, 0xe1, aGateCel);
	}
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// what the mission of the slow gate is for, and which planet it shows
	std::string aText = kClickForNext;
	int aKind = m1b28;
	int aCelWidth = IMAGE_WARP_PLANETS->GetCelWidth();
	int aPlanetHeight = IMAGE_WARP_PLANETS->mHeight;
	Rect aSrc(0, 0, 0, 0);
	if (aKind != 3 && aKind != 4)
	{
		int aNextLevel = m1940 + 1;
		if (aNextLevel % 4 == 0 && aNextLevel <= 0x20)
			aKind = 2;
	}
	if ((unsigned int) aKind <= 4)
	{
		switch (aKind)
		{
		case 0:
			aSrc = Rect(0, 0, aCelWidth, aPlanetHeight);
			break;
		case 1:
			aText = kForBonusRound;
			aSrc = Rect(aCelWidth, 0, aCelWidth, aPlanetHeight);
			break;
		case 2:
			aText = kForUpgrade;
			aSrc = Rect(aCelWidth * 2, 0, aCelWidth, aPlanetHeight);
			break;
		default:
			aText = kForPilot;
			aSrc = Rect(aCelWidth * 3, 0, aCelWidth, aPlanetHeight);
			break;
		}
	}

	if (Fn433130(m1940 + 1) == 0)
		aText = kClickForNext;
	if ((m1940 + 1) % 4 == 0 && m1b28 != 3 && m1b28 != 4)
		aSrc = Rect(aCelWidth * 2, 0, aCelWidth, aPlanetHeight);
	g->DrawImage(IMAGE_WARP_PLANETS,
		(IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - IMAGE_WARP_PLANETS->GetCelWidth() / 2) + m1a70,
		(IMAGE_SLOW_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + 0xe1, aSrc);

	// the fast gate: its mission, always with the third planet
	std::string aFastText = kClickForNext;
	if (m1a08 > 0)
	{
		switch (Fn433130(m1a08 / 2 + 1))
		{
		case 1:
			aFastText = kForBonusRound;
			break;
		case 2:
			aFastText = kForUpgrade;
			break;
		case 3:
		case 4:
			aFastText = kForPilot;
			break;
		default:
			break;
		}
		Rect aFastSrc(aCelWidth * 2, 0, aCelWidth, aPlanetHeight);
		g->DrawImage(IMAGE_WARP_PLANETS,
			(IMAGE_FAST_WARP_GATE->GetCelWidth() / 2 + m1a74) - IMAGE_WARP_PLANETS->GetCelWidth() / 2,
			(IMAGE_FAST_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + 0xe1, aFastSrc);
	}

	// the titles over the gates
	g->SetColor(Color(0xff, 0xff, 0xff));
	Font* aFont = GetFontById((m1a08 > 0) ? FONT_DIALOG_BOX_BTN_ID : FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	std::string aTitle = StrFormat(GTC(0x00564634), m1940 + 1);
	if (aKind == 2)
		aTitle += GT(0x00564624);
	int aTitleY = 0xfa - g->GetFont()->mHeight;
	g->DrawString(aTitle, (IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - aFont->StringWidth(aTitle) / 2) + m1a70, aTitleY);

	if (!aText.empty())
	{
		g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
		WriteWordWrapped(g, Rect(m1a70, 0x168, IMAGE_SLOW_WARP_GATE->GetCelWidth(), 200), aText, -1, 0);
	}

	if (m1a08 > 0)
	{
		std::string aFastTitle = StrFormat(GTC(0x0056460c), m1a08 / 2 + 1);
		int aFastTitleY = 0xfa - g->GetFont()->mHeight;
		g->DrawString(aFastTitle,
			(IMAGE_FAST_WARP_GATE->GetCelWidth() / 2 + m1a74) - aFont->StringWidth(aFastTitle) / 2, aFastTitleY);
		WriteWordWrapped(g, Rect(m1a74, 0x168, IMAGE_SLOW_WARP_GATE->GetCelWidth(), 200), aFastText, -1, 0);
	}
}
