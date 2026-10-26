// port(variant): arrow gems' drawing (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.20, stage 18; the rules are in
// BoardArrows.cpp). Not part of the original game. Apart from the board's rules so the verifier, which draws nothing,
// need not build the art: it has empty ones (headless/HeadlessShims.cpp).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "art/GemArt.h"
#include "art/Raster.h"

#include <algorithm>
#include <vector>

using namespace Sexy;

namespace
{

enum
{
	ARROW_PULSE_TICKS = 60      // the warning's pulse
};

// The warning's glow, one per gem colour (art/GemArt's own colours)
const Color kArrowGlow[12] = {Color(250, 212, 28), Color(232, 232, 242), Color(38, 122, 250), Color(232, 30, 48),
	Color(184, 62, 226), Color(255, 136, 24), Color(46, 194, 64), Color(255, 110, 188), Color(22, 194, 198),
	Color(124, 86, 255), Color(150, 150, 170), Color(166, 234, 38)};

// The arrows' sheet (GemArt::DrawArrowSheet), made on first use at the gem sheet's HD scale, again when a board needs
// more colours or the scale changes
MemoryImage* ArrowSheet(WinDMApp* theApp, int theColors)
{
	static MemoryImage* sSheet = NULL;
	static int sColors = 0;
	static int sScale = 0;
	MemoryImage* aGems = (MemoryImage*) theApp->mImages[IMAGE_GEMSHEET];
	int aScale = ((aGems != NULL) && aGems->HasMirrorableHDBits()) ? aGems->mHDScale : 1;
	aScale = std::max(1, std::min(4, aScale));
	theColors = std::max(1, std::min(12, theColors));
	if ((sSheet != NULL) && (sColors >= theColors) && (sScale == aScale))
		return sSheet;
	std::vector<uint32_t> aBits;
	Art::DrawArrowSheet(theColors, aScale, aBits);
	int aH = 52 * theColors;
	if (sSheet == NULL)
		sSheet = new MemoryImage(theApp);
	if (aScale == 1)
		sSheet->SetBits((ulong*) &aBits[0], 208, aH, true);
	else
	{
		std::vector<uint32_t> aLow;
		Art::DownsampleArgb(aBits, 208 * aScale, aH * aScale, aScale, aLow);
		sSheet->SetBits((ulong*) &aLow[0], 208, aH, true);
		sSheet->SetHDBits(&aBits[0], 208 * aScale, aH * aScale, aScale);
	}
	sColors = theColors;
	sScale = aScale;
	return sSheet;
}

}

bool Board::DrawArrowGem(Graphics* g, Piece* thePiece, int theX, int theY)
{
	if (!mVariant.mArrows || (thePiece->mArrow == 0) || thePiece->mDying)
		return false;
	MemoryImage* aSheet = ArrowSheet(mApp, mRules.mBoard.mGemColors);
	int aColor = std::min(thePiece->mColor, 11);
	int aDir = thePiece->mArrow - 1;
	g->DrawImage(aSheet, theX, theY, Rect(aDir * 52 + 2, aColor * 52, GEM_SIZE, GEM_SIZE));
	// one step from its edge: it pulses white
	if ((mState == STATE_PLAYING) && (ArrowDistance(thePiece->mCol, thePiece->mRow, aDir) == 0))
	{
		int aPhase = mUpdateCnt % ARROW_PULSE_TICKS;
		int anAlpha = 150 * std::min(aPhase, ARROW_PULSE_TICKS - aPhase) / (ARROW_PULSE_TICKS / 2);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, anAlpha));
		g->DrawImage(aSheet, theX, theY, Rect(aDir * 52 + 2, aColor * 52, GEM_SIZE, GEM_SIZE));
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	return true;
}

// Over the gems: beyond each arrow one step from its edge, a bar in the arrow's colour glows along that edge of its cell
void Board::DrawArrowWarnings(Graphics* g)
{
	if (mState != STATE_PLAYING)
		return;
	int aPhase = mUpdateCnt % ARROW_PULSE_TICKS;
	int anAlpha = 90 + 140 * std::min(aPhase, ARROW_PULSE_TICKS - aPhase) / (ARROW_PULSE_TICKS / 2);
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mArrow == 0) || aPiece->mDying)
				continue;
			int aDir = aPiece->mArrow - 1;
			if (ArrowDistance(aCol, aRow, aDir) != 0)
				continue;
			Color aColor = kArrowGlow[std::min(aPiece->mColor, 11)];
			aColor.mAlpha = anAlpha;
			g->SetColor(aColor);
			int aX = GetColX(aCol), aY = GetRowY(aRow);
			// the cell's outer side, 4 px thick, just past the gem
			switch (aDir)
			{
			case GameRules::ARROW_UP: g->FillRect(aX + 4, aY - 4, CELL_SIZE - 10, 4); break;
			case GameRules::ARROW_DOWN: g->FillRect(aX + 4, aY + GEM_SIZE + 1, CELL_SIZE - 10, 4); break;
			case GameRules::ARROW_LEFT: g->FillRect(aX - 4, aY + 4, 4, CELL_SIZE - 10); break;
			default: g->FillRect(aX + GEM_SIZE + 1, aY + 4, 4, CELL_SIZE - 10); break;
			}
		}
	}
}
