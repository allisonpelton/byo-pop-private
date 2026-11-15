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
	if ((mLevelEndScreenKind == 2 || mLevelEndScreenKind == 4 || mLevelEndScreenKind == 3) && !mUpgradeDialogDone)
		return;

	if (mWarpGateAlpha < 0xff)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, mWarpGateAlpha));
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	int aGateCel = (mWarpGateAnim.mNumFrames == 1) ? 0 : mWarpGateAnim.mFrame;
	DrawCel(g, IMAGE_SLOW_WARP_GATE, mSlowGateX, 0xe1, aGateCel);
	if (mFastGateStage > 0)
		DrawCel(g, IMAGE_FAST_WARP_GATE, mFastGateX, 0xe1, aGateCel);
	if (mWarpGateAlpha == 0xff)
		g->SetColorizeImages(true);
	if (mSlowGateGlow > 0)
	{
		g->SetColor(Color(mSlowGateGlow, mSlowGateGlow, mSlowGateGlow, mSlowGateGlow));
		DrawCel(g, IMAGE_SLOW_WARP_GATE, mSlowGateX, 0xe1, aGateCel);
	}
	if (mFastGateStage > 0 && mFastGateGlow > 0)
	{
		g->SetColor(Color(mFastGateGlow, mFastGateGlow, mFastGateGlow, mFastGateGlow));
		DrawCel(g, IMAGE_FAST_WARP_GATE, mFastGateX, 0xe1, aGateCel);
	}
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);

	// what the mission of the slow gate is for, and which planet it shows
	std::string aText = kClickForNext;
	int aKind = mNextLevelScreenKind;
	int aCelWidth = IMAGE_WARP_PLANETS->GetCelWidth();
	int aPlanetHeight = IMAGE_WARP_PLANETS->mHeight;
	Rect aSrc(0, 0, 0, 0);
	if (aKind != 3 && aKind != 4)
	{
		int aNextLevel = mLevelNumber + 1;
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

	if (UpgradeScreenKind(mLevelNumber + 1) == 0)
		aText = kClickForNext;
	if ((mLevelNumber + 1) % 4 == 0 && mNextLevelScreenKind != 3 && mNextLevelScreenKind != 4)
		aSrc = Rect(aCelWidth * 2, 0, aCelWidth, aPlanetHeight);
	g->DrawImage(IMAGE_WARP_PLANETS,
		(IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - IMAGE_WARP_PLANETS->GetCelWidth() / 2) + mSlowGateX,
		(IMAGE_SLOW_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + 0xe1, aSrc);

	// the fast gate: its mission, always with the third planet
	std::string aFastText = kClickForNext;
	if (mFastGateStage > 0)
	{
		switch (UpgradeScreenKind(mFastGateStage / 2 + 1))
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
			(IMAGE_FAST_WARP_GATE->GetCelWidth() / 2 + mFastGateX) - IMAGE_WARP_PLANETS->GetCelWidth() / 2,
			(IMAGE_FAST_WARP_GATE->mHeight / 2 - IMAGE_WARP_PLANETS->mHeight / 2) + 0xe1, aFastSrc);
	}

	// the titles over the gates
	g->SetColor(Color(0xff, 0xff, 0xff));
	Font* aFont = GetFontById((mFastGateStage > 0) ? FONT_DIALOG_BOX_BTN_ID : FONT_HIGH_SCORE_ID);
	g->SetFont(aFont);
	std::string aTitle = StrFormat(GTC(0x00564634), mLevelNumber + 1);
	if (aKind == 2)
		aTitle += GT(0x00564624);
	int aTitleY = 0xfa - g->GetFont()->mHeight;
	g->DrawString(aTitle, (IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - aFont->StringWidth(aTitle) / 2) + mSlowGateX, aTitleY);

	if (!aText.empty())
	{
		g->SetFont(GetFontById(FONT_DIALOG_BOX_BTN_ID));
		WriteWordWrapped(g, Rect(mSlowGateX, 0x168, IMAGE_SLOW_WARP_GATE->GetCelWidth(), 200), aText, -1, 0);
	}

	if (mFastGateStage > 0)
	{
		std::string aFastTitle = StrFormat(GTC(0x0056460c), mFastGateStage / 2 + 1);
		int aFastTitleY = 0xfa - g->GetFont()->mHeight;
		g->DrawString(aFastTitle,
			(IMAGE_FAST_WARP_GATE->GetCelWidth() / 2 + mFastGateX) - aFont->StringWidth(aFastTitle) / 2, aFastTitleY);
		WriteWordWrapped(g, Rect(mFastGateX, 0x168, IMAGE_SLOW_WARP_GATE->GetCelWidth(), 200), aFastText, -1, 0);
	}
}
