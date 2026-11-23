// Sexy::QuestGoal and ClearGoal: see the header
#include "QuestGoal.h"

#include "ButterflyBoard.h"
#include "DiamondMineBoard.h"
#include "QuestBoard.h"

#include <SDL.h>

namespace Sexy {

// slot 53 (0xd4), 0x6ae510 (bejeweled3): the settings Gems, GemsPerLevel and GemsMax are read (atoi: a missing one reads as 0)
void ClearGoal::Load()
{
	mGems = mBoard->SettingInt("Gems");
	mGemsPerLevel = mBoard->SettingInt("GemsPerLevel");
	mGemsMax = mBoard->SettingInt("GemsMax");
}

// slot 22 (0x58), 0x6aa320 (bejeweled3): the level's size: the level's index (+0xe04) times GemsPerLevel plus Gems; with GemsMax at 1 or more, no more than it
int ClearGoal::GetLevelMax()
{
	const int aSize = mBoard->mLevelIndex * mGemsPerLevel + mGems;
	if (mGemsMax < 1)
		return aSize;
	return aSize < mGemsMax ? aSize : mGemsMax;
}

// slot 40 (0xa0), 0x6aa360 (bejeweled3): a gem is cleared: the score (+0xd24) and the progress (+0xe00) go up by 1
void ClearGoal::OnPieceCleared(Piece* thePiece)
{
	(void)thePiece;
	mBoard->mScorePoints++;
	mBoard->mLevelProgress++;
}

// 0x6ddc70 (bejeweled3), the goal's part of QuestBoard::NewGame: the setting Goal, upper case, is compared with the names in this order (the object's size in
// brackets): CLEAR GEMS (ClearGoal, 0x11c), WALL BLAST (0x6ab720, 0x260), GOLD (GoldGoal, 0x114), DETONATOR, ARSENAL, GOLD LINES (0x6afcd0, 0x940), GOLD LINES FLIP
// (0x6afe00, 0x944), GOLD YELLOW (0x6ae170), CLEAR BOMBS (0x6ae760), SCORE, FIRESTORM, PRESERVER (0x118), HIDDEN GEMS (0x11c), HIDDEN GRID (0x21c), SURVIVAL (0x118),
// CATCH BUTTERFLIES (ButterflyGoal, 0x114), HIDDEN OBJECTS (0x6afb60, 0x1f80) and DIG (0x6c4230, DigGoal, 0xb18); a later match replaces an earlier one, and no
// match leaves the board with no goal. port: ClearGoal, DigGoal and ButterflyGoal
QuestGoal* MakeQuestGoal(const std::string& theName, QuestBoard* theBoard)
{
	QuestGoal* aGoal = nullptr;
	if (theName == "CLEAR GEMS")
		aGoal = new ClearGoal();
	else if (theName == "DIG")
		aGoal = new DigGoal();
	else if (theName == "CATCH BUTTERFLIES")
		aGoal = new ButterflyGoal();
	else if (!theName.empty())
		SDL_Log("Quest: the goal \"%s\" is not made yet", theName.c_str());
	if (aGoal != nullptr)
		aGoal->mBoard = theBoard;
	return aGoal;
}

}  // namespace Sexy
