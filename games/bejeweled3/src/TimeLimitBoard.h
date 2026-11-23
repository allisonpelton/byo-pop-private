// Sexy::TimeLimitBoard (bejeweled3, vftable 0x86a234, 0x3258 bytes before Diamond Mine's own, constructor 0x6dd3b0) over Sexy::QuestBoard: the Quest boards that have a clock
// (quest.cfg's Challenge "Time Limit"; Diamond Mine is one too). The clock counts hundredths of a second from the quest's setting Time. See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class TimeLimitBoard : public QuestBoard {
public:
	// 0x73bb80: the clock counts in hundredths; it starts after this many game ticks (the first 250 are the board's intro)
	enum { CLOCK_START_TICKS = 250, TICKS_PER_SECOND = 100 };

	int mTimeSeconds = 90;                  // +0x3248: "Time", the seconds the clock runs (a bonus' seconds are added to it)
	int mTimeBonus = 10;                    // +0x324c: "TimeBonus" (10 until the settings are read: 0x6dd3b0)
	int mTimePenalty = 3000;                // +0x3250: "TimePenalty" (3000 until the settings are read: 0x6dd3b0)
	bool mTimeUp = false;                   // +0xe28: the time ran out and the end is asked for
	bool IsTimeUp() const override { return mTimeUp; }   // the hint boxes read +0xe28 (Board::CanShowHint)

	// slot 135 (0x6e5030): Time, TimeBonus and TimePenalty, then the quest's (0x6ddc70)
	void NewGame() override;
	void UpdateTick() override;                                                // slot 234 (0x6e5660) with the time's end (slot 216, 0x73b890)
	int GetTimeLimit() const override { return mTimeSeconds; }                // slot 104 (0x6c4a30)
	bool CanPause() const override { return true; }                           // slot 107 (0x6c4a20; DigBoard shares it)
	// slot 187 (0x2ec): the hundredths of a second left (Diamond Mine's own, 0x736b50, caps it)
	virtual int GetTimeLeft() const { return GetClockTimeLeft(); }
	// 0x73bb80 (bejeweled3): the hundredths left by the game's ticks alone
	int GetClockTimeLeft() const;
	// 0x6e5030 (bejeweled3): the settings Time, TimeBonus and TimePenalty are read (atoi)
	void ReadTimeSettings();
	// slot 216 (0x73b890, bejeweled3): when the clock has run out (the time left is 0), nothing is going on (the board is at rest) and the game is on, the time's end is
	// asked for with the flag +0xe28 set: slot 161
	void CheckTimeUp();
};

}  // namespace Sexy
