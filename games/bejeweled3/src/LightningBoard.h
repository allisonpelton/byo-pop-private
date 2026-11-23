// Sexy::SpeedBoard (bejeweled3, vftable 0x8665c4, 0x3480 bytes, made by 0x718090 from the mode's start 0x773440 over Sexy::Board): the
// game screen of Lightning mode. The exe's own names are "Speed" (the mode's name 0x7144a0 and its save file "speed.sav", its quest file
// speed.cfg) and "Lightning" (the board's Title and the leaderboard's); the port's class is LightningBoard. It derives from Board directly
// and changes 40 slots of Board; the ones that a game of the port reads are overridden here, each saying its slot and address. A game lasts 60
// seconds of the board's own clock (100 game ticks a second): time gems (flag 0x20000, +0x244 seconds) that are matched put their seconds
// to the Last Hurrah's account (+0x30ac) and when the 60 seconds are out a Last Hurrah of that many seconds is played at a higher
// multiplier, after which the game ends and every special gem left goes off. See docs/NOTES.md, "Zen and Lightning".
#pragma once

#include "Board.h"
#include "CurvedVal.h"
#include "X86Math.h"

namespace Sexy {

// The exe's chance counter (0x78 bytes: a CurvedVal of the setting's curve string, then two floats, stepped by 0x41c090): how likely each
// new gem is to be a time gem of one length. The counter counts the game ticks (scaled) since the last such gem and the chance rises with
// it along the curve
struct TimeGemChance {
	CurvedVal mCurve;     // +0: the setting "5SecChanceCurve", "10SecChanceCurve" or "15SecChanceCurve"
	float mCounter = 0.0f;   // +0x70: the time since the last gem of the length, in the unit of the steps it is given
	float mChance = 0.0f;    // +0x74: the chance one new gem has now
	// 0x41c090 (bejeweled3): the counter goes up by theStep; with the curve's value g above 0 the chance is g times the 1.2 power of
	// (1.5 g counter) held between 0.2 and 2.5, else 0
	void Step(float theStep);
};

class LightningBoard : public Board {
public:
	// the seconds a time gem can be worth: the gem is of the first length whose draw wins, the 10 first (0x716c60)
	enum { SHORT_GEM_SECONDS = 5, LONG_GEM_SECONDS = 10 };
	// the counter 0x715360 adds one to (0x73d170) for each time gem matched: the end dialog's count of them
	enum { COUNTER_TIME_GEMS = 0x22 };

	int mLastHurrahPoints = 0;      // +0x3080: the score as the Last Hurrah ended (0x719350)
	int mRoundNumber = 0;           // +0x3084: the levels cleared in the points-goal form of the mode (slot 156, 0x714280); the shipped speed.cfg has no goal, so it stays 0
	int mPointsGoalStart = 0;       // +0x30f8: the setting "PointsGoalStart" (the shipped speed.cfg has none, 0)
	int mPointsGoalPerLevel = 0;    // +0x30fc: "AddPointsGoalPerLevel" (0)
	int mTimeStart = 0;             // +0x30ec: "TimeStart" (60: the seconds; slot 104's own 60 is the constant the code reads)
	bool mUsePoints = false;        // +0x3094: "UsePM" (0)
	bool mHasGoalAndTime = false;   // +0x3095: the goal and the time are both above 0 (0 with the shipped file)
	bool mTimeUpShown = false;      // +0x3096: the Last Hurrah is over and the game is ending: the specials go off one by one
	int mEndingAge = 0;             // +0x30a4: updates since mTimeUpShown was set
	int mTicksPlayed = 0;           // +0x30a8: the game ticks the board has run since the game began
	int mBonusSeconds = 0;          // +0x30ac: the seconds of the time gems matched (the Last Hurrah's length, until it begins)
	int mBonusSecondsTotal = 0;     // +0x30b0: the seconds of all the time gems matched in the game (the end dialog's)
	int mGemSpacing = 0;            // +0x30e4: 20 at the first update, then every 400 game ticks it is lowered by a draw (not below 5); nothing translated reads it
	bool mSpecialsStarted = false;  // port: the exe's curve at +0x33e0 that 0x719350 sets when the specials begin to go off and reads (above 0) before the end: set here instead
	int mStartDelay = 0;            // +0x30b8: 250 at a new game (500 at a new level): counts down in the points-goal form's penalty (stays above 0 here)
	int mTimeLimitTicks = 6000;     // +0x30f4: the round's length in game ticks; the Last Hurrah puts its seconds times 100 here
	float mClock = 0.0f;            // +0x3284: the board's clock, in game ticks: each tick adds the scale below (or the override)
	float mClockScaleOverride = 0.0f;   // +0x3288: when not 0 the clock's step instead of the speed of the last seconds
	float mTimeStep = 0.0f;         // +0x327c: the setting "TimeStep": each game tick steps the chance counters by it over 100
	float mLevelTimeStep = 0.0f;    // +0x3280: the setting "LevelTimeStep": the Last Hurrah's start steps them by it
	bool mEnding = false;           // +0xe28: the time is out and the end is being played (slot 185 makes no time gems then)
	bool IsTimeUp() const override { return mEnding; }   // the hint boxes read +0xe28 (Board::CanShowHint)
	TimeGemChance mChance5;         // +0x3108, its counter and chance at +0x3178 and +0x317c
	TimeGemChance mChance10;        // +0x3180, at +0x31f0 and +0x31f4
	TimeGemChance mChance15;        // +0x31f8, at +0x3268 and +0x326c (stepped, never read by 0x716c60)
	bool mLastHurrah = false;       // port: the Last Hurrah is being played (the exe's curve at +0x33e0 is above 0 for its text, 0x719350)
	std::string mHudLabel;          // +0x3458: the label the four HUD animations were last given (0x718400)
	double mPenalty = 0.0;          // +0x30d0: the level bar's drain in the points-goal form (0 with the shipped file)

	LightningBoard();   // 0x718090 (bejeweled3): the Title "Lightning"; SpeedBoard's own 0x7144a0 says the mode is "Speed"

	const char* GetModeName() const override { return "Speed"; }   // slot 89 (0x164), 0x7144a0: the song and the log's mode
	void ResetGame() override;                                      // slot 85 (0x154), 0x718500
	void Update() override;                                         // slot 22 (0x58), 0x7173f0
	int GetGridY() const override { return 0x82; }                  // slot 172 (0x2b0), 0x714100: +0x2c50 (0) and 0x82
	float GetLevelFactor() const override { return 5.0f; }          // slot 122 (0x1e8), 0x714090 (slot 123's 0x714080 is 5.6666665)
	int GetTimeLimit() const override { return 0x3c; }              // slot 104 (0x1a0), 0x7140f0
	int GetLevelMax() const override { return mPointsGoalPerLevel * mRoundNumber + mPointsGoalStart; }   // slot 154 (0x268), 0x7140d0
	int GetLevelProgress() const override { return mLevelProgress - Ftol(mPenalty); }                    // slot 155 (0x26c), 0x7140b0
	float GetBarTarget() override;                                  // slot 188 (0x2f0), 0x716f70
	bool OnFillAccepted(std::vector<Piece*>& theNewPieces) override;   // slot 185 (0x2e4), 0x716c60
	void OnPieceCleared(Piece* thePiece) override;                  // slot 143 (0x23c), 0x715360
	void NoMoreMoves(bool theAnnounce) override;                    // slot 161 (0x284), 0x719350
	ClassicEndLevelDialog* MakeEndLevelDialog() override;          // slot 233 (0x3a4), 0x716ab0: the Lightning end dialog

	void DrawScreenFrame(Graphics* g) override;   // slot 238 (0x714840) in place of Classic's frames and level bar
	void DrawScoreText(Graphics* g) override;     // slot 255 (0x714660)
	void DrawModeOverlay(Graphics* g) override;   // slot 43 (0x716ef0), which calls 0x715cb0
	void DrawBottomWidget(Graphics* g) override;  // slot 251 with the mode's picture (0x741440)

	// 0x718400 (bejeweled3): the four HUD animations (the bottom, the middle, the top and the mega gem's) go to the label over theTime
	// updates, the top one only to "death" or else to "unpowered"; the label is kept
	void StartHudLabel(const char* theLabel, int theTime);

	// slot 218 (0x714130) and 0x73d9f0 (bejeweled3): the red warning of the last seconds without an account, and its alpha
	bool IsWarning() const;
	int GetWarningAlpha(float theBoardAlpha) const;

	// slot 187 (0x2ec), 0x713fa0: the game ticks left of the round (the Last Hurrah's, once it began), or -1 where the mode has no clock
	int GetTimeLeft() const;

private:
	static int SettingInt(const std::map<std::string, std::string>& theSettings, const char* theName);
	static double SettingDouble(const std::map<std::string, std::string>& theSettings, const char* theName);
};

}  // namespace Sexy
