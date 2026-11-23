#include "Board.h"

#include "Background.h"
#include "Bej3App.h"
#include "Color.h"
#include "Common.h"
#include "WidgetManager.h"
#include "X86Math.h"
#include "Font.h"
#include "Profile.h"
#include "Graphics.h"
#include "Image.h"
#include "PIEffect.h"
#include "Res.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace Sexy {

namespace {

// The game's backgrounds are 19: 0x8d2170 gives the order they come in, each a name of the table at 0x8d2118
const int BACKGROUND_COUNT = 19;

}  // namespace

// 0x7203b0 (bejeweled3), with 0x720120 (bejeweled3): the curves the exe's piece holds (+0x20, +0x90, +0x100, +0x198, ...)
// are all idle at the start, which is what every test of them in the fill reads, so they are left out
Piece::Piece(Board* theBoard)
	: mBoard(theBoard)
	, mId(theBoard->mPieceCounter++)
	, mCol(0)
	, mRow(0)
	, mX(0.0f)
	, mY(0.0f)
	, mCreatedUpdate(0)
	, mFallSpeed(0.0f)
	, mColumnTag(-1)
	, mCanFall(true)
	, mCanSwap(true)
	, mSpecialState(0)
	, mMovedUpdate(0)
	, mColor(NO_COLOR)
	, mFlags(0)
	, mBusyCount(0)
{
	mScaleCurve.SetConstant(1.0);   // 0x720120
}

// 0x74ae30 (bejeweled3): the part of the constructor that the first fill reads
Board::Board()
{
	// 0x74ae30: the board takes the profile's flags of the boxes and helps seen (+0x26a8 from +0x21c); +0x2b40 is 0 and +0x2b44 is -1
	if (gApp->mProfile != nullptr)
		mHintFlags = gApp->mProfile->mHintFlags;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++)
			mGrid[aRow][aCol] = nullptr;
	}
	for (int aCol = 0; aCol < COLS; aCol++) {
		mColumnSpeed[aCol] = 0.0f;
		mColumnTag[aCol] = -1;
	}
	// 0x74b333 and 0x74b350: "Seed: %d" is written to the debug output and the board's generator seeded with the app's
	// next number (0x439a90 is the app's Rand())
	mCoverCurve.SetConstant(1.0);
	int aSeed = Rand();
	mRand.SRand(aSeed);
	// 0x74b960: each colour twice, so that each is as likely as every other
	for (int aColor = 0; aColor < Piece::COLOR_COUNT; aColor++) {
		mColorList.push_back(aColor);
		mColorList.push_back(aColor);
	}
	if (PIEFFECT_LEVELBAR != nullptr) {
		mLevelBarEffect = PIEFFECT_LEVELBAR->Duplicate();
		mLevelBarEffect->mLoop = true;
	}
	mEffectsUnder = new EffectsManager(this);   // 0x74b653 and 0x74b6a0: 0x72a510 twice, the second added as a child
	mEffectsOver = new EffectsManager(this);
	MakeClassicButtons();   // 0x741840: the layout puts the buttons where the pictures say
}

// 0x720040 (bejeweled3), with 0x71ffd0: each effect that holds the piece is told it has none (its piece id is 0) and loses a user
Piece::~Piece()
{
	for (Effect* anEffect : mEffects) {
		anEffect->mPieceId = 0;
		anEffect->mUsers--;
	}
}

Board::~Board()
{
	ClearEffects();
	if (mBackground != nullptr) {
		if (mBackground->mParent != nullptr)
			mBackground->mParent->RemoveWidget(mBackground);
		delete mBackground;
	}
	delete mLevelBarEffect;
	delete mEffectsUnder;
	delete mEffectsOver;
	// the two buttons are the board's children
	for (ButtonWidget* aButton : {mHintButton, mMenuButton, mResetButton}) {
		if (aButton != nullptr) {
			RemoveWidget(aButton);
			delete aButton;
		}
	}
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++)
			delete mGrid[aRow][aCol];
	}
}

// 0x73b390 (bejeweled3): the Classic board adds nothing to the fields translated
ClassicBoard::ClassicBoard()
{
}

// 0x74a880 (bejeweled3): the pieces go and the counters the fill reads are back to what a new board has
void Board::ResetGame()
{
	ClearEffects();   // 0x74ad6f: 0x729ca0 for +0x12bc and +0x12c0
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = nullptr;
		}
	}
	mLevelIndex = 0;
	mLevelNumber = 1;   // 0x74a880: +0xe0c
	mScore = 0;
	mScorePoints = 0;
	mMatchCounter = 0;
	mMoveRecords.clear();
	mSwaps.clear();
	mMatchGroups.clear();
	mGroupGlow = 0.0f;   // 0x74a880: +0x27b8 and +0x1240
	mRingFlash = 0.0f;
	mPendingSwaps.clear();
	mMouseDown = false;
	mSelectedAtMouseUp = nullptr;
	// 0x74aa22 to 0x74aad5: the counters of a game are back to what a new board has
	mGameTicks = 0;
	mLandTicks = 0;
	mUpdateCnt = 0;
	mFallFactorCurve.SetConstant(1.0);
	mGravityHold = 0;
	// 0x74a880: the level, the bars, the game's end and the hint timer are back to what a new board has (the counters by slot 153)
	for (int aKey = 0; aKey < COUNTER_COUNT; aKey++)
		mCounters[aKey] = 0;
	mLevelProgress = 0;
	mLevelBar = 0.0f;
	mSecondBar = 0.0f;
	mBarRateCurve.SetConstant(1.0);
	mLevelUpPending = false;
	mHintQueue.clear();   // the help a mode asks for is queued again by the start (RequestHelp)
	mHelpHold = 0;
	mHyperspace.reset();
	mLevelCounter = 0;
	mGameOver = false;
	mGameOverTicks = 0;
	mEndScreenShown = false;
	mHintTimer = 0;
	mHintFlash = 0;
	mCheckerFlash = 0.0f;   // 0x74aa8d: +0x1188
}

// slot 164, 0x749c20 (bejeweled3), with the step 1 (0x74e750, the level change at 0x769c36 and 0x760bcd: the index goes up by the step, modulo
// 19, and the table at 0x8d2170 gives which of the 20 animation names at 0x8d2118 it is). The one before is taken off the manager and let
// go; the new one is sized (-160, 0, 1920, 1200) in 1200ths of the art height, put behind everything (z-order -1) and added to the
// app's widget manager. port: left out: the random pick (a step of 0 with no index yet) and the exe's check that backgrounds are on
// (slot 99, 0x18c) and that the app has a manager
void Board::SetBackground(int theStep)
{
	// 0x8d2118: the names (the 20th, "pointy_ice_path_purple", is not reached by the order)
	static const char* const aNames[20] = {"canyon_wall_castle.pam", "crystal_mountain_peak.pam", "dark_cave_thing.pam",
		"desert_pyramids_sunset.pam", "fairy_cave_village.pam", "floating_rock_city.pam", "flying_sail_boat.pam",
		"horse_forest_tree.pam", "jungle_ruins_path.pam", "lantern_plants_world.pam", "Lion_tower_cascade.pam", "pointy_ice_path.pam",
		"rock_city_lake.pam", "bridge_shroom_castles.pam", "snowy_cliffs_castle.pam", "treehouse_waterfall.pam", "tube_forest_night.pam",
		"water_bubble_city.pam", "water_fall_cliff.pam", "pointy_ice_path_purple.pam"};
	// 0x8d2170: the order of the 19 backgrounds, as indices of the names
	static const int anOrder[BACKGROUND_COUNT] = {10, 7, 5, 9, 14, 11, 8, 18, 12, 16, 2, 17, 4, 0, 15, 13, 1, 3, 6};
	if (mBackground != nullptr) {
		if (mBackground->mParent != nullptr)
			mBackground->mParent->RemoveWidget(mBackground);
		gApp->SafeDeleteWidget(mBackground);
		mBackground = nullptr;
	}
	mBackgroundIndex = (mBackgroundIndex + BACKGROUND_COUNT + theStep) % BACKGROUND_COUNT;
	mBackground = new Background(aNames[anOrder[mBackgroundIndex]]);
	mBackground->Resize(gApp->mArtRes * -160 / 1200, 0, gApp->mArtRes * 1920 / 1200, gApp->mArtRes * 1200 / 1200);
	gApp->mWidgetManager->AddWidget(mBackground);
}

void Board::ChooseBackground()
{
	// 0x74e63b and 0x74e750: the index is set to -1 and 0x749c20 takes the next one, with no random number
	mBackgroundIndex = -1;
	SetBackground(1);
}

// 0x74e410 (bejeweled3): Classic has no seed override (0x73b4f0 returns 0), the saved game of the mode is not there, so the
// background is the first of the game's order and the first fill is made. port: the board's camera setup (0x73e440) and the counters
// the update reads (+0xc84 to +0xd18) come with their steps
void Board::NewGame()
{
	ChooseBackground();
	mSettledOnce = false;   // 0x74e775
	// 0x74e1f0 would load a saved game here
	FillInBlanks(false);
}

// 0x73caa0 (bejeweled3) is false while the board holds no effect pieces (the list at +0xc30 is empty), and a piece's
// curves at +0x24 and +0x198 are idle: so no piece is held by them
// 0x763e90 (bejeweled3)
void Board::FillInBlanks(bool theNoMatchCheck)
{
	if (mFillHeld != 0 || IsFillStopped())
		return;
	// a piece that is still being worked on holds the whole fill
	for (int aRow = mLastRow; aRow >= 0; aRow--) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && mGrid[aRow][aCol]->mBusyCount > 0)
				return;
		}
	}

	std::vector<Piece*> aNewPieces;
	bool aChanged;
	do {
		aChanged = false;
		for (int aCol = 0; aCol < COLS; aCol++) {
			bool aGapSeen = false;
			int aTop = -GetGridY();                    // the highest y of a piece in the column, as a whole number
			double aLastFallSpeed = mColumnSpeed[aCol];
			int aColumnTag = mColumnTag[aCol];
			for (int aRow = mLastRow; aRow >= 0; aRow--) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == nullptr) {
					if (!aGapSeen) {
						aGapSeen = true;
					}
					else
						mGrid[aRow + 1][aCol] = nullptr;
					continue;
				}
				aPiece->mCanFall = true;
				if (aPiece->mY < (float)aTop)
					aTop = (int)aPiece->mY;
				if (!aGapSeen)
					continue;
				if (aPiece->mSpecialState == -1) {
					// kept in place: what is under it is empty, and the run of gaps starts again above it
					mGrid[aRow + 1][aCol] = nullptr;
					aGapSeen = false;
					continue;
				}
				// it falls one row
				aChanged = true;
				if (aPiece->mFallSpeed == 0.0f) {
					aPiece->mFallSpeed = (float)(mColumnSpeed[aCol] + 1.0 + aPiece->mFallSpeed);
					aPiece->mMovedUpdate = mUpdateCnt;
				}
				aLastFallSpeed = aPiece->mFallSpeed;
				aPiece->mRow++;
				mGrid[aRow][aCol] = nullptr;
				mGrid[aRow + 1][aCol] = aPiece;
				aColumnTag = aPiece->mColumnTag;
			}
			if (aGapSeen) {
				// a new piece at the top of the column, above the highest one, a few more cells up by chance
				Piece* aNewPiece = MakePiece(aCol, 0);
				aChanged = true;
				aNewPiece->mFallSpeed = (float)(aLastFallSpeed - 0.55000001192092896);
				aNewPiece->mColumnTag = aColumnTag;
				int aAbove = aTop - CELL;
				unsigned long aJitter = mRand.Next(15UL);
				aNewPiece->mY = (float)(((double)(float)aAbove - (double)(int)aJitter) - 10.0);
				if (-128.0f < (float)GetGridY() + aNewPiece->mY)
					aNewPiece->mY = (float)(-CELL - GetGridY());
				aNewPieces.push_back(aNewPiece);
				// 0x764247 to 0x76435e: how long the column takes to land, kept as the longest in +0xe40: for each piece
				// from the last row up, the time to fall to its row from where it is (or, for one far above the screen,
				// from a place that is 256 higher for each such piece), by s = a t t / 2 with a = 0.265 times the
				// fall-speed curve's factor (UpdateFalling uses 0.275 and the same factor)
				int aStackedAbove = 0;
				for (int aRow = mLastRow; aRow >= 0; aRow--) {
					const Piece* aColumnPiece = mGrid[aRow][aCol];
					if (aColumnPiece == nullptr)
						continue;
					float aScreenY = (float)GetGridY() + aColumnPiece->mY;
					if (!((double)(-GetGridY()) <= (double)aScreenY)) {   // kept as the exe compares the floats: a NaN position counts as above the screen
						aStackedAbove += 0x100;
						aScreenY = (float)(-(GetGridY() + aStackedAbove));
					}
					double aDistance = ((double)(aColumnPiece->mRow << 7) - (double)aScreenY) * 2.0;
					int aLandTicks = Ftol(std::sqrt(aDistance / ((double)GetFallFactor() * 0.26499999999999999)));
					if (mLandTicks > aLandTicks)
						aLandTicks = mLandTicks;
					mLandTicks = aLandTicks;
				}
			}
			mColumnTag[aCol] = -1;
		}
	} while (aChanged);

	if (aNewPieces.empty())
		return;

	// The colours: every piece the fill made gets one of the list's, and all of them again until the grid has no match
	// and has a move (0x764390 and on). Classic's slots 182, 183 and 128 (0x2d8, 0x2dc, 0x200) are false, so the
	// special-gem choices they guard do not run, and slot 90 (0x73c410) is false after a reset
	bool aOk = false;
	int aTries = 0;
	const int aTryLimit = mSettledOnce ? 200000 - 0x30c78 : 200000;      // 0x30d40, less 0x30c78 once the board has been settled (+0x1190)
	const int aBonusTries = 5;         // slot 190 (0x73bb70)
	for (;;) {
		for (Piece* aPiece : aNewPieces) {
			aPiece->mFlags = 0;
			aPiece->mColor = Piece::NO_COLOR;
			aPiece->mCanFall = true;
			aPiece->mTimeGemSeconds = 0;   // 0x7642xx: +0x244
		}
		for (size_t i = 0; i < aNewPieces.size(); i++) {
			Piece* aPiece = aNewPieces[i];
			if (aPiece->mFlags == 0 || (aPiece->mFlags & Piece::FLAG_RANDOM_PICK) != 0)
				aPiece->mColor = mColorList[mRand.Next((unsigned long)mColorList.size())];
			if (aTries >= aTryLimit && i == 0) {
				theNoMatchCheck = true;
				aOk = true;
			}
		}
		// slot 184 (0x73bc10) is true: a move is looked for
		if (HasMove(aTries < aBonusTries))
			aOk = true;
		if (!theNoMatchCheck && HasMatches())
			aOk = false;
		// slot 199 (0x73c870) finds a run through a piece of this update with the bit 0x10, which a plain piece has not
		if (aOk && OnFillAccepted(aNewPieces))
			break;      // slot 185 (0x2e4): 0x73bc00 is true; SpeedBoard's 0x716c60 makes the time gems and is true as well
		aTries++;
	}
	OnFillDone(aNewPieces);   // slot 204 (0x330)
}

// 0x8d2da8 (bejeweled3): the game's speed, 1 at the normal pace; Update eases it towards what the state asks for
static float sGameSpeed = 1.0f;

// 0x759170 (bejeweled3). port: what the update does besides the steps is left out until its step: 0x73b640 (slot 207: true
// while +0xd1c and +0xd20 are not above 0), +0x304c (0 until the fireworks), +0x2c28 (0 until its effect), the
// widgets' alphas (+0x12bc and +0x12c0 at +0x110), the unpause of dialog 0x17 and the pulse of +0x29f0. Draws no random number
// on a plain game
void Board::Update()
{
	if (mLevelBarEffect != nullptr)
		mLevelBarEffect->Update();   // port: the exe's own update of the bar's effect (set from 0x755620) is not read
	// +0x1414 and +0x1410 are 0 while a game is played: the speed is asked to be 1 (0.95 is how fast it follows)
	const float aWantedSpeed = 1.0f;
	const float aFollow = 0.95f;
	sGameSpeed = (float)((double)aWantedSpeed * (1.0 - (double)aFollow) + (double)sGameSpeed * (double)aFollow);
	if (std::fabs((double)(sGameSpeed - aWantedSpeed)) < 0.01)
		sGameSpeed = aWantedSpeed;
	// slot 120 (0x73b5e0) is 1 for Classic
	sGameSpeed = (float)(1.0 * (double)sGameSpeed);
	float aSteps = mTickFraction + sGameSpeed;
	int aWholeSteps = Ftol((double)aSteps);
	mTickFraction = (float)((double)aSteps - (double)aWholeSteps);
	for (; aWholeSteps > 0; aWholeSteps--)
		UpdateTick();
	// 0x759684: the screen that is leaving goes when it has faded out (the app's board is none again)
	if (mLeaving && mCoverCurve.GetOutVal() == 0.0 && gApp->mBoard == this) {
		if (mParent != nullptr)
			mParent->RemoveWidget(this);
		gApp->SafeDeleteWidget(this);
		gApp->mBoard = nullptr;
	}
}

// slot 232, 0x756730 (bejeweled3). The Main Menu button of the options dialog (dialog 0x15): the app shows the first screen's play
// menu again (0x7716e0), the picture behind goes out of sight, this screen's cover fades from 1 to 0 over 50 updates (the exe also
// zooms the screen from 1 to 1.2 by the curve at +0x2908, which the port does not make), the hint and menu buttons are disabled
// (slot 253, 0x73b510), and the game stands still (UpdateGame, 0x767f85). port: the exe's other branch (a Quest game, +0x2b44 or
// the app's +0xbd8 not 0: 0x76c500), the sound of the click (0x76aaf0) and the picture's fade (0x77dde0) are left out
void Board::LeaveToMenu()
{
	if (mLeaving)
		return;
	gApp->ShowPlayMenu();
	if (mBackground != nullptr)
		mBackground->SetVisible(false);
	if (mCoverCurve.GetOutVal() > 0.0)
		mCoverCurve.SetCurve("b;0,1,0.02,1,~###         ~#EAC");
	mLeaving = true;
	if (mHintButton != nullptr)
		mHintButton->SetDisabled(true);
	if (mMenuButton != nullptr)
		mMenuButton->SetDisabled(true);
	if (mResetButton != nullptr)
		mResetButton->SetDisabled(true);
}

// 0x767090 (bejeweled3). port: left out until its step: the dialogs that stop a step (ids 0x29, 0x2b and 0x27, and 0x17), the
// saved-game and replay branches (+0x306c, +0x306d, +0x12f8), the intro's slot 228
// (0x7428a0, after 200 updates), the fades of +0x12c8 and +0x12cc, the hold of +0x2b40, the pieces' own update (0x720a50:
// for a plain piece it steps two idle curves and clears its shake offsets), the clock records of 0x763a30 and the panel
// of 0x7596e0 (slot 111 is true), the timed events of +0xc60, the idle curves of the pieces (+0x198, +0x90, +0x280 ...),
// the pulse of +0x2858 and the +0x170 fades. 0x75a6e0 is 0 while no piece has +0x183 set, and slot 230 (0x73b6d0)
// changes nothing while the shown score is the score
void Board::UpdateTick()
{
	// 0x767090: while a help dialog is up the board does not step at all. After a mode's start asked for help (+0x2b40, 100) the board waits: at 10 the
	// first request is put up (0x765d50), and the game goes on when the count has run out (the exe's slot 253, which enables the buttons, is not made)
	if (gApp->GetDialog(HELP_DIALOG_ID) != nullptr)
		return;
	if (mHelpHold > 0) {
		if (mHelpHold == 10) {
			CheckForTutorialDialogs();
			if (gApp->GetDialog(HELP_DIALOG_ID) != nullptr)
				return;
		}
		mHelpHold--;
		return;
	}
	// 0x475660 is WidgetContainer::Update: the board's own update count goes up here, not in Update
	WidgetContainer::Update();
	mEffectsOver->Update();   // the +0x12c0 layer is a child of the board, added before the overlay: its update (0x7303f0) is part of the container's
	UpdateHyperspace();   // the overlay is a child of the board: its update (0x725ef0) is part of the container's
	mEffectsUnder->Update();   // 0x767293: slot 22 of +0x12bc, which is not a child
	// 0x75ee20 for the moves the player asked for since the last update (the list at +0xc60)
	std::vector<PendingSwap> aPending;
	aPending.swap(mPendingSwaps);
	for (size_t i = 0; i < aPending.size(); i++) {
		Piece* aPiece = FindPieceById(aPending[i].mPieceId);
		if (aPiece != nullptr)
			StartSwap(aPiece, aPending[i].mRow, aPending[i].mCol, aPending[i].mForced);
	}
	UpdateGame();
	UpdateShownScore();
	UpdateBot();   // port: test only
}

// 0x767e20 (bejeweled3). port: left out until its step: the other 19 curves it steps, the flag at +0x2978, the flagged
// pieces at game tick 150 (0x1800), 0x73d170 every 100 game ticks, 0x73d800, the swaps (0x73c630, 0x759840, 0x759ba0) and
// the match search (slot 201, 0x75af60), which find nothing on a board that was filled without a match; 0x75fd30 while
// +0x2ef8 is not 0, the timers of +0x22e0, the hint (slot 98), the flag loops of the pieces (+0x184, 0x60, 0x100, 0x270),
// slot 181 and the busy counts (+0x230), the moves of the swap curve (+0x198), 0x765d50 (the tutorial boxes of the
// special gems), 0x75dee0 (the matches), slots 178, 208, 240, 0x75a760, 0x755940 and 0x755620 (the bars), and what comes
// after the idle shine (the score bar, the fades). The random numbers it draws are the idle shine's, at most three
// (the app's Rand()) in each update in which the board is settled
void Board::UpdateGame()
{
	mShineCurve.IncInVal();
	mCoverCurve.IncInVal();
	if (mLeaving)
		return;   // 0x767f85: nothing else steps once the screen is leaving
	// slot 101 (0x73c330) is 0 when no dialog holds the game: the game's own tick goes on. port: the end dialog (0x27) is not among those the port
	// stops it for; a hint box (0x12) is
	if (!HintDialogUp()) {
		mGameTicks++;
		if (mGameTicks % 100 == 0)
			AddCounter(COUNTER_SECONDS, 1, -1);   // 0x768125: a second of play
	}
	StepSlideCurves();   // 0x767ad0: the slide curve of every piece (+0x198), before the match search
	FindMatches();   // slot 201 (0x75af60)
	if (mFillHeld == 0) {
		// 0x73c9xx: the fill waits while a match is alive (the list at +0x125c)
		if (!HasLiveMatches())
			FillInBlanks(false);
		UpdateMoveRecords();   // 0x76818e: 0x759840, between the fill (slot 0x32c) and the swaps (0x759ba0)
		UpdateSwaps();
		UpdateFalling();
	}
	for (int aCol = 0; aCol < COLS; aCol++) {
		float aSum = (float)((double)mColumnSpeed[aCol] + 0.27500000596046448);
		mColumnSpeed[aCol] = aSum > 0.0f ? 0.0f : aSum;
	}
	// 0x7685xx: the special gems' countdowns (+0x184) go down, then the busy counts (+0x230) are run
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mSpecialCountdown != 0)
				aPiece->mSpecialCountdown--;
		}
	}
	UpdateWarnings();   // slot 181
	UpdateBusyPieces();
	UpdatePiecePulses();   // 0x768f7b to 0x7690b8, before the call of 0x75dee0
	UpdatePieceSpins();    // 0x7690d0 to 0x769149
	UpdateSlidePositions();   // 0x769189: a piece that slides is drawn along its curve
	if (!HasLiveMatches())
		FillInBlanks(false);   // 0x7691xx: slot 203
	// 0x767e20: the board is at rest, no tutorial box is up (0x765d50), the game goes on (+0xd1c is 0), slot 97 is 0 and no level change is asked
	// for: with no move left (0x75a8e0) the game ends (slot 161)
	if (IsSettled())
		CheckForTutorialDialogs();   // 0x767e20: before the move search
	if (IsSettled() && mGameOverTicks == 0 && !FindMove(nullptr, 0, false, nullptr, false) && !mLevelUpPending)
		NoMoreMoves(true);
	UpdateMatchGroups();   // 0x75dee0
	StartLevelChange();   // 0x767e20: the block for +0x288c, after the groups
	// port: slot 178 (0x75fc10, which counts +0xe3c), the curves (0x743ff0, slot 208) and the bar stage (0x75a760: only the music
	// and the look read it) are left out
	UpdateLevelBar();   // 0x755940 (0x75a760 and 0x755620 are the look)
	// the hint: with the board at rest and nothing fading (+0x12c8 is 0) the timer counts to 15 * 100 (slot 106 is 15)
	if (IsSettled() && HintAllowed()) {
		mHintTimer++;
		if (mHintTimer == GetHintWaitSeconds() * 100)
			ShowHint(false);
	}
	UpdateGameOver();   // the hold block (+0xd1c)
	if (!IsSettled()) {
		mShineOn = false;
	}
	else {
		// the idle shine: how soon it comes again (0.1 at least) follows from how much happens
		double aRate = (double)mCounters[COUNTER_ACTIVITY] / ((double)mGameTicks / 100.0);
		float aShineRate = aRate >= (double)0.10000000149011612f ? (float)aRate : 0.10000000149011612f;
		float aFirst = (float)((double)(Rand() % 1000000) / 1000000.0);
		bool aStart = false;
		if (aFirst < 0.01f && mShineUpdate == 0 && mUpdateCnt >= 0x32)
			aStart = true;
		else {
			float aSecond = (float)((double)(Rand() % 1000000) / 1000000.0);
			if (0.0030000000260770321 * (double)aShineRate > (double)aSecond && !mShineOn)
				aStart = true;
			else {
				float aThird = (float)((double)(Rand() % 1000000) / 1000000.0);
				if ((double)aShineRate * 0.0006000000284984708 > (double)aThird)
					aStart = true;
			}
		}
		if (aStart) {
			mShineCurve.SetCurve("b+-200,1500,0.006667,1,#0zN         ~~W7v");
			mShineOn = true;
			mShineUpdate = mUpdateCnt;
		}
	}
	if (mHintFlash > 0)
		mHintFlash--;   // +0x26a0
	// 0x769df4 to 0x769e46: with no match group alive the checkered squares' light (+0x1188) goes down. port: when it comes to 0 the exe
	// calls 0x742db0 (and puts 0 in +0x1078), which is not translated
	if (!HasLiveMatches() && mCheckerFlash > 0.0f) {
		float aLeft = (float)((double)mCheckerFlash - 0.00125);
		mCheckerFlash = aLeft > 0.0f ? aLeft : 0.0f;
	}
	if (mLandTicks > 0)
		mLandTicks--;
}

// 0x75fca0 (bejeweled3). port: slot 140 (0x73c180) is a return, so the first settling calls nothing
bool Board::IsSettled()
{
	if (!IsAtRest())
		return false;
	if (!mSettledOnce) {
		mSettledOnce = true;
		OnFirstSettled();   // 0x75fd15: slot 140
	}
	return true;
}

// 0x75fc10 (bejeweled3)
bool Board::IsAtRest()
{
	if (mLandTicks != 0)
		return false;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && !PieceIsSettled(mGrid[aRow][aCol]))
				return false;
		}
	}
	if (HasLiveMatches())
		return false;
	if (HasMatches())
		return false;
	return true;
}

// 0x755ad0 (bejeweled3): no speed, its slide curve (+0x198) not running, in its cell, its scale curve not a pulse (type 6),
// nothing holds it (0x73caa0, with 0 as the second argument), free (can fall, or bit 0x10000), not busy, no slide shown and
// no bit 0x2000
bool Board::PieceIsSettled(const Piece* thePiece)
{
	Piece* aPiece = const_cast<Piece*>(thePiece);   // the exe's curves are read through GetOutVal, which keeps the value
	return thePiece->mFallSpeed == 0.0f && !aPiece->mSlideCurve.IsDoingCurve() && thePiece->mY == (float)(thePiece->mRow << 7) &&
		aPiece->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA && !thePiece->mBoard->IsHeldBySwap(thePiece) &&
		(thePiece->mCanFall || (thePiece->mFlags & 0x10000) != 0) && thePiece->mBusyCount == 0 &&
		aPiece->mSlideCurve.GetOutVal() == 0.0 && (thePiece->mFlags & 0x2000) == 0;
}

// 0x73dd70 (bejeweled3), slot 214: gravity, column by column from the last row up. Each piece moves down by its speed;
// above its row it speeds up (by 0.275 times the fall factor), at its row it stops (heard as a hit when it was moving
// at 2 or more), and it cannot go below the piece under it, whose speed it takes. port: the dialog 0x12 that also holds
// the game (0x73c2d0) is not translated, only the hold at +0xe24
void Board::UpdateFalling()
{
	if (mGravityHold != 0) {
		// 0x73dd91: the game is held: a piece that moves creeps at 0.01, and the columns' speeds are 0
		mGravityHold--;
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				if (mGrid[aRow][aCol] != nullptr && mGrid[aRow][aCol]->mFallSpeed != 0.0f)
					mGrid[aRow][aCol]->mFallSpeed = 0.0099999997764825821f;
			}
		}
		for (int aCol = 0; aCol < COLS; aCol++)
			mColumnSpeed[aCol] = 0.0f;
		return;
	}
	int aLandings = 0;
	int aLandingXSum = 0;
	for (int aCol = 0; aCol < COLS; aCol++) {
		float aBelowY = 1200.0f;          // where the piece under the one in hand is: the bottom of the grid
		float aBelowSpeed = 0.0f;
		for (int aRow = ROWS - 1; aRow >= 0; aRow--) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			// 0x73caa0: a piece held by a swap does not fall (its speed stays unless it is negative); a plain piece's type
			// (+0x24) is not 6
			if (IsHeldBySwap(aPiece) && aPiece->mFallSpeed >= 0.0f)
				continue;
			float aY = aPiece->mY + aPiece->mFallSpeed;
			aPiece->mY = aY;
			float aRowY = (float)(aRow * CELL);
			if (aRowY > aY) {
				// above its row: it cannot pass the piece below, else it speeds up
				float aFloorY = (float)((double)aBelowY - 128.0);
				if (aFloorY > aY)
					aPiece->mFallSpeed = (float)((double)GetFallFactor() * 0.27500000596046448 + (double)aPiece->mFallSpeed);
				else {
					aPiece->mY = aFloorY;
					aPiece->mFallSpeed = aBelowSpeed;
				}
			}
			else {
				// at its row
				aPiece->mY = aRowY;
				if (aPiece->mFallSpeed >= 2.0f) {
					aLandings++;
					aLandingXSum += 0x40 + Ftol((double)((float)GetGridX() + aPiece->mX));
				}
				if (aPiece->mFallSpeed > 0.0f)
					aPiece->mFallSpeed = 0.0f;
			}
			if (aPiece->mFallSpeed != 0.0f)
				aPiece->mMovedUpdate = mUpdateCnt;
			aBelowY = aPiece->mY;
			aBelowSpeed = aPiece->mFallSpeed;
		}
	}
	// 0x73df83: landings are heard at most every 8 updates, panned by where they were
	if (aLandings > 0 && std::abs(mLastLandSoundUpdate - mUpdateCnt) >= 8) {
		mLastLandSoundUpdate = mUpdateCnt;
		int aMiddle = aLandingXSum / aLandings - GetGridX() - 0x200;
		double aPan = (double)aMiddle * 0.0009765625 * 2.0;
		gApp->PlaySoundEx(SOUND_GEM_HIT, 1.0, Ftol(aPan * 800.0), 0.0);
	}
}

// 0x7415c0 (bejeweled3)
Piece* Board::MakePiece(int theCol, int theRow)
{
	Piece* aPiece = new Piece(this);
	aPiece->mCreatedUpdate = mUpdateCnt;
	aPiece->mMovedUpdate = mUpdateCnt;
	aPiece->mX = (float)(theCol * CELL);
	aPiece->mCol = theCol;
	aPiece->mRow = theRow;
	aPiece->mY = (float)(theRow * CELL);
	mGrid[theRow][theCol] = aPiece;
	return aPiece;
}

// 0x73ba40 (bejeweled3): rows first, then columns; a cell with no piece or no colour breaks a run
bool Board::HasMatches() const
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		int aRun = 0;
		int aColor = -1;
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr) {
				aColor = -1;
				continue;
			}
			int aPieceColor = aPiece->mColor;
			if (aPieceColor == -1 || aPieceColor != aColor) {
				aRun = 1;
				aColor = aPieceColor;
			}
			else if (++aRun > 2)
				return true;
		}
	}
	for (int aCol = 0; aCol < COLS; aCol++) {
		int aRun = 0;
		int aColor = -1;
		for (int aRow = 0; aRow < ROWS; aRow++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr) {
				aColor = -1;
				continue;
			}
			int aPieceColor = aPiece->mColor;
			if (aPieceColor == -1 || aPieceColor != aColor) {
				aRun = 1;
				aColor = aPieceColor;
			}
			else if (++aRun > 2)
				return true;
		}
	}
	return false;
}

}  // namespace Sexy
