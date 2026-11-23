// Sexy::BalanceBoard: see the header
#include "BalanceBoard.h"

#include "Font.h"
#include "Graphics.h"
#include "Res.h"

#include <cmath>
#include <cstdio>

#include "Bej3App.h"

namespace Sexy {

namespace {

// the exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

}  // namespace

// slot 135 (0x21c), 0x6e4c80 (bejeweled3): the settings Gems, Balance, RollSpeedDivisor and CascadeThreshold are read; the scale, the counts and the display values
// (+0x3264 and +0x3268) are 0 and the effects (+0x3368 on) are cleared; the quest's new game (0x6ddc70) follows
void BalanceBoard::NewGame()
{
	mGemsPerColor = SettingInt("Gems");
	mBalanceScale = SettingInt("Balance");
	mRollSpeedDivisor = SettingInt("RollSpeedDivisor");
	mCascadeThreshold = SettingInt("CascadeThreshold");
	mBalance = 0.0f;
	mBlueCount = 0;
	mRedCount = 0;
	QuestBoard::NewGame();
}

// slot 143 (0x23c), 0x6d06a0 (bejeweled3): a gem is cleared: its colour (the one it had when it was matched, +0x21c, when it has none now) red counts a red gem while fewer than
// Gems are counted, blue the same; the score and the progress are the two counts together; then the goal's slot 40 (none) and Board's
void BalanceBoard::OnPieceCleared(Piece* thePiece)
{
	const int aColor = thePiece->mColor == -1 ? thePiece->mRememberedColor : thePiece->mColor;
	if (aColor == COLOR_RED && mGemsPerColor > mRedCount)
		mRedCount++;
	if (aColor == COLOR_BLUE && mGemsPerColor > mBlueCount)
		mBlueCount++;
	mScorePoints = mBlueCount + mRedCount;
	mLevelProgress = mBlueCount + mRedCount;
	QuestBoard::OnPieceCleared(thePiece);
}

// slot 22 (0x58), 0x6e3c90 (bejeweled3): the quest's update; with the scale past 1 either way the game ends (slot 161 with 1). Nothing more when the game's ticks did not go
// up (a held update). While the hint is allowed (slot 98) and the level is not full, the scale rolls: the difference of the blue and red counts over Balance, held to 1 in
// size, over RollSpeedDivisor is added to it. port: the display values that follow the counts by at most 0.02 (+0x3264 and +0x3268) and the arrow's effects are drawn only
// and are left out
void BalanceBoard::Update()
{
	const int aTicksBefore = mGameTicks;
	QuestBoard::Update();
	if (std::fabs(mBalance) > 1.0f)
		NoMoreMoves(true);
	if (mGameTicks == aTicksBefore)
		return;
	if (HintAllowed() && GetLevelMax() > mLevelProgress) {
		float aRoll = (float)(mBlueCount - mRedCount) / (float)mBalanceScale;
		if (std::fabs(aRoll) > 1.0f)
			aRoll = aRoll / std::fabs(aRoll);
		aRoll = aRoll / (float)mRollSpeedDivisor;
		mBalance += aRoll;
	}
}

// slots 220, 238, 239, 244 and 248 draw the scale, its arrow and the two counts. port: the scale, the counts and the level's progress are written plainly beside the score
void BalanceBoard::DrawModeOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	char aText[64];
	snprintf(aText, sizeof(aText), "Balance: %d%%", (int)(mBalance * 100.0f));
	g->WriteString(aText, Scaled(250), Scaled(330), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "Red %d  Blue %d  of %d", mRedCount, mBlueCount, mGemsPerColor);
	g->WriteString(aText, Scaled(250), Scaled(390), 0, 0, true, 0, -1, -1);
}

}  // namespace Sexy
