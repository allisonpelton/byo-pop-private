// Sexy::ButterflyBoard: see the header. Every function says its slot and address in the exe (bejeweled3.exe).
#include "ButterflyBoard.h"

#include "Bej3App.h"
#include "EndLevelDialog.h"
#include "Graphics.h"
#include "PopAnim.h"
#include "Res.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// 0x6d7c70: the two passes and the attempts in each, and the row where butterflies are dropped (the bottom one)
const int SPAWN_PASSES = 2;
const int SPAWN_ATTEMPTS = 100;
const int SPAWN_ROW = 7;
// where the spider's animation hangs, in 1200ths: its widget's top (0x6e82a0 puts it at y -0x1a9) and its shift from the column's left edge
// (port: found by looking: the exe's x, grid x less 0x32 less a value of the spider's smoothed place that was not followed)
const int SPIDER_Y = -0x1a9;
const int SPIDER_SHIFT_X = 0;

}  // namespace

// 0x6dcdf0 (bejeweled3): QuestBoard's constructor (0x6d9000), then the vftable 0x867d0c and the fields at +0x3248 to +0x3388: the settings are
// read by NewGame, the spider is over column 3 (+0x3288) and the quest's own settings give the Title
ButterflyBoard::ButterflyBoard()
{
}

// slot 85 (0x154), 0x6ceda0 (bejeweled3): in a secret mode +0xc1c is put to 1 (nothing the port runs reads it), then QuestBoard's reset
// (0x6cda70), which is Board's with the goal's own: not translated
void ButterflyBoard::ResetGame()
{
	QuestBoard::ResetGame();
}

// slot 135 (0x21c), 0x6e3660 (bejeweled3): the settings of the quest are read (atoi for the counts, wtof stored as a float for the rest),
// the counts that are 0 become 1, the drop and the spider are put to their start, and the board's own new game (QuestBoard 0x6ddc70, which
// is Board's 0x74e410: the background of the quest and the first fill) is made. port: the goal object (ButterflyGoal, vftable 0x86a90c: the
// setting "Butterflies", 20, is its level goal, which the secret mode never reads), the description's text and the hypermixers (+0x3186) are
// left out
void ButterflyBoard::NewGame()
{
	mDropCountdownSetting = SettingInt("DropCountdown");
	mMoveCountdown = SettingInt("MoveCountdown");
	mDropCountdownPerLevel = (float)SettingDouble("DropCountdownPerLevel");
	mMoveCountdownPerLevel = (float)SettingDouble("MoveCountdownPerLevel");
	mSideSpawnChance = (float)SettingDouble("SideSpawnChance");
	mSideSpawnChancePerLevel = (float)SettingDouble("SideSpawnChancePerLevel");
	mSideSpawnChanceMax = (float)SettingDouble("SideSpawnChanceMax");
	mSpawnCountStart = SettingInt("SpawnCountStart");
	mSpawnCountMax = (float)SettingDouble("SpawnCountMax");
	mSpawnCountPerLevel = (float)SettingDouble("SpawnCountPerLevel");
	if (mSpawnCountMax == 0.0f)
		mSpawnCountMax = 1.0f;
	if (mSpawnCountStart == 0)
		mSpawnCountStart = 1;
	mSpawnPending = false;
	mDropCountdown = 0;
	mSpawnCount = (float)mSpawnCountStart;
	mSpawnCredit = 0.0f;
	mSpiderX = 3.0f;
	mGrabTick = -1;
	mScoringButterfly = false;
	if (POPANIM_ANIMS_SPIDER != nullptr)
		POPANIM_ANIMS_SPIDER->PlayLabel("IDLE", true);
	QuestBoard::NewGame();   // 0x6ddc70 (0x6e3660 ends with it): the goal (ButterflyGoal) is made, then Board's new game
}

// slot 256 (0x6dcce0), which QuestBoard's slot 140 (0x6d0af0) runs the first time the board is at rest: with no drop owed
// yet (+0x3274 is 0) the first butterflies are dropped
void ButterflyBoard::OnQuestStart()
{
	if (mSpawnCredit == 0.0f)
		SpawnButterflies();
}

// 0x6c6b80 (bejeweled3): the butterflies on the grid (the bit 0x80) that nothing has yet let go (+0x181 is 0: the exe sets it when a piece is
// cleared, and the port takes a cleared piece off the grid at once)
int ButterflyBoard::CountButterflies() const
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && (aPiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
				aCount++;
		}
	}
	return aCount;
}

// 0x6d7c70 and 0x6e82a0 (bejeweled3): a butterfly's countdown is the larger of 1 and the setting MoveCountdown less the whole part of the
// level index (+0xe04) times the per-level step, the whole part taken by the CRT's floor (0x793050) from the float of the product, and the
// result turned to an integer (0x792bf0)
int ButterflyBoard::MoveCountdownFor(float thePerLevel) const
{
	float aProduct = (float)((double)mLevelIndex * (double)thePerLevel);
	double aWhole = std::floor((double)aProduct);
	double aLeft = (double)(float)mMoveCountdown - (double)(float)aWhole;
	return Ftol(aLeft >= 1.0 ? aLeft : 1.0);
}

// 0x6d7c70 (bejeweled3): butterflies are dropped. The drop's countdown is set again (the larger of 1 and DropCountdown less the whole part of
// the level times DropCountdownPerLevel); the count of a drop goes up by SpawnCountPerLevel (to SpawnCountMax) and the chance of a side
// column by SideSpawnChancePerLevel (to SideSpawnChanceMax), and the count is added to the credit. While the credit is 1 or more one butterfly is
// made, in two passes: each tries up to 100 columns, from two draws of the board's own generator (+0x258): a first draw over 100000 says by
// the chance whether the column is one of the inner six (1 to 6, a second draw) or a side one (a second draw's low bit: 0 or 7). The gem on the
// bottom row there must not be a butterfly yet, must be settled (0x755ad0) and either have no flags or, in the second pass, any flags
// but a colour of none. The gem becomes a butterfly (0x7492c0), gets the board's per-level step in +0x254 and its countdown; 1 is taken from
// the credit whether or not a gem was found
void ButterflyBoard::SpawnButterflies()
{
	double aDropProduct = std::floor((double)(float)((double)mDropCountdownPerLevel * (double)mLevelIndex));
	double aDropLeft = (double)(float)mDropCountdownSetting - (double)(float)aDropProduct;
	mDropCountdown = Ftol(aDropLeft >= 1.0 ? aDropLeft : 1.0);
	float aCount = mSpawnCountPerLevel + mSpawnCount;
	if (mSpawnCountMax < aCount)
		aCount = mSpawnCountMax;
	mSpawnCount = aCount;
	float aChance = mSideSpawnChancePerLevel + mSideSpawnChance;
	if (mSideSpawnChanceMax < aChance)
		aChance = mSideSpawnChanceMax;
	mSideSpawnChance = aChance;
	mSpawnCredit = aCount + mSpawnCredit;
	while (mSpawnCredit >= 1.0f) {
		bool aMade = false;
		for (int aPass = 0; aPass < SPAWN_PASSES && !aMade; aPass++) {
			for (int anAttempt = 0; anAttempt < SPAWN_ATTEMPTS && !aMade; anAttempt++) {
				unsigned aColumn;
				double aDraw = (double)(mRand.Next() % 100000) / 100000.0;   // 0x7d98: kept in the x87's precision, not rounded to a float
				if ((double)mSideSpawnChance <= aDraw)
					aColumn = mRand.Next() % 6 + 1;
				else
					aColumn = (mRand.Next() & 1) != 0 ? 7 : 0;
				Piece* aPiece = aColumn < (unsigned)COLS ? mGrid[SPAWN_ROW][aColumn] : nullptr;
				if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
					continue;
				if (aPiece->mFlags != 0 && !(aPass == 1 && aPiece->mColor > -1))
					continue;
				if (!PieceIsSettled(aPiece))
					continue;
				MakeButterflyGem(aPiece);
				aPiece->mMoveCountdownPerLevel = mMoveCountdownPerLevel;
				aPiece->mTimeGemSeconds = MoveCountdownFor(mMoveCountdownPerLevel);
				aMade = true;
			}
		}
		mSpawnCredit = (float)((double)mSpawnCredit - 1.0);
	}
}

// slot 191 (0x2fc), 0x6d7f40 (bejeweled3): a swap has found a match: with the swap one of the player's (+0xf9 is 0: the butterflies' own climbing swaps are forced) the drop's countdown goes down and, at
// 0 or less (or with no butterfly left on the grid), butterflies are dropped. The goal object's own slot 24 (0x60) then sees the swap: not
// translated (the secret mode has no goal to reach)
void ButterflyBoard::OnSwapMatched(const SwapRecord& theSwap)
{
	if (!theSwap.mForced) {
		mDropCountdown--;
		if (CountButterflies() == 0)
			mDropCountdown = 0;
		if (mDropCountdown < 1)
			SpawnButterflies();
	}
}

// slot 180 (0x2d0), 0x6d0390 (bejeweled3): a hypercube's group is made: the goal's slot, Board's 0x74db70 and then a drop is asked for at the
// next quiet update
void ButterflyBoard::MakeHypercubeGroup(Piece* theCube, int theColor)
{
	QuestBoard::MakeHypercubeGroup(theCube, theColor);
	mSpawnPending = true;
}

// slot 98 (0x188), 0x6c6bd0 (bejeweled3): no hint while a butterfly is caught (a gem with +0x20c at -1), else Board's (0x73d4f0)
bool ButterflyBoard::HintAllowed() const
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mSpecialState == -1)
				return false;
		}
	}
	return Board::HintAllowed();
}

// slot 143 (0x23c), 0x6d03d0 (bejeweled3): a piece is cleared. A butterfly (the goal's slot 40, 0x6ab2f0: the game's count of butterflies
// freed, counter 0x1c, goes up by one for the move's tag) frees points: the move's own count n of butterflies freed (counter 0x1c of the
// record), which is also kept as the best the game has seen (counter 0x1d, by 0x73cfd0: the larger of the two; the profile's statistic it
// also raises is not translated), and the points are (the game's count including this one + 10 + 20 n) times 5, which AddPoints multiplies
// by the level factor (slot 122, 5)
void ButterflyBoard::OnPieceCleared(Piece* thePiece)
{
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
		return;
	if (mGoal != nullptr)
		mGoal->OnPieceCleared(thePiece);   // 0x6d03d0 runs the goal's slot 40 first (0x6ab2f0, ButterflyGoal)
	else
		AddCounter(COUNTER_BUTTERFLIES_FREED, 1, thePiece->mColumnTag);   // a secret mode has no goal: the same count
	int aMoveCount = GetMoveCounter(thePiece->mColumnTag, COUNTER_BUTTERFLIES_FREED);   // 0x73cf10
	mCounters[COUNTER_BEST_COMBO] = std::max(mCounters[COUNTER_BEST_COMBO], aMoveCount);   // 0x73cfd0
	if (thePiece->mColumnTag != -1) {
		MoveRecord* aRecord = FindMoveRecord(thePiece->mColumnTag);
		if (aRecord != nullptr)
			aRecord->mCounters[COUNTER_BEST_COMBO] = std::max(aRecord->mCounters[COUNTER_BEST_COMBO], aMoveCount);
	}
	mScoringButterfly = true;
	AddPoints((mCounters[COUNTER_BUTTERFLIES_FREED] + 10 + aMoveCount * 20) * 5, thePiece->mColumnTag);
	mScoringButterfly = false;
	// 0x6d01b0 (the burst at the butterfly) is left out. When the level's goal is met (not in a secret mode) every butterfly on the grid gets one move more
	if (!mIsSecretMode && GetLevelProgress() >= GetLevelMax()) {
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece != nullptr && (aPiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
					aPiece->mTimeGemSeconds++;
			}
		}
	}
}

// slot 53 (0xd4), 0x6ae830 (bejeweled3): the setting Butterflies
void ButterflyGoal::Load()
{
	mButterflies = mBoard->SettingInt("Butterflies");
}

// slot 22 (0x58), 0x6aa420 (bejeweled3): the level's size is the butterflies to free, 0 in a secret mode
int ButterflyGoal::GetLevelMax()
{
	return mBoard->mIsSecretMode ? 0 : mButterflies;
}

// slot 40 (0xa0), 0x6ab2f0 (bejeweled3): a butterfly is freed (while the game is not lost, or in a secret mode): the game's count of butterflies freed (counter 0x1c) goes up for
// the move's tag, and in a quest the score (+0xd24) and the progress (+0xe00) go up by 1
void ButterflyGoal::OnPieceCleared(Piece* thePiece)
{
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
		return;
	if (mBoard->mGameOverTicks != 0 && !mBoard->mIsSecretMode)
		return;
	mBoard->AddCounter(ButterflyBoard::COUNTER_BUTTERFLIES_FREED, 1, thePiece->mColumnTag);
	if (!mBoard->mIsSecretMode) {
		mBoard->mScorePoints++;
		mBoard->mLevelProgress++;
	}
}

// slot 161 (0x284), 0x6dcd50 (a jump to QuestBoard's 0x6d8e30, bejeweled3): the game ends. The goal's veto (slot 10) and the level-full test
// (slot 217) are false in a secret mode; then the score is handed in once (0x6ca240: the +0x307c wait and 0x113, the leaderboard) and the
// game ends (0x7602c0): Board's own NoMoreMoves does both
void ButterflyBoard::NoMoreMoves(bool theAnnounce)
{
	QuestBoard::NoMoreMoves(theAnnounce);   // 0x6d8e30: for a secret mode (it records high scores) Board's; a quest just ends
}

// slot 257 (0x404), 0x6d0590 (bejeweled3): the end dialog of the mode (0x784c30, vftable 0x86be7c)
ClassicEndLevelDialog* ButterflyBoard::MakeEndLevelDialog()
{
	return new ButterflyEndLevelDialog(this);
}

// slot 231 (0x39c), 0x6e82a0 (bejeweled3): Board's step first (through QuestBoard's, with the move records), then the spider and the
// butterflies. port: the exe's curve at +0x3310 that +0x3284 follows (a look only), the sounds, the spider's idle animation (its idle label is
// played at the draw of the app's Rand() of the exe's other branch) and the glow at +0x264 of each butterfly (drawn only) are left out
void ButterflyBoard::UpdateGame()
{
	QuestBoard::UpdateGame();
	// the pieces that are caught (+0x20c is -1) creep: their speed is 0.01 (0x3c23d70a)
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mSpecialState == -1)
				aPiece->mFallSpeed = 0.0099999997764825821f;
		}
	}
	StepSpider();
	if (POPANIM_ANIMS_SPIDER != nullptr)
		POPANIM_ANIMS_SPIDER->Update();
	if (mSpawnPending && mMatchGroups.empty()) {
		SpawnButterflies();
		mSpawnPending = false;
	}
	ClimbButterflies();
}

// 0x6e82a0 (bejeweled3), the spider: while its walk runs it goes on (a step of the curve; at the end it is over the column), else it looks for
// a butterfly: the rows -1 (the caught ones, whose cell is the top row) and 0 on to the fifth (the eighth in the first 150 game ticks), the
// first row with one being the one it looks at, and in it the one nearest to the spider (the first on a tie). Over its column it ends the
// game when that butterfly is caught (and the board has no move in the making: no tagged gem that is not caught, and every caught one on the
// top row); otherwise it starts a walk (in the first 150 ticks it jumps), whose speed falls with the distance and the row
void ButterflyBoard::StepSpider()
{
	if (mSpiderCurve.IsDoingCurve()) {
		if (!mSpiderCurve.IncInVal()) {
			mSpiderX = mSpiderTo;
			if (POPANIM_ANIMS_SPIDER != nullptr)
				POPANIM_ANIMS_SPIDER->PlayLabel("IDLE", true);   // 0x6e8350: the walk is over (the string at 0x7d6604)
		}
		else {
			double aShare = mSpiderCurve.GetOutVal();
			mSpiderX = (float)((double)mSpiderTo * aShare + (1.0 - aShare) * (double)mSpiderFrom);
		}
		return;
	}
	bool anEarly = mGameTicks < SPIDER_EARLY_TICKS;
	Piece* aTarget = nullptr;
	for (int aRow = -1; aRow < (anEarly ? 8 : 5) && aTarget == nullptr; aRow++) {
		int aGridRow = aRow < 0 ? 0 : aRow;
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aGridRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
				continue;
			if (aRow < 0 && aPiece->mSpecialState >= 0)
				continue;
			if (aTarget != nullptr) {
				float aNew = std::fabs((float)aPiece->mCol - mSpiderX);
				float aOld = std::fabs((float)aTarget->mCol - mSpiderX);
				if (aOld <= aNew)
					continue;
			}
			aTarget = aPiece;
		}
	}
	if (aTarget == nullptr)
		return;
	if ((float)aTarget->mCol == mSpiderX) {
		if (aTarget->mSpecialState == -1) {
			if (!mGameOver && !aTarget->mSlideCurve.IsDoingCurve()) {
				// 0x6e8530: the spider grabs the caught butterfly once, when its slide is 0.3 up and the end has gone on for 50 updates
				if (mGameOverTicks > 0x32 && mGrabTick == -1 && aTarget->mSlideCurve.GetOutVal() >= 0.3) {
					if (POPANIM_ANIMS_SPIDER != nullptr)
						POPANIM_ANIMS_SPIDER->PlayLabel("GRAB", true);   // the string at 0x7d660c; its sound is left out
					mGrabTick = mGameTicks;
				}
				bool aMoveMade = false;
				for (int aRow = 0; aRow < ROWS; aRow++) {
					for (int aCol = 0; aCol < COLS; aCol++) {
						const Piece* aPiece = mGrid[aRow][aCol];
						if (aPiece != nullptr && aPiece->mColumnTag != -1 && aPiece->mSpecialState >= 0)
							aMoveMade = true;
					}
				}
				for (int aRow = 0; aRow < ROWS; aRow++) {
					for (int aCol = 0; aCol < COLS; aCol++) {
						const Piece* aPiece = mGrid[aRow][aCol];
						if (aPiece != nullptr && aPiece->mSpecialState < 0 && aPiece->mRow != 0)
							aMoveMade = false;
					}
				}
				if (mGameOverTicks == 0 && !aMoveMade)
					NoMoreMoves(true);
			}
		}
		else if (Rand() % 0x5dc == 0) {
			if (POPANIM_ANIMS_SPIDER != nullptr)
				POPANIM_ANIMS_SPIDER->PlayLabel("IDLE", true);   // 0x6e8700: the string at 0x7d6614
		}
		return;
	}
	if (anEarly) {
		mSpiderX = (float)aTarget->mCol;
		return;
	}
	mSpiderFrom = mSpiderX;
	mSpiderTo = (float)aTarget->mCol;
	if (POPANIM_ANIMS_SPIDER != nullptr)
		POPANIM_ANIMS_SPIDER->PlayLabel("WALK", true);   // 0x6e87b0: the string at 0x7d661c
	mSpiderCurve.SetCurve("b+0,1,0.005,1,####         ~~###");
	float aDistance = std::fabs((float)aTarget->mCol - mSpiderX);
	mSpiderCurve.mIncRate = (double)((float)aTarget->mRow * -0.1f + (1.8f - aDistance * 0.1f)) * mSpiderCurve.mIncRate;
}

// 0x6e82a0 (bejeweled3), the butterflies, in the order of the grid. A butterfly whose countdown is out, with no swap effect, no match group
// (the lists at +0x26b8, empty in the port, and +0x125c) climbs: with a gem above it asks for the swap with it (slot 194 with 1, 0, 0),
// whatever comes of it, and its countdown is made again. On the top row it is caught instead when its whole column is full and none of its
// gems pulses: it slides up out of the grid (to row -1, 50 updates), and nothing after it is looked at in this update
bool ButterflyBoard::ClimbButterflies()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & Piece::FLAG_BUTTERFLY) == 0)
				continue;
			if (aPiece->mTimeGemSeconds >= 1 || !mMatchGroups.empty())
				continue;
			int aAbove = aPiece->mRow - 1;
			if (aAbove >= 0) {
				if (mGrid[aAbove][aPiece->mCol] != nullptr)
					TrySwap(aPiece, aAbove, aPiece->mCol, true);   // slot 194 with 1, 0, 0
				aPiece->mTimeGemSeconds = MoveCountdownFor(aPiece->mMoveCountdownPerLevel);
				continue;
			}
			int aFilled = 0;
			while (aFilled < ROWS && mGrid[aFilled][aPiece->mCol] != nullptr &&
				mGrid[aFilled][aPiece->mCol]->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA)
				aFilled++;
			if (aFilled == ROWS) {
				if (aPiece->mSpecialState != -1) {
					aPiece->mSlideCol = aPiece->mCol;
					aPiece->mSpecialState = -1;
					aPiece->mSlideCurve.SetCurve("b+0,0.55,0.02,1,####         ~~^?L");
				}
				return true;
			}
		}
	}
	return false;
}

// slot 43 (0x2c), 0x6dcd00 (bejeweled3): the screen's own pass. port: the spider (a copy of POPANIM_ANIMS_SPIDER in the exe, a widget at +0x3384 that the
// update puts at (grid x - 0x32 less the spider's smoothed place, -0x1a9) in 1200ths) hangs on its thread over its column; the glow and
// the count of the goal's HUD (slot 56 of the goal, 0x6b0770) and the combo text (+0x3298) are not translated
void ButterflyBoard::DrawModeOverlay(Graphics* g)
{
	const int aAlphaByte = Ftol(mCoverCurve.GetOutVal() * 255.0);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, aAlphaByte));
	if (POPANIM_ANIMS_SPIDER != nullptr) {
		g->PushState();
		g->Translate(Scaled(GetGridX() + Ftol((double)mSpiderX * 128.0) + SPIDER_SHIFT_X), Scaled(SPIDER_Y));
		POPANIM_ANIMS_SPIDER->Draw(g);
		g->PopState();
	}
}

}  // namespace Sexy
