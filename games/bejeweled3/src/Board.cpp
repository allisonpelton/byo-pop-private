#include "Board.h"

#include "Bej3App.h"
#include "Color.h"
#include "Common.h"
#include "X86Math.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
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
}

// 0x74ae30 (bejeweled3): the part of the constructor that the first fill reads
Board::Board()
{
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
}

Board::~Board()
{
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
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = nullptr;
		}
	}
	mHintCount = 0;
	// 0x74aa22 to 0x74aad5: the counters of a game are back to what a new board has
	mGameTicks = 0;
	mLandTicks = 0;
	mUpdateCnt = 0;
	mFallFactorCurve.SetConstant(1.0);
	mGravityHold = 0;
}

// 0x74e410 (bejeweled3): Classic has no seed override (0x73b4f0 returns 0), the saved game of the mode is not there, so the
// background is the first of the game's order and the first fill is made. port: the background itself, the board's
// camera setup (0x73e440) and the counters the update reads (+0xc84 to +0xd18) come with their steps
void Board::NewGame()
{
	// 0x74e63b and 0x74e750: the index is set to -1 and 0x749c20 takes the next one, with no random number
	mBackgroundIndex = (-1 + BACKGROUND_COUNT + 1) % BACKGROUND_COUNT;
	mSettledOnce = false;   // 0x74e775
	// 0x74e1f0 would load a saved game here
	FillInBlanks(false);
}

// 0x73caa0 (bejeweled3) is false while the board holds no effect pieces (the list at +0xc30 is empty), and a piece's
// curves at +0x24 and +0x198 are idle: so no piece is held by them
// 0x763e90 (bejeweled3)
void Board::FillInBlanks(bool theNoMatchCheck)
{
	if (mFillHeld != 0)
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
					if (!((double)(-GetGridY()) <= (double)aScreenY)) {
						aStackedAbove += 0x100;
						aScreenY = (float)(-(GetGridY() + aStackedAbove));
					}
					double aDistance = ((double)(aColumnPiece->mRow << 7) - (double)aScreenY) * 2.0;
					int aLandTicks = Ftol(std::sqrt(aDistance / ((double)GetFallFactor() * 0.26499999999999999)));
					if (!(mLandTicks <= aLandTicks))
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
		if (aOk)
			break;      // slot 185 (0x73bc00) is true
		aTries++;
	}
}

// 0x8d2da8 (bejeweled3): the game's speed, 1 at the normal pace; Update eases it towards what the state asks for
static float sGameSpeed = 1.0f;

// 0x759170 (bejeweled3). port: what the update does besides the steps is left out until its step: 0x73b640 (slot 207: true
// while +0xd1c and +0xd20 are not above 0), +0x304c (0 until the fireworks), +0x2c28 (0 until its effect), the
// widgets' alphas (+0x12bc and +0x12c0 at +0x110), the unpause of dialog 0x17 and the pulse of +0x29f0. Draws no random number
// on a plain game
void Board::Update()
{
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
}

// 0x767090 (bejeweled3). port: left out until its step: the dialogs that stop a step (ids 0x29, 0x2b and 0x27, and 0x17), the
// saved-game and replay branches (+0x306c, +0x306d, +0x12f8), the effects widget's update (+0x12bc), the intro's slot 228
// (0x7428a0, after 200 updates), the fades of +0x12c8 and +0x12cc, the hold of +0x2b40, the pieces' own update (0x720a50:
// for a plain piece it steps two idle curves and clears its shake offsets), the clock records of 0x763a30 and the panel
// of 0x7596e0 (slot 111 is true), the timed events of +0xc60, the idle curves of the pieces (+0x198, +0x90, +0x280 ...),
// the pulse of +0x2858 and the +0x170 fades. 0x75a6e0 is 0 while no piece has +0x183 set, and slot 230 (0x73b6d0)
// changes nothing while the shown score is the score
void Board::UpdateTick()
{
	// 0x475660 is WidgetContainer::Update: the board's own update count goes up here, not in Update
	WidgetContainer::Update();
	UpdateGame();
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
	// slot 101 (0x73c330) is 0 when no dialog holds the game: the game's own tick goes on
	mGameTicks++;
	if (mFillHeld == 0) {
		// the match list at +0x125c is empty
		FillInBlanks(false);
		UpdateFalling();
	}
	for (int aCol = 0; aCol < COLS; aCol++) {
		float aSum = (float)((double)mColumnSpeed[aCol] + 0.27500000596046448);
		mColumnSpeed[aCol] = aSum > 0.0f ? 0.0f : aSum;
	}
	// the swap curve (+0x198) of every piece is idle, so the speeds the exe also adds at +0x608 to +0x610 are 0 and
	// nothing but UpdateFalling moves a piece; the match list is empty
	FillInBlanks(false);
	if (!IsSettled()) {
		mShineOn = false;
	}
	else {
		// the idle shine: how soon it comes again (0.1 at least) follows from how much happens
		double aRate = (double)mActivityCount / ((double)mGameTicks / 100.0);
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
	if (mLandTicks > 0)
		mLandTicks--;
}

// 0x75fca0 (bejeweled3). port: slot 140 (0x73c180) is a return, so the first settling calls nothing
bool Board::IsSettled()
{
	if (mLandTicks != 0)
		return false;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && !PieceIsSettled(mGrid[aRow][aCol]))
				return false;
		}
	}
	// the match list at +0x125c is empty
	if (HasMatches())
		return false;
	mSettledOnce = true;
	return true;
}

// 0x755ad0 (bejeweled3): no speed, in its cell, free (a bit 0x10000 would count too: no plain piece has it) and not busy.
// port: the piece's swap curve (+0x198) is idle, nothing holds it (0x73caa0) and its type (+0x24) is not 6
bool Board::PieceIsSettled(const Piece* thePiece)
{
	return thePiece->mFallSpeed == 0.0f && thePiece->mY == (float)(thePiece->mRow << 7) && thePiece->mCanFall &&
		thePiece->mBusyCount == 0 && (thePiece->mFlags & 0x2000) == 0;
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
			// 0x73caa0 (nothing holds a piece: the effects list is empty) is false, and a plain piece's type (+0x24) is not 6
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

// 0x75a8e0 (bejeweled3) with the arguments the fill gives it (no piece to include, the first move wanted, rows from the
// top). Every cell is tried against its right, lower, left and upper neighbour, in that order: the two pieces swap in
// the grid and the colour that came to the cell is counted along its row and column. port: the tests of the pieces' curves,
// busy counts and bits (0x2000, 0x2100) are all true for the pieces the fill makes
bool Board::HasMove(bool theNeedBonus)
{
	static const int aDirections[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			for (int aDirection = 0; aDirection < 4; aDirection++) {
				int aNeighbourCol = aCol + aDirections[aDirection][0];
				int aNeighbourRow = aRow + aDirections[aDirection][1];
				if (aNeighbourCol < 0 || aNeighbourCol >= COLS || aNeighbourRow < 0 || aNeighbourRow >= ROWS)
					continue;
				Piece* aNeighbour = mGrid[aNeighbourRow][aNeighbourCol];
				if (aNeighbour == nullptr || aNeighbour->mColor == -1)
					continue;
				mGrid[aRow][aCol] = aNeighbour;
				mGrid[aNeighbourRow][aNeighbourCol] = aPiece;
				int aColor = aNeighbour->mColor;
				int aLeft = aCol;
				while (aLeft > 0 && mGrid[aRow][aLeft - 1] != nullptr && mGrid[aRow][aLeft - 1]->mColor == aColor)
					aLeft--;
				int aRight = aCol;
				while (aRight < COLS - 1 && mGrid[aRow][aRight + 1] != nullptr && mGrid[aRow][aRight + 1]->mColor == aColor)
					aRight++;
				int aUp = aRow;
				while (aUp > 0 && mGrid[aUp - 1][aCol] != nullptr && mGrid[aUp - 1][aCol]->mColor == aColor)
					aUp--;
				int aDown = aRow;
				while (aDown < ROWS - 1 && mGrid[aDown + 1][aCol] != nullptr && mGrid[aDown + 1][aCol]->mColor == aColor)
					aDown++;
				mGrid[aRow][aCol] = aPiece;
				mGrid[aNeighbourRow][aNeighbourCol] = aNeighbour;
				int aAcross = aRight - aLeft;
				int aDownward = aDown - aUp;
				bool aCounts;
				if (theNeedBonus)
					aCounts = (aAcross > 2 || aDownward > 2) || (aAcross > 1 && aDownward > 1);
				else
					aCounts = aAcross > 1 || aDownward > 1;
				if (aCounts)
					return true;
			}
		}
	}
	return false;
}

// 0x73b250 (bejeweled3), the Classic board's Draw: the panel (0x751d50), then the gems and the score (0x750320). The exe's
// measures are in 1200ths of the art size in use
static int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 1200;
}

// 0x757df0 (bejeweled3), the plain gem's branch: the gem's sheet of the colour drawn at the piece's own place, which is its
// x and y (they are its cell's, or above it while it falls) added to the grid's corner, each cut to a whole number and
// scaled. port: at the sheet's first cel. The exe draws a sprite of its own table (the app's +0xb5c, a row of 15 for each
// colour, made at start up): frame 7 at rest, picked by (2 - the scale curve at +0x20) * 8 - 1, and that curve is the
// constant 1 for a plain piece, so nothing spins it; which cel of IMAGE_GEMS_* the table's frame 7 is has not been
// looked for yet
void Board::DrawPiece(Graphics* g, const Piece* thePiece)
{
	Image* aSheets[Piece::COLOR_COUNT] = {IMAGE_GEMS_RED, IMAGE_GEMS_WHITE, IMAGE_GEMS_GREEN, IMAGE_GEMS_YELLOW,
		IMAGE_GEMS_PURPLE, IMAGE_GEMS_ORANGE, IMAGE_GEMS_BLUE};
	if (thePiece->mColor < 0 || thePiece->mColor >= Piece::COLOR_COUNT || aSheets[thePiece->mColor] == nullptr)
		return;
	int aX = Ftol((double)((float)GetGridX() + thePiece->mX));
	int aY = Ftol((double)((float)GetGridY() + thePiece->mY));
	g->DrawImageCel(aSheets[thePiece->mColor], Scaled(aX), Scaled(aY), 0);
}

// 0x751d50 and 0x750320 (bejeweled3). port: the checkered panel behind the gems, the side glows, the level bar and the
// buttons of the screen are not translated yet
void Board::Draw(Graphics* g)
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr)
				DrawPiece(g, mGrid[aRow][aCol]);
		}
	}
	// 0x746f20: the score, in FONT_SCORE, in the middle of x 250 at y 202
	g->SetFont(FONT_SCORE);
	g->SetColor(Color(255, 255, 255, 255));
	g->WriteString(CommaSeperate(mScore), Scaled(250), Scaled(202), 0, 0, true, 0, -1, -1);
}

}  // namespace Sexy
