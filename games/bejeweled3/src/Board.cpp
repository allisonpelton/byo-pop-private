#include "Board.h"

#include "Bej3App.h"
#include "Color.h"
#include "Common.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

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
	, mFallDelay(0.0f)
	, mColumnTag(-1)
	, mCanFall(true)
	, mCanSwap(true)
	, mSpecialState(0)
	, mFallStartUpdate(0)
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
		mColumnDelay[aCol] = 0.0f;
		mColumnTag[aCol] = -1;
	}
	// 0x74b333 and 0x74b350: "Seed: %d" is written to the debug output and the board's generator seeded with the app's
	// next number (0x439a90 is the app's Rand())
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
}

// 0x74e410 (bejeweled3): Classic has no seed override (0x73b4f0 returns 0), the saved game of the mode is not there, so the
// background is the first of the game's order and the first fill is made. port: the background itself, the board's
// camera setup (0x73e440) and the counters the update reads (+0xc84 to +0xd18) come with their steps
void Board::NewGame()
{
	// 0x74e63b and 0x74e750: the index is set to -1 and 0x749c20 takes the next one, with no random number
	mBackgroundIndex = (-1 + BACKGROUND_COUNT + 1) % BACKGROUND_COUNT;
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
			double aLastFallDelay = mColumnDelay[aCol];
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
				if (aPiece->mFallDelay == 0.0f) {
					aPiece->mFallDelay = (float)(mColumnDelay[aCol] + 1.0 + aPiece->mFallDelay);
					aPiece->mFallStartUpdate = mUpdateCount;
				}
				aLastFallDelay = aPiece->mFallDelay;
				aPiece->mRow++;
				mGrid[aRow][aCol] = nullptr;
				mGrid[aRow + 1][aCol] = aPiece;
				aColumnTag = aPiece->mColumnTag;
			}
			if (aGapSeen) {
				// a new piece at the top of the column, above the highest one, a few more cells up by chance
				Piece* aNewPiece = MakePiece(aCol, 0);
				aChanged = true;
				aNewPiece->mFallDelay = (float)(aLastFallDelay - 0.55000001192092896);
				aNewPiece->mColumnTag = aColumnTag;
				int aAbove = aTop - CELL;
				unsigned long aJitter = mRand.Next(15UL);
				aNewPiece->mY = (float)(((double)(float)aAbove - (double)(int)aJitter) - 10.0);
				if (-128.0f < (float)GetGridY() + aNewPiece->mY)
					aNewPiece->mY = (float)(-CELL - GetGridY());
				aNewPieces.push_back(aNewPiece);
				// port: 0x764247 to 0x76435e work out how long the column takes to land into the board's +0xe40; the
				// update that waits for it comes with its step
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
	const int aTryLimit = 200000;      // 0x30d40, less 0x30c78 when the byte at +0x1190 is set (it is not)
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

// 0x7415c0 (bejeweled3)
Piece* Board::MakePiece(int theCol, int theRow)
{
	Piece* aPiece = new Piece(this);
	aPiece->mCreatedUpdate = mUpdateCount;
	aPiece->mFallStartUpdate = mUpdateCount;
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

// 0x757df0 (bejeweled3), the plain gem's branch: the gem's sheet of the colour drawn at the piece's place, whole cell
// at (the grid's corner + the piece's x and y) scaled. port: the piece is drawn at its cell, where it settles, not at its
// y above the board while it falls (the fall is not translated yet), and at the sheet's first cel: the exe's cel is the
// piece's own spin, which the update drives
void Board::DrawPiece(Graphics* g, const Piece* thePiece)
{
	Image* aSheets[Piece::COLOR_COUNT] = {IMAGE_GEMS_RED, IMAGE_GEMS_WHITE, IMAGE_GEMS_GREEN, IMAGE_GEMS_YELLOW,
		IMAGE_GEMS_PURPLE, IMAGE_GEMS_ORANGE, IMAGE_GEMS_BLUE};
	if (thePiece->mColor < 0 || thePiece->mColor >= Piece::COLOR_COUNT || aSheets[thePiece->mColor] == nullptr)
		return;
	g->DrawImageCel(aSheets[thePiece->mColor], Scaled(GetGridX() + thePiece->mCol * CELL),
		Scaled(GetGridY() + thePiece->mRow * CELL), 0);
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
