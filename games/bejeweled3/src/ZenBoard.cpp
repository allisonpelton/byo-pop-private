// Sexy::ZenBoard: see the header.
#include "ZenBoard.h"

#include "Bej3App.h"
#include "Common.h"
#include "Graphics.h"
#include "Res.h"
#include "X86Math.h"

#include <climits>

namespace Sexy {

// 0x7126f0 (bejeweled3): Board's constructor 0x74ae30, then the vftable; the "Title" of the settings is "Zen" (the song and the log's mode)
ZenBoard::ZenBoard()
{
	mSettings["Title"] = "Zen";
}

// slot 154 (0x268), 0x70e450 (bejeweled3)
int ZenBoard::GetLevelMax() const
{
	int aIndex = mLevelIndex;
	if (aIndex > 0x1d)
		aIndex = 0x1e;
	return aIndex * 0x2ee + 0x9c4;
}

// slot 161 (0x284), 0x70e4a0 (bejeweled3): a move is made for the player instead of the game's end. The two draws are of the board's
// own generator (+0x258: 0x5865a0 with the address in EDX), in this order: the column, then the row
void ZenBoard::NoMoreMoves(bool theAnnounce)
{
	(void)theAnnounce;
	unsigned long aCol = mRand.Next() & 7;
	unsigned long aRow = mRand.Next() & 7;
	if (aRow < ROWS && aCol < COLS && mGrid[aRow][aCol] != nullptr)
		MakeHypercube(mGrid[aRow][aCol], true);   // slot 167 (0x29c)
}

namespace {
// the left panel's middle (slot 248, 0x3e0, which Zen does not change) and a measure in 1200ths of the art height
const int PANEL_MIDDLE_X = 0xfa;
int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }
}  // namespace

// slot 249 (0x3e4), 0x70fdf0 (bejeweled3): the picture IMAGE_ZENNECKLACE_SCORE (the score's plate) at its place in resources.xml with the panel's
// middle less 0xfc, in the board's alpha; then the necklace: jewels and strings along its curve, which the level fills (the board's +0xe04, the level index, is
// the one it shows), their patterns and curves in 0x70fdf0 lines 100 to 374.
// port: the necklace is not drawn (an Oddity in docs/NOTES.md: the jewel and string pictures, their offsets, the pattern table 0xd25bd8 and the
// scale and fade curves of each jewel are read only as far as the structure)
void ZenBoard::DrawTopWidget(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	if (IMAGE_ZENNECKLACE_SCORE != nullptr)
		g->DrawImage(IMAGE_ZENNECKLACE_SCORE, Scaled(PANEL_MIDDLE_X - 0xfc + GetImageOffsetX(IMAGE_ZENNECKLACE_SCORE_ID)),
			Scaled(GetImageOffsetY(IMAGE_ZENNECKLACE_SCORE_ID)));
}

// slot 255 (0x3fc), 0x70e5c0 (bejeweled3): the shown score (the board's +0xd64), or the text of string 0x200 ("MAX SCORE") when it is
// 0x7fffffff, in the middle of the panel at y 0x7d. port: the font is the score's (the exe draws with whatever the screen frame left; the
// Classic score's is FONT_SCORE) and its OUTLINE layer is not recoloured
void ZenBoard::DrawScoreText(Graphics* g)
{
	g->SetFont(FONT_SCORE);
	g->SetColor(Color(255, 255, 255, Ftol(mCoverCurve.GetOutVal() * 255.0)));
	std::string aText = mScore == INT_MAX ? gApp->GetString("512", "MAX SCORE") : CommaSeperate(mScore);
	g->WriteString(aText, Scaled(PANEL_MIDDLE_X), Scaled(0x7d), -1, 0, true, 0, -1, -1);
}

// slot 251 (0x3ec), 0x70e500 (bejeweled3): IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN at its place with the panel's 400 taken off
void ZenBoard::DrawBottomWidget(Graphics* g)
{
	if (IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN != nullptr)
		g->DrawImage(IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN, Scaled(PANEL_MIDDLE_X - 400 + GetImageOffsetX(IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN_ID)),
			Scaled(GetImageOffsetY(IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN_ID)));
}

}  // namespace Sexy
