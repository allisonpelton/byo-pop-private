// Sexy::MoveLimitBoard: see the header
#include "MoveLimitBoard.h"

#include "Font.h"
#include "Graphics.h"
#include "Res.h"

#include <cstdio>

#include "Bej3App.h"

namespace Sexy {

namespace {

// the exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

}  // namespace

// slot 135 (0x21c), 0x6e0170 (bejeweled3): the setting Moves (atoi) is the number of moves and the moves left; +0x3250 is put to the float -10.0 (the bounce of
// the moves counter's picture), and the quest's new game (0x6ddc70) follows
void MoveLimitBoard::NewGame()
{
	mMoves = SettingInt("Moves");
	mMovesLeft = mMoves;
	QuestBoard::NewGame();
}

// slot 22 (0x58), 0x6e00e0 (bejeweled3): the quest's update, then (the counter's bounce at +0x3250, which is drawn only, is left out) when the board is at rest with
// no moves left and the game's flag (+0xd8c) is not set, the game ends (slot 161 with 1)
void MoveLimitBoard::Update()
{
	QuestBoard::Update();
	if (IsSettled() && mMovesLeft == 0 && !mGameOver)
		NoMoreMoves(true);
}

// slot 191 (0x2fc), 0x6c5a60 (bejeweled3): a swap found a match: a move is used, then the goal's slot 24 (0x60), which the goals made so far do not use
void MoveLimitBoard::OnSwapMatched(const SwapRecord& theSwap)
{
	(void)theSwap;
	mMovesLeft--;
}

// slot 180 (0x2d0), 0x6cfb50 (bejeweled3): a hypercube's group is made: with a colour (not -1) a move is used; then the goal's slot 23 (0x5c) and Board's (0x74db70)
void MoveLimitBoard::MakeHypercubeGroup(Piece* theCube, int theColor)
{
	if (theColor != -1)
		mMovesLeft--;
	QuestBoard::MakeHypercubeGroup(theCube, theColor);
}

// slot 98 (0x188), 0x6c6500 (bejeweled3): no hint with no moves left, else Board's (0x73d4f0)
bool MoveLimitBoard::HintAllowed() const
{
	if (mMovesLeft == 0)
		return false;
	return Board::HintAllowed();
}

// slot 240 (0x6cba10, DrawBottomFrame) writes "Moves Left: %i" (string 0x12d) in the middle of the frame under the grid; slots 47, 242 (0x6d2c30) and 250 (0x6d5a00) draw the
// move counter and its bounce. port: the text and the goal's count (the gems cleared over the goal's size) are written plainly beside the score, in the font of Diamond Mine's clock
void MoveLimitBoard::DrawModeOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	char aText[64];
	snprintf(aText, sizeof(aText), "Moves Left: %d", mMovesLeft);
	g->WriteString(aText, Scaled(250), Scaled(330), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "%d / %d", mLevelProgress, GetLevelMax());
	g->WriteString(aText, Scaled(250), Scaled(390), 0, 0, true, 0, -1, -1);
}

}  // namespace Sexy
