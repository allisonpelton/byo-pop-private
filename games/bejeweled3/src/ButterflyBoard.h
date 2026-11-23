// Sexy::ButterflyBoard (bejeweled3, vftable 0x867d0c, 0x3388 bytes, made by 0x6dcdf0 over Sexy::QuestBoard; the mode's start 0x774060 picks it when
// the quest's setting Challenge reads BUTTERFLIES or BUTTERFLY, the secret quest 1 of secret.cfg, "Butterflies"): the secret mode where gems
// grow wings. Butterfly gems (the bit 0x80) rise one row each move; a spider over the top row walks to the nearest one; a butterfly that
// reaches the top is caught and the spider's arrival ends the game. Matching a butterfly frees it for points. See docs/NOTES.md,
// "Butterflies and Poker", for the slots it changes (each function here says its slot and address).
#pragma once

#include "QuestBoard.h"
#include "CurvedVal.h"

namespace Sexy {

// vftable 0x86a90c (bejeweled3), the goal "Catch Butterflies" of the Butterflies quests of quest.cfg (made by QuestBoard::CreateGoal, 0x6ddc70): free a number of butterflies
// (the setting Butterflies). A secret mode has no goal object, so its board counts the butterflies freed itself
class ButterflyGoal : public QuestGoal {
public:
	int mButterflies = 0;   // +0x110: the setting Butterflies, the butterflies to free

	void Load() override;                              // slot 53 (0xd4), 0x6ae830
	int GetLevelMax() override;                        // slot 22 (0x58), 0x6aa420
	void OnPieceCleared(Piece* thePiece) override;     // slot 40 (0xa0), 0x6ab2f0
};

class ButterflyBoard : public QuestBoard {
public:
	// the counters 0x6d03d0 adds to (0x73d170, end dialog strings 0x784400): the butterflies freed and the best number freed by one move
	enum { COUNTER_BUTTERFLIES_FREED = 0x1c, COUNTER_BEST_COMBO = 0x1d };
	// 0x6e82a0 compares the game's ticks (+0xe38) with it: before it the spider jumps to a butterfly's column and looks 8 rows down, not 5
	enum { SPIDER_EARLY_TICKS = 0x96 };

	int mMoveCountdown = 0;              // +0x3248: the setting MoveCountdown: the moves a butterfly waits before it climbs, at level 0
	int mDropCountdownSetting = 0;       // +0x324c: the setting DropCountdown: the moves between two drops of butterflies, at level 0
	int mSpawnCountStart = 0;            // +0x325c: the setting SpawnCountStart (0 reads as 1)
	float mDropCountdownPerLevel = 0.0f; // +0x3264: the setting DropCountdownPerLevel
	float mMoveCountdownPerLevel = 0.0f; // +0x3268: the setting MoveCountdownPerLevel
	float mSpawnCountMax = 0.0f;         // +0x326c: the setting SpawnCountMax (0 reads as 1)
	float mSpawnCountPerLevel = 0.0f;    // +0x3270: the setting SpawnCountPerLevel
	float mSideSpawnChance = 0.0f;       // +0x3278: the setting SideSpawnChance, which rises by mSideSpawnChancePerLevel at each drop up to mSideSpawnChanceMax
	float mSideSpawnChancePerLevel = 0.0f;   // +0x327c: "SideSpawnChancePerLevel"
	float mSideSpawnChanceMax = 0.0f;    // +0x3280: "SideSpawnChanceMax"
	int mDropCountdown = 0;              // +0x3258: the moves left before the next drop of butterflies (slot 191 counts it down)
	bool mSpawnPending = false;          // +0x3260: a hypercube's group has gone (slot 180): the next quiet update drops butterflies
	float mSpawnCount = 0.0f;            // +0x3294: the butterflies one drop is worth (it starts at SpawnCountStart and rises to mSpawnCountMax)
	float mSpawnCredit = 0.0f;           // +0x3274: the butterflies owed; each one made takes 1 of it
	float mSpiderX = 3.0f;               // +0x3288: the column the spider is over (a fraction while it walks)
	float mSpiderFrom = 0.0f;            // +0x328c: where the walk began
	float mSpiderTo = 0.0f;              // +0x3290: where it ends
	CurvedVal mSpiderCurve;              // +0x32a0: the walk (0 to 1, its step scaled by the distance and the target's row)
	int mGrabTick = -1;                  // +0x3250: the game tick the spider grabbed a caught butterfly at (-1: not yet)
	bool mScoringButterfly = false;      // +0x3298: set while the points of a freed butterfly are being added (the HUD's combo text reads it)

	ButterflyBoard();   // 0x6dcdf0 (bejeweled3)

	const char* GetModeName() const override { return "Butterflies"; }      // slot 89 (0x6c78d0): the music's and the log's name; its save is "butterfly.sav"
	void ResetGame() override;                                             // slot 85 (0x6ceda0)
	void NewGame() override;                                               // slot 135 (0x6e3660)
	void UpdateGame() override;                                            // slot 231 (0x6e82a0)
	bool HintAllowed() const override;                                     // slot 98 (0x6c6bd0)
	bool IsSlowMode() const override { return true; }                      // slot 113 (0x6c5320)
	float GetLevelFactor() const override { return mIsSecretMode ? 5.0f : 1.0f; }   // slot 122 (0x6c5340): 0x880e2c is 5.0
	int GetGridY() const override { return 0x82; }                         // slot 172 (0x6c52f0): +0x2c50 (0) and 0x82
	void OnPieceCleared(Piece* thePiece) override;                         // slot 143 (0x6d03d0)
	void MakeHypercubeGroup(Piece* theCube, int theColor) override;        // slot 180 (0x6d0390)
	void OnSwapMatched(const SwapRecord& theSwap) override;                // slot 191 (0x6d7f40)
	void OnQuestStart() override;                                          // slot 256 (0x6dcce0)
	void NoMoreMoves(bool theAnnounce) override;                           // slot 161 (0x6dcd50, which jumps to 0x6d8e30): the end of the game
	ClassicEndLevelDialog* MakeEndLevelDialog() override;                  // slot 257 (0x6d0590)

	void DrawModeOverlay(Graphics* g) override;    // slot 43 (0x6dcd00): the spider and the butterflies' moves left

	// 0x6d7c70 (bejeweled3): drops butterflies on the bottom row (see the .cpp)
	void SpawnButterflies();
	// 0x6c6b80 (bejeweled3): the butterflies on the grid that are not being let go
	int CountButterflies() const;
	// the move countdown a butterfly is given: the larger of 1 and MoveCountdown less the whole part of the level times the per-level step
	int MoveCountdownFor(float thePerLevel) const;

private:
	void StepSpider();
	bool ClimbButterflies();
};

}  // namespace Sexy
