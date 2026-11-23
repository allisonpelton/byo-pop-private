// Sexy::TimeLimitBoard: see the header
#include "TimeLimitBoard.h"

#include <algorithm>

namespace Sexy {

// 0x6e5030 (bejeweled3): the settings Time, TimeBonus and TimePenalty
void TimeLimitBoard::ReadTimeSettings()
{
	mTimeSeconds = SettingInt("Time");
	mTimeBonus = SettingInt("TimeBonus");
	mTimePenalty = SettingInt("TimePenalty");
}

// slot 135 (0x21c), 0x6e5030 (bejeweled3): the time's settings, then the quest's new game
void TimeLimitBoard::NewGame()
{
	ReadTimeSettings();
	QuestBoard::NewGame();
}

// slot 234 (0x3a8), 0x6e5660 (the quest's step, Board's) with the time's end
void TimeLimitBoard::UpdateTick()
{
	QuestBoard::UpdateTick();
	CheckTimeUp();
}

void TimeLimitBoard::CheckTimeUp()
{
	if (GetTimeLimit() > 0 && !mTimeUp && mGameOverTicks == 0 && GetTimeLeft() <= 0 && IsSettled()) {
		mTimeUp = true;
		NoMoreMoves(true);
	}
}

// 0x73bb80 (bejeweled3): the clock's hundredths left: the limit times 100 less the game's ticks past the first 250 (0 before), never below 0
int TimeLimitBoard::GetClockTimeLeft() const
{
	const int aLimit = mTimeSeconds * TICKS_PER_SECOND;
	const int aPassed = std::max(0, mGameTicks - (int)CLOCK_START_TICKS);
	return std::max(0, aLimit - aPassed);
}

}  // namespace Sexy
