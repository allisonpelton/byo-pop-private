// Sexy::InfernoBoard, the port's IceStormBoard (bejeweled3, vftable 0x8685ec, 268 slots, its eight columns at +0x3d28, made by
// 0x6e2d90 over Sexy::QuestBoard; the mode's start 0x774060 picks it when the quest's setting Challenge reads INFERNO, the secret quest 2 of
// secret.cfg, "Ice Storm"): the secret mode where eight columns of ice rise from the bottom of the grid. Matches push them down (each cleared
// gem a little, a gem that a special gem cleared more), a match whose gems all stand in one column crushes that column's ice, and a column
// that has reached the top cracks and ends the game once its crack has run its seconds. The ice that has been pushed back fills a
// bar; each time it is full the score's multiplier rises. See docs/NOTES.md, "Ice Storm", for the slots it changes (each function here says
// its slot and address).
#pragma once

#include "QuestBoard.h"
#include "CurvedVal.h"

#include <string>
#include <vector>

namespace Sexy {

class IceStormBoard : public QuestBoard {
public:
	enum { COLUMN_COUNT = 8 };
	// the counter 0x6db1a0 adds one to (0x73d170, end dialog string 0x111): the columns crushed
	enum { COUNTER_COLUMNS_CRUSHED = 0x23 };
	// 0x6e2670 puts the grace (+0x3264) to this: the updates after a game starts before the ice moves or a column is made
	enum { GRACE_TICKS = 0x96 };
	// 0x6e08a0 and 0x6e0650: the updates a made column waits before it rises, and the least left when every column is waiting
	enum { SPAWN_DELAY_TICKS = 0x32, SPAWN_DELAY_WHEN_ALL_WAIT = 0x14 };
	// 0x6c5d30: the next column is made at the latest this many updates after the last one (the first is due 0x19 after a reset)
	enum { SPAWN_DEADLINE_FIRST_TICKS = 0x19, SPAWN_DEADLINE_TICKS = 0x4b };
	// 0x6e5850: the pause the multiplier's rise holds the ice for, and the longest it waits for the columns' reprieve to end
	enum { MULTIPLIER_PAUSE_TICKS = 200, MULTIPLIER_PAUSE_EXTRA_TICKS = 300 };
	// 0x6e5850: the pause of a column before its reprieve push starts is this, plus this for each column further on
	enum { REPRIEVE_DELAY_BASE_TICKS = 0x50, REPRIEVE_DELAY_STEP_TICKS = 5 };
	// 0x6e0cc0: the ice's top is at this position (rows from the bottom) and the lowest it goes is this
	static constexpr float POSITION_TOP = 8.0f;
	static constexpr float POSITION_LOWEST = -0.2f;
	static constexpr float POSITION_HELD_ONCE_REACHED = 0.5f;   // a column that has reached this does not go below it again unless it is crushed
	static constexpr float PUSH_DECAY_PER_TICK = 0.003f;
	// 0x6e0450: the death's curve (0 to 1 over 100 updates) and 0x6e5850's ramp of a column after the multiplier rose
	static const char* const DEATH_CURVE;
	static const char* const RAMP_CURVE;
	// the rows of levels the settings have at most (Level1 to Level15)
	enum { LEVEL_ROWS_MAX = 15 };

	// one row of the settings Level1 to Level15: "doubleCols, colCount, fireSpeed, seconds, crackSpeedMod[, curve]" (the row at +0x3d1c, 0x88 bytes)
	struct LevelRow {
		int mDoubleColumns = 0;           // +0
		int mColumnCount = 0;             // +4
		float mFireSpeed = 0.0f;          // +8: already multiplied by FireSpeedMult
		int mDurationTicks = 0;           // +0xc: the seconds times 100
		float mCrackSpeedMod = 1.0f;      // +0x10
		std::string mCurve;               // +0x18: how the stage's fire speed goes from the last row's to this one's (empty: at once)
	};

	// one column of ice (0x348 bytes at +0x3d28 + 0x348 * column, 0x6e0cc0 steps it)
	struct Column {
		double mFreeze = 0.0;             // +0x00: the strength of the freeze that slows a rise after the ice was pushed back
		float mPreviousPosition = 0.0f;   // +0x08
		float mPosition = 0.0f;           // +0x0c: from 0 (the bottom) to 8 (the top)
		float mPushStrength = 0.0f;       // +0x10: what matches have pushed it down by, which decays
		float mSpeedDeviation = 0.0f;     // +0x14: the column's own share of MaxRandFireSpeedColDelta, from 0 to 1
		float mPendingIceRemoved = 0.0f;  // +0x18: set when the column is crushed: how much ice that took off the bar (at least 1)
		int mOwner = 0;                   // +0x1c: the column of the ice's left side (a double column's two have the same owner)
		int mPlannedAnchor = 0;           // +0x20: the owner the layout plan wants it to have
		int mSpan = 1;                    // +0x24: 1 or 2 columns wide
		int mSpawnDelay = 0;              // +0x28: updates left before it rises
		int mCrackTicks = -1;             // +0x34: updates the ice has stood at the top (-1: not there)
		int mReprieveDelay = 0;           // +0x38: updates left before the multiplier's reprieve pushes it down
		int mRampDelay = 0;               // +0x3c: updates left before the ramp (the slow start after a multiplier) goes on
		int mPlannedWidth = 1;            // +0x40
		bool mReachedHalf = false;        // +0x44
		bool mActive = false;             // +0x45
		bool mCrushed = false;            // +0x46
		double mCrackSpeedMod = 1.0;      // +0x48
		CurvedVal mRamp;                  // +0x218
		int mCrushedTicks = 0;            // port: updates since the column was crushed (the exe waits for its crush effect to finish)
	};

	// the settings of the quest (NewGame, 0x6e6a10 reads them)
	float mMatchPushStrength = 0.0f;               // "MatchPushStr": what a cleared gem pushes its column's ice down by
	float mSpecialGemPushMod = 1.0f;               // "SpecialGemPushMod": a gem cleared by a special gem pushes this times as hard
	int mSecondsUntilLose = 10;                    // +0x3cbc: "SecondsUntilLose" (10 until the settings are read)
	double mDoubleEdgeMult = 1.0;                  // +0x3678
	float mFireSpeedMult = 1.0f;                   // +0x3260
	int mColumnDestroyBonus = 0;                   // +0x36b0: "ColDestroyBonus"
	std::vector<int> mColumnCountBonus;            // +0x3644
	std::vector<float> mMultiplierIceReq;          // +0x365c: "MultiplierIceReq", each times 8
	double mMaxRandomFireSpeedColumnDelta = 0.0;   // +0x3690
	double mDoubleColumnSpeedMult = 1.0;           // +0x325c
	double mFreezeDurationPerNegativeStrength = 0.0;   // +0x3d00
	double mFreezeMax = 0.0;                       // +0x3d08
	CurvedVal mRowFireSpeed;                       // +0x3850
	CurvedVal mColumnDistribution;                 // +0x39a0
	CurvedVal mReprieveStrengthVsRow;              // +0x3a10
	CurvedVal mColumnComboCoolDownVsCount;         // +0x35a0
	std::vector<LevelRow> mLevelRows;              // +0x3d1c

	// the game's state
	Column mColumns[COLUMN_COUNT];                 // +0x3d28
	int mTicks = 0;                                // +0x3274: the updates the ice has run
	int mGraceTicks = GRACE_TICKS;                 // +0x3264
	int mStage = 0;                                // +0x3ccc: the row of the settings in play
	int mStageStartTick = 0;                       // +0x3cd4
	int mStageDurationTicks = 1;                   // +0x3cd0
	int mMaxActiveColumns = 0;                     // +0x3278: this stage's column count
	int mDoubleColumns = 0;                        // +0x327c: this stage's double columns
	int mStartColumns = 0;                         // +0x3280: the setting StartColCount
	float mStageFireSpeed = 0.0f;                  // +0x324c: this stage's row's fire speed times FireSpeedMult
	float mPreviousStageFireSpeed = 0.0f;          // the row before's
	double mStageCrackSpeedMod = 1.0;              // +0x3688: given to the columns made now
	CurvedVal mStageCurve;                         // +0x38c0: the stage's fire speed's way from the last stage's to this one's
	float mIceRemoved = 0.0f;                      // +0x326c: the ice pushed back and crushed since the multiplier last rose
	float mPreviousIceRemoved = 0.0f;              // +0x3268
	float mMultiplierIceThreshold = 10.0f;         // +0x3270: the ice (in rows) the bar needs
	int mProgress = 0;                             // +0x369c: the ice times 10000
	int mProgressTarget = 0;                       // +0x36a0: the threshold times 10000
	bool mPaused = false;                          // +0x36a4: the ice holds still while the multiplier's reprieve runs
	int mPauseEndTick = 0;                         // +0x36a8
	int mSpawnDeadlineTick = SPAWN_DEADLINE_FIRST_TICKS;   // +0x3610
	int mLoseColumn = -1;                          // +0x3cc8: the column whose crack ran out (-1: none yet)
	std::vector<int> mVerticalRunPieceIds;         // +0x3620: the ids of the pieces of the runs that stand in one column
	int mColumnPoints = 0;                         // +0x3668: the points of the column bonuses
	int mColumnBonusTotal = 0;                     // +0x366c: ColDestroyBonus times the multiplier of each column crushed
	int mBestChain = 0;                            // +0x3670: the most columns crushed in a chain (shown on the end screen)
	int mChainCount = 0;                           // +0x36c8
	int mChainWindowTicks = 0;                     // +0x36c4
	int mChainLastTick = 0;                        // +0x36bc
	CurvedVal mDeathCurve;                         // +0x3300: runs 0 to 1 once the game is lost, then the game ends
	bool mDeathStarted = false;                    // port: the death's curve was set (the exe asks whether its value is above 0)

	IceStormBoard();   // 0x6e2d90 (bejeweled3)

	const char* GetModeName() const override { return "Icestorm"; }       // slot 89 (0x6c78a0): the music's and the log's name
	void ResetGame() override;                                              // slot 85 (0x6e2670)
	void NewGame() override;                                                // slot 135 (0x6e6a10)
	void UpdateTick() override;                                             // slot 234 (0x6e5850)
	void Update() override;                                                 // slot 22 (0x6e63c0)
	float GetLevelFactor() const override { return mIsSecretMode ? 2.0f : 1.0f; }   // slot 122 (0x6c5da0)
	int GetGridY() const override { return mIsSecretMode ? 0x28 + GRID_Y : GRID_Y; }   // slot 172 (0x6c5ce0): +0x3284 is 0x28 in a secret mode
	int GetHintWaitSeconds() const override { return mIsSecretMode ? 5 : 15; }   // slot 106 (0x6c5cc0)
	void OnRunFound(const std::vector<Piece*>& thePieces) override;         // slot 211 (0x6e2250)
	void OnPieceCleared(Piece* thePiece) override;                          // slot 143 (0x6dd580)
	void MakeFlameGem(Piece* thePiece) override;                            // slot 166 (0x6caf30)
	void NoMoreMoves(bool theAnnounce) override;                            // slot 161 (0x6e0450): the death begins
	ClassicEndLevelDialog* MakeEndLevelDialog() override;                   // slot 257 (0x6d3180)
	void DrawModeOverlay(Graphics* g) override;                             // slot 43 (0x6da820)

	LevelRow StageRow(int theStage) const;   // 0x6c6600 (bejeweled3)
	void LoadStage(int theStage);            // 0x6c6600 (bejeweled3): the stage's row is read
	double StageFireSpeed();                 // 0x6d3df0 (bejeweled3)
	double CrackFraction(const Column& theColumn) const;   // 0x6ce540 (bejeweled3)
	void LimitSpawnDeadline();               // 0x6c5d30 (bejeweled3)
	// 0x6e5690 (bejeweled3): the columns are planned as singles or doubles for the stage
	void PlanLayout();
	// 0x6e08a0 (bejeweled3): one more column of ice is made (true when it was)
	bool SpawnColumn();
	// 0x6e0650 (bejeweled3): a column is made or ended: the anchor, how wide it is and which way
	void SetColumnActive(int theColumn, int theWidth, bool theActive);
	// 0x6e0cc0 (bejeweled3): the ice moves one step
	void StepColumns(bool theMove, bool theMayLose, bool theMaySpawn);
	// 0x6db1a0 (bejeweled3): the ice of a column is crushed (true when it was); 0x6d6d60 gives the points
	bool CrushColumn(int theOwner, int theRow);
	// 0x6d6d60 (bejeweled3): the points of a crushed column and the chain
	void ScoreCrushedColumn();
	// 0x6ceba0 (bejeweled3): how strongly the reprieve pushes a column that is high
	float ReprieveStrength(int theColumn);
	// 0x6e5850 (bejeweled3): the multiplier rises
	void RaiseMultiplier();
	// 0x6c4fb0 (bejeweled3): the made columns, singles and doubles, that are not crushed
	void CountActiveColumns(int& theSingles, int& theDoubles) const;
	// 0x6c6870 (bejeweled3): the ice the bar needs for a multiplier
	float MultiplierThreshold() const;
	// 0x6c6520 (bejeweled3): whether a match group (a hypercube's or a star's) is at work on a column
	bool ColumnHasLiveGroup(int theColumn) const;
};

}  // namespace Sexy
