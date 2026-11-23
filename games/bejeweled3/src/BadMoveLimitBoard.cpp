// Sexy::BadMoveLimitBoard: see the header
#include "BadMoveLimitBoard.h"

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

// slot 135 (0x21c), 0x6e4f60 (bejeweled3): the setting BadMovesAllowed is read and the count used is 0; the quest's new game (0x6ddc70) follows
void BadMoveLimitBoard::NewGame()
{
	mBadMovesAllowed = SettingInt("BadMovesAllowed");
	mBadMovesUsed = 0;
	QuestBoard::NewGame();
}

// slot 22 (0x58), 0x6e4f30 (bejeweled3): the quest's update, then with the board at rest and all the bad moves used the game ends (slot 161 with 1)
void BadMoveLimitBoard::Update()
{
	QuestBoard::Update();
	if (IsSettled() && mBadMovesUsed == mBadMovesAllowed && !mGameOver)
		NoMoreMoves(true);
}

// slot 192 (0x300), 0x6c6050 (bejeweled3): a swap found no match (0x759ba0 calls it where the swap goes back): a bad move is used
void BadMoveLimitBoard::OnSwapFailed(const SwapRecord& theSwap)
{
	(void)theSwap;
	mBadMovesUsed++;
}

// slot 98 (0x188), 0x6c6c40 (bejeweled3): no hint once all the bad moves are used, else Board's (0x73d4f0)
bool BadMoveLimitBoard::HintAllowed() const
{
	if (mBadMovesUsed == mBadMovesAllowed)
		return false;
	return Board::HintAllowed();
}

// slot 124 (0x6ccb70) makes the text of the bad moves left (the setting less the count used) for the frame under the grid (slot 240). port: the text is written plainly
// beside the score, with the goal's count as Move Limit's overlay does
void BadMoveLimitBoard::DrawModeOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	char aText[64];
	snprintf(aText, sizeof(aText), "Bad Moves Left: %d", mBadMovesAllowed - mBadMovesUsed);
	g->WriteString(aText, Scaled(250), Scaled(330), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "%d / %d", mLevelProgress, GetLevelMax());
	g->WriteString(aText, Scaled(250), Scaled(390), 0, 0, true, 0, -1, -1);
}

}  // namespace Sexy
