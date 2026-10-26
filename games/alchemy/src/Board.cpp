// The board's rules: the grid, the pieces, placing and forging, and how a game goes on and ends (see Board.h).

#include "Board.h"
#include "AlchemyApp.h"
#include "Piece.h"
#include "Wizard.h"
#include "Effects.h"
#include "SidePanel.h"
#include "ButtonWidget.h"
#include "Dialog.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "WidgetManager.h"
#include "ImageFont.h"
#include "PortCompat.h"
#include "Compat2004.h"
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

using namespace Sexy;

// The ranks: the score that gets the name gRankNames[i] (rdata 0x4e3220)
static const int gRankScores[22] =
{
	0, 400, 700, 1000, 1500, 2000, 2500, 3000, 3500, 4500, 5000, 6000, 7000, 8000, 10000, 12000, 14000, 16000,
	20000, 25000, 30000, 40000
};

// 0x41df30: the bits it takes to write a number
static int BitsRequired(int theNum)
{
	int aBits = 0;
	if (theNum < 0)
		theNum = -1 - theNum;
	if (theNum >= 1)
	{
		do
			aBits++;
		while (theNum >= (1 << aBits));
	}
	return aBits;
}

// 0x407ebe
Board::Board(AlchemyApp* theApp)
{
	mApp = theApp;
	mState = STATE_PLAYING;
	mScriptPos = 0;
	mCurPiece = NULL;
	mFxPiece = NULL;
	mButton = NULL;

	// the original seeds its generator with two draws of the C library's rand()
	unsigned long aSeed = (unsigned long) (CrtRand() << 16);
	aSeed |= (unsigned long) (CrtRand() & 0xffff);
	SeedMTRand2004(mRand, aSeed);

	// the header of the record: what an internet score would be checked with
	mRecord.WriteByte(1);
	mRecord.WriteString("");
	mRecord.WriteString("");
	mRecord.WriteLong((long) aSeed);
	mRecord.WriteBoolean(mApp->mTimeTrial);
	mRecord.WriteNumBits(mApp->mSkillLevel, 2);
	mRecord.WriteLong((long) time(NULL));

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			mGrid[aRow][aCol] = NULL;
			mLit[aRow][aCol] = false;
			mFlash[aRow][aCol] = false;
			mValid[aRow][aCol] = false;
		}
	mNumValid = 0;

	mAnimateLights = true;
	mTimeTrial = mApp->mTimeTrial && !mApp->mTutorialMode;
	mPristine = true;
	mMoves = 0;
	mDiscards = 0;
	mMultiMoves = 0;
	mNextBoardFade = 0;
	mHoverX = -1;
	mHoverY = -1;
	mProgress = 0;
	mScore = 0;
	mProgRate = 0;
	mProgDecay = 0;
	mAnimDir = 0;
	mAnimPct = 0;
	mClearRow = -1;
	mClearCol = -1;
	mSparkle = 0;
	mLightOn = false;
	mLightFading = false;
	mLightFade = 0;
	mLightVariant = 0;
	mLightCol = 0;
	mLightRow = 0;
	mLightPos = 0;
	mLightMix = 0;
	mLightMixStep = 0.1;
	mBombOn = false;
	mBombX = 0;
	mBombY = 0;
	mBombFrame = 0;
	mOverFade = 0;
	mOverTimer = 0;

	mLightBase = new MemoryImage(mApp);
	mLightBase->Create(CELL, CELL);
	mLightBase->SetImageMode(false, false);
	mLightLit = new MemoryImage(mApp);
	mLightLit->Create(CELL, CELL);
	mLightLit->SetImageMode(false, false);
	mLightOut = new MemoryImage(mApp);
	mLightOut->Create(CELL, CELL);
	mLightOut->SetImageMode(false, false);

	mSkillBase = mApp->mSkillLevel * 5 - 5;
	mBgImage = new MemoryImage(mApp);
	mFxImage = new MemoryImage(mApp);
	mFxImage->Create(0x32, 0x32);
	mFxImage->SetImageMode(true, true);

	mTutorial = mApp->mTutorialMode;
	mTutStep = mTutorial ? -1 : 0;
	mTutCount = 0;
	mStartTick = GameTickCount();
	mElapsed = 0;
	mTotalSeconds = 0;
	mPause = 0;
	mTrialPause = 0;
	mLastWarning = 0;
	mFullCount = 0;
	mRun = 0;
	mLongestRun = 0;
	mBoardsCleared = 0;
	mNextRank = 0;
	mHintOn = false;
	mHintTimer = 0;
	mNewPieceDelay = 0;
	mMouseX = 0;
	mMouseY = 0;
	mTutWaiting = false;
	mRate = 1.0;
	mLastShape = -1;
	mLastColor = -1;
	mHelpShown = false;

	SetDifficulty();

	mHelpBox = new WizardBox(mApp);

	// a game starts with a blank block in the middle of the grid
	if (!mApp->mTutorialMode)
	{
		Piece* aPiece = new Piece(mApp);
		aPiece->mCol = FIRST_COL;
		aPiece->mRow = FIRST_ROW;
		aPiece->mX = ColToX(FIRST_COL) + mX;
		aPiece->mY = RowToY(FIRST_ROW) + mY;
		aPiece->mSkull = true;
		mGrid[FIRST_ROW][FIRST_COL] = aPiece;
		mLit[FIRST_ROW][FIRST_COL] = true;
	}

	NewCurrentPiece();
}

// 0x4026fa
Board::~Board()
{
	if (mApp->mSidePanel != NULL)
		mApp->mSidePanel->SetFlashing(false);

	// a game that was nearly lost counts as played
	if (mProgress >= 2.9 && mState != STATE_GAME_OVER)
		mApp->mTimesPlayed++;

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			delete mGrid[aRow][aCol];
	delete mCurPiece;
	delete mFxPiece;
	delete mButton;
	delete mHelpBox;
	delete mBgImage;
	delete mFxImage;
	delete mLightBase;
	delete mLightLit;
	delete mLightOut;
}

// 0x4014f3
int Board::XToCol(int theX)
{
	if (theX > 0xc)
	{
		int aCol = (theX - 0xd) / CELL;
		if (aCol < COLS)
			return aCol;
	}
	return -1;
}

// 0x40151d
int Board::YToRow(int theY)
{
	if (theY > 0x28)
	{
		int aRow = (theY - 0x29) / CELL;
		if (aRow < ROWS)
			return aRow;
	}
	return -1;
}

// 0x401304
int Board::CountPieces()
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mGrid[aRow][aCol] != NULL)
				aCount++;
	return aCount;
}

// 0x401220: the skill sets the points, the number of boards cleared the pieces and the speed of the forge in
// time trial mode
void Board::SetDifficulty()
{
	if (mApp->mSkillLevel == 1)
		mRate = 0.5;
	else if (mApp->mSkillLevel == 2)
		mRate = 1.0;
	else if (mApp->mSkillLevel == 3)
		mRate = 2.0;
	if (mTimeTrial)
		mRate = mRate * 2.0;

	double aCleared = (double) mBoardsCleared;
	mProgRate = (aCleared * mApp->mDiff[1] + mApp->mDiff[0]) * mRate;
	mProgDecay = (aCleared * mApp->mDiff[3] + mApp->mDiff[2]) * mRate;

	int aLevel = mSkillBase + mBoardsCleared;
	int aColors = (aLevel + 3) / 4 + 4;
	if (aColors > 8)
		aColors = 9;
	mNumColors = aColors;
	int aShapes = (aLevel * 3) / 4 + 5;
	if (aShapes > 0x15)
		aShapes = 0x16;
	mNumShapes = aShapes;
	int aChance = aLevel * -3 + 0x23;
	if (aChance < 1)
		aChance = 0;
	mBombChance = aChance;
	mSkullChance = aChance;
}

// 0x401324: a new piece, never the same shape and colour as the last
Piece* Board::NewPiece()
{
	while (true)
	{
		Piece* aPiece = new Piece(mApp);
		aPiece->Resize(0, 0, 0x4b, 0x4b);

		if (!mTutorial)
		{
			if ((mRand.Next() % 1000) < (unsigned long) mSkullChance && CountPieces() > 2)
			{
				aPiece->mSkull = true;
				aPiece->mColor = 7;
			}
			else if ((unsigned long) mBombChance <= (mRand.Next() % 1000) || CountPieces() < 3)
			{
				aPiece->mShape = (int) (mRand.Next() % mNumShapes);
				aPiece->mColor = (int) (mRand.Next() % mNumColors);
			}
			else
			{
				aPiece->mBomb = true;
				aPiece->mColor = 7;
			}
		}
		else
		{
			switch (mTutStep)
			{
			case 2:
				if (mTutCount == 0)
					aPiece->mColor = 1;
				else if (mTutCount == 1)
					aPiece->mShape = 1;
				else if (mTutCount == 2)
				{
					aPiece->mColor = 2;
					aPiece->mShape = 2;
				}
				else if (mTutCount == 3)
				{
					aPiece->mColor = 3;
					aPiece->mShape = 2;
				}
				break;
			case 4:
				aPiece->mColor = 5;
				aPiece->mShape = 7;
				break;
			case 7:
				aPiece->mColor = 4;
				aPiece->mShape = 2;
				break;
			case 9:
				aPiece->mShape = 5;
				aPiece->mColor = (int) (mRand.Next() % mNumColors);
				break;
			case 0xb:
				aPiece->mSkull = true;
				aPiece->mColor = 7;
				break;
			case 0xc:
				aPiece->mBomb = true;
				aPiece->mColor = 7;
				break;
			}
		}

		if (mTutorial || mLastColor != aPiece->mColor || mLastShape != aPiece->mShape)
		{
			mLastColor = aPiece->mColor;
			mLastShape = aPiece->mShape;
			return aPiece;
		}
		delete aPiece;
	}
}

// 0x401c9b: the piece to place is replaced by a new one that grows from nothing
void Board::NewCurrentPiece()
{
	if (mCurPiece != NULL)
	{
		if (mWidgetManager != NULL)
			mWidgetManager->RemoveWidget(mCurPiece);
		delete mCurPiece;
	}
	mCurPiece = NewPiece();
	if (mWidgetManager != NULL)
	{
		mWidgetManager->AddWidget(mCurPiece);
		mWidgetManager->PutBehind(mCurPiece, mApp->mEffects);
		mCurPiece->mShrink = 0x14;
		mCurPiece->mShrinkDir = -1;
	}
	ComputeValid();
}

// 0x40153a: reasons it does not fit: 0 outside, 1 no piece next to the cell, 2 and 3 a neighbour of another colour
// and symbol (3: some others match), 4 the cell is taken, 5 (a bomb) there is nothing to blast
bool Board::CanPlace(Piece* thePiece, int theCol, int theRow, int* theReason)
{
	if (thePiece == NULL)
	{
		*theReason = 2;
		return false;
	}

	*theReason = 0;
	if (theCol < 0 || theRow < 0 || theCol >= COLS || theRow >= ROWS)
		return false;

	if (!thePiece->mBomb)
	{
		if (mGrid[theRow][theCol] != NULL)
		{
			*theReason = 4;
			return false;
		}

		if (CountPieces() == 0)
			return true;

		static const int aOffsets[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
		int aMatches = 0;
		int aMismatches = 0;
		for (int i = 0; i < 4; i++)
		{
			int aCol = theCol + aOffsets[i][0];
			int aRow = theRow + aOffsets[i][1];
			if (aRow < 0 || aCol < 0 || aCol >= COLS || aRow >= ROWS)
				continue;
			Piece* aNeighbor = mGrid[aRow][aCol];
			if (aNeighbor == NULL)
				continue;

			if (!aNeighbor->mSkull && !thePiece->mSkull && thePiece->mColor != aNeighbor->mColor &&
				thePiece->mShape != aNeighbor->mShape)
				aMismatches++;
			else
				aMatches++;
		}

		if (aMismatches < 1)
		{
			if (aMatches != 0)
				return true;
			*theReason = 1;
		}
		else
			*theReason = (aMatches > 0) + 2;
		return false;
	}

	if (mGrid[theRow][theCol] != NULL)
		return true;
	*theReason = 5;
	return false;
}

// 0x401682: where the piece fits; a piece that fits nowhere shows its red ring on the beginner level
void Board::ComputeValid()
{
	mNumValid = 0;
	int aReason;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			mValid[aRow][aCol] = CanPlace(mCurPiece, aCol, aRow, &aReason);
			if (mValid[aRow][aCol])
				mNumValid++;
		}

	if (mCurPiece != NULL && mApp->mSkillLevel == 1 && !mTimeTrial)
		mCurPiece->mStuck = (mNumValid == 0);
}

// 0x4017ba
int Board::CountValid()
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mValid[aRow][aCol])
				aCount++;
	return aCount;
}

// 0x4017dd
void Board::ClearGrid()
{
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = NULL;
			mLit[aRow][aCol] = false;
			mFlash[aRow][aCol] = false;
			mValid[aRow][aCol] = false;
		}
}

// 0x40182f: the wizard slides in over the left of the board with a message
void Board::ShowHelpBox()
{
	mHelpBox->mFadeDir = 1;
	mWidgetManager->AddWidget(mHelpBox);
	mHelpBox->Resize(mX, mY, 0xae, 0xbc);
	mHelpShown = true;
}

// 0x4010e2: the player did something and the game is not over
bool Board::HasProgress()
{
	return !mPristine && mState != STATE_GAME_OVER && !mTutorial;
}

// 0x401130: the clock stops (and in time trial mode the board is covered)
void Board::Pause(bool isTrialMode)
{
	if (mPause == 0)
	{
		mElapsed += (GameTickCount() - mStartTick) / 1000;
		MarkDirty();
	}
	if (isTrialMode)
	{
		if (mTrialPause == 0)
			MarkDirty();
		mTrialPause++;
	}
	mPause++;
}

// 0x40118b
void Board::Resume(bool isTrialMode)
{
	if (isTrialMode)
	{
		if (mTrialPause > 0)
		{
			mTrialPause--;
			if (mTrialPause < 1)
				MarkDirty();
		}
	}
	if (mPause > 0)
	{
		mPause--;
		if (mPause == 0)
			mStartTick = GameTickCount();
	}
}

// 0x4011d5: the index of the cell among the valid ones
void Board::RecordMove(int theIndex)
{
	mRecord.WriteNumBits(theIndex, BitsRequired(mNumValid));
}

// 0x4011fb: a forge is move 0
void Board::RecordForge(bool isLastOne)
{
	RecordMove(0);
	mRecord.WriteNumBits(isLastOne ? 1 : 0, 1);
}

// 0x402f42
void Board::RecordPlacement(int theCol, int theRow)
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			if (mValid[aRow][aCol])
				aCount++;
			if (aCol == theCol && aRow == theRow)
			{
				RecordMove(aCount);
				return;
			}
		}
}

// 0x4114ff: the wizard's message of three lines (header, text, footer) as a dialog
void Board::ShowMessage(const char* thePrefix, int theFirst)
{
	std::string aPrefix = thePrefix;
	char aNum[3][2] = {{(char) ('0' + theFirst), 0}, {(char) ('1' + theFirst), 0}, {(char) ('2' + theFirst), 0}};
	SexyString aHeader = mApp->GetString(aPrefix + aNum[0]);
	SexyString aLines = mApp->GetString(aPrefix + aNum[1]);
	SexyString aFooter = mApp->GetString(aPrefix + aNum[2]);
	mApp->DoDialog(DIALOG_HINT, true, aHeader, aLines, aFooter, Dialog::BUTTONS_FOOTER);
}

// 0x40187c: a line of the statistics, the label on the left and the value on the right
void Board::DrawStatLine(Graphics* g, const Rect& theRect, int theDX, int theDY, const SexyString& theLabel,
	const SexyString& theValue)
{
	g->DrawString(theLabel, theRect.mX + 0x10 + theDX, g->GetFont()->GetAscent() + theRect.mY + theDY);
	int aWidth = g->GetFont()->StringWidth(theValue);
	g->DrawString(theValue, (theRect.mWidth - aWidth) + theRect.mX - 0x10 + theDX,
		g->GetFont()->GetAscent() + theRect.mY + theDY);
}

// 0x4019d1: a piece in its cell (the blank block as it is, a symbol in its colour)
void Board::DrawPieceImage(Graphics* g, Piece* thePiece, int theX, int theY)
{
	if (thePiece->mSkull)
	{
		g->DrawImage(mApp->mImages[IMAGE_NULLBLOCK], theX, theY);
		return;
	}

	g->SetColorizeImages(true);
	g->SetColor(Color(gPieceColors[thePiece->mColor][0], gPieceColors[thePiece->mColor][1],
		gPieceColors[thePiece->mColor][2]));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], theX + 1, theY + 1, Rect(thePiece->mShape * 0x32, 0, 0x32, 0x32));
	g->SetColorizeImages(false);
}

// 0x401a82: a cell of the picture of the board: the backdrop, the level's overlay, the tile if it was used and the
// piece. A lit tile has a half-opaque rim where it borders an unlit one.
void Board::RedrawCell(int theCol, int theRow)
{
	Graphics g(mBgImage);
	int aX = ColToX(theCol);
	int aY = RowToY(theRow);
	g.ClipRect(aX, aY, CELL, CELL);
	g.DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0x96, 0, 0x1ea, 0x1e0));

	int aLevel = mBoardsCleared % 9;
	if (aLevel > 0)
		g.DrawImage(mApp->mImages[IMAGE_LEVEL2 + aLevel - 1], GRID_X, GRID_Y);

	if (mLit[theRow][theCol])
	{
		int aDX = aX;
		int aDY = aY;
		int aWidth = CELL;
		int aHeight = CELL;
		if (theCol > 1 && !mLit[theRow][theCol - 1])
		{
			aDX++;
			aWidth = 0x33;
		}
		if (theCol < 8 && !mLit[theRow][theCol + 1])
			aWidth--;
		if (theRow > 1 && !mLit[theRow - 1][theCol])
		{
			aDY++;
			aHeight = 0x33;
		}
		if (theRow < 7 && !mLit[theRow + 1][theCol])
			aHeight--;

		g.SetColorizeImages(true);
		g.SetColor(Color(255, 255, 255, 0x80));
		g.DrawImage(mApp->mBackdrops[aLevel], GRID_X, GRID_Y);
		g.SetColorizeImages(false);
		g.DrawImage(mApp->mBackdrops[aLevel], aDX, aDY, Rect(aDX - GRID_X, aDY - GRID_Y, aWidth, aHeight));
	}

	Piece* aPiece = mGrid[theRow][theCol];
	if (aPiece != NULL && mState != STATE_ENDING)
		DrawPieceImage(&g, aPiece, aX, aY);
}

// 0x40204a
void Board::RedrawAllCells()
{
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			RedrawCell(aCol, aRow);
}

// 0x401d15: the piece goes into the cell; a tile that was not lit starts to glow in
void Board::PlacePiece(Piece* thePiece, int theCol, int theRow)
{
	bool wasLit = mLit[theRow][theCol];
	mLit[theRow][theCol] = true;
	mGrid[theRow][theCol] = NULL;
	RedrawCell(theCol, theRow);

	if (!wasLit && mAnimateLights)
	{
		mLightMix = 0;
		mLightFading = false;
		mLightMixStep = 0.1;
		mLightOn = true;
		mLightCol = theCol;
		mLightRow = theRow;
		mLightPos = 0;
		mLightFade = 0;
		mLightVariant = 0;

		int aX = -ColToX(theCol);
		int aY = -RowToY(theRow);
		{
			Graphics g(mLightLit);
			g.DrawImage(mBgImage, aX, aY);
		}
		{
			Graphics g(mLightBase);
			g.DrawImage(mApp->mImages[IMAGE_BACKDROP], aX, aY, Rect(0x96, 0, 0x1ea, 0x1e0));
		}
		{
			Graphics g(mLightOut);
			g.DrawImage(mApp->mImages[IMAGE_BACKDROP], aX, aY, Rect(0x96, 0, 0x1ea, 0x1e0));
		}
	}

	mGrid[theRow][theCol] = thePiece;
	thePiece->mShrink = 0;
	thePiece->mShrinkDir = 0;
	thePiece->mCol = theCol;
	thePiece->mRow = theRow;
	thePiece->mX = ColToX(theCol) + mX;
	thePiece->mY = RowToY(theRow) + mY;
	RedrawCell(theCol, theRow);
}

// 0x406347: the piece is put down where it hovers
void Board::PlaceCurrentPiece()
{
	bool isBombUsed = false;
	int aCol = mCurPiece->mCol;
	int aRow = mCurPiece->mRow;
	RecordPlacement(aCol, aRow);

	// the tutorial's row has to be finished in the one place
	if (mTutorial && mTutStep == 4 && (aRow != 5 || aCol != 8))
	{
		ShowMessage("TUTORIAL_CLEAR_ROW", 2);
		return;
	}

	mRun++;
	mHintOn = false;
	mHintTimer = 0;
	mPristine = false;
	mMoves++;
	bool checkAllLit = false;

	if (!mCurPiece->mBomb)
	{
		mClearCol = -1;
		mClearRow = -1;
		bool isCleared = false;
		bool wasLit = mLit[aRow][aCol];
		PlacePiece(mCurPiece, aCol, aRow);

		// the points: more for a piece that joins more pieces
		static const int aNeighbors[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
		int aJoined = 0;
		for (int i = 0; i < 4; i++)
		{
			int aNCol = aNeighbors[i][0] + aCol;
			int aNRow = aNeighbors[i][1] + aRow;
			if (aNRow > -1 && aNCol > -1 && aNCol < COLS && aNRow < ROWS && mGrid[aNRow][aNCol] != NULL)
				aJoined++;
		}
		if (aJoined > 1)
			mMultiMoves++;

		static const int aLitPoints[5] = {2, 2, 4, 6, 8};
		static const int aNewPoints[5] = {10, 10, 30, 60, 100};
		int aPoints = aNewPoints[aJoined];
		int aPointsLit = aLitPoints[aJoined];

		bool isRowFull = true;
		for (int i = 0; i < COLS; i++)
			if (mGrid[aRow][i] == NULL)
				isRowFull = false;
		if (isRowFull)
		{
			aPoints += 100;
			aPointsLit += 10;
			isCleared = true;
			mClearRow = aRow;
		}

		bool isColFull = true;
		for (int i = 0; i < ROWS; i++)
			if (mGrid[i][aCol] == NULL)
				isColFull = false;
		if (isColFull)
		{
			aPoints += 100;
			aPointsLit += 10;
			isCleared = true;
			mClearCol = aCol;
		}

		if (isRowFull && isColFull)
		{
			aPoints += 300;
			aPointsLit += 0x1e;
		}

		if (wasLit)
			aPoints = aPointsLit;

		int aScore = (int) (aPoints * mRate);
		mScore += aScore;
		mApp->mEffects->AddScorePopup(mCurPiece->mX + 0x19, (mCurPiece->mY - mY) - 7, aScore);

		if (!isCleared)
			checkAllLit = true;
		else
		{
			mLightOn = false;
			mApp->PlaySample(SOUND_FINALVANISH);
			mProgress = 1.0;
			mAnimPct = 0;
			mState = STATE_CLEARING;
			mAnimDir = 1;
		}
	}
	else
	{
		// a bomb takes the piece in the cell with it
		if (mLightOn && mLightCol == aCol && mLightRow == aRow)
			mLightOn = false;
		delete mGrid[aRow][aCol];
		mGrid[aRow][aCol] = NULL;
		mBombOn = true;
		mBombX = ColToX(aCol) - 6;
		mBombY = RowToY(aRow) - 0x1e;
		mBombFrame = 0;
		for (int i = 0; i < 6; i++)
		{
			double anAngle = i * 1.0471966666666666;
			BombSpark aSpark;
			aSpark.mLife = 0x19;
			aSpark.mX = ColToX(aCol) + 0x19;
			aSpark.mY = RowToY(aRow) + 0x19;
			aSpark.mVX = cos(anAngle) * 6.0;
			aSpark.mVY = sin(anAngle) * 6.0;
			mSparks.push_back(aSpark);
		}
		isBombUsed = true;
		mApp->PlaySample(SOUND_SKULL);
	}

	mApp->mSidePanel->SetFlashing(false);
	MarkDirty();
	mApp->PlaySample(SOUND_THUNK);

	if (mProgress > 0.0)
	{
		if (!mTimeTrial)
			mApp->PlaySample(SOUND_FORGEDOWN);
		mProgress = mProgress - 1.0;
		if (mProgress < 0.01)
			mProgress = 0;
	}

	// the cell and the lit cells around it look different now
	RedrawCell(aCol, aRow);
	if (aCol > 0 && mLit[aRow][aCol - 1])
		RedrawCell(aCol - 1, aRow);
	if (aCol < 8 && mLit[aRow][aCol + 1])
		RedrawCell(aCol + 1, aRow);
	if (aRow > 0 && mLit[aRow - 1][aCol])
		RedrawCell(aCol, aRow - 1);
	if (aRow < 7 && mLit[aRow + 1][aCol])
		RedrawCell(aCol, aRow + 1);

	mWidgetManager->RemoveWidget(mCurPiece);
	if (isBombUsed)
		delete mCurPiece;
	mCurPiece = NULL;

	if (!mTutorial)
		NewCurrentPiece();
	else
	{
		mTutCount++;
		switch (mTutStep)
		{
		case 0:
		case 7:
		case 9:
		case 0xb:
		case 0xc:
			AdvanceTutorial();
			break;
		case 2:
			if (mTutCount != 4)
				NewCurrentPiece();
			else
				AdvanceTutorial();
			break;
		case 4:
			break;
		default:
			NewCurrentPiece();
			break;
		}
	}

	if (checkAllLit)
		CheckAllLit();

	if (isBombUsed && mCurPiece != NULL)
		mCurPiece->mDelay = 0x32;
}

// 0x405e31: the piece is thrown into the forge (a second piece of the same colours could not be thrown now)
void Board::UseForge()
{
	if (mState != STATE_PLAYING || mCurPiece == NULL)
		return;

	if (mTutorial && mTutStep != 6)
	{
		ShowMessage("TUTORIAL_NO_DISCARD");
		return;
	}

	if (mFxPiece != NULL)
	{
		mWidgetManager->RemoveWidget(mFxPiece);
		delete mFxPiece;
		mFxPiece = NULL;
	}

	if (mLongestRun < mRun)
		mLongestRun = mRun;
	mDiscards++;
	mRun = 0;
	mPristine = false;
	if (mApp->mSidePanel->mOnTarget)
		mApp->mSidePanel->mHold = 0x1e;

	mFxPiece = mCurPiece;
	mCurPiece->mFollowMouse = false;
	mCurPiece->SetVisible(true);
	mFxPiece->mFlyX = (double) mFxPiece->mX;
	mFxPiece->mFlyY = (double) mFxPiece->mY;
	mFxPiece->mFlyVX = (38.0 - mFxPiece->mFlyX) * 0.05;
	mFxPiece->mFlyVY = (320.0 - mFxPiece->mFlyY) * 0.05;
	mFxPiece->mSpeed = sqrt(mFxPiece->mFlyVY * mFxPiece->mFlyVY + mFxPiece->mFlyVX * mFxPiece->mFlyVX);
	mCurPiece = NULL;

	// the piece's picture melts later: it is made here in a picture of its own
	mFxImage->Clear();
	{
		Graphics g(mFxImage);
		if (!mFxPiece->mBomb)
		{
			if (mFxPiece->mSkull)
				g.DrawImage(mApp->mImages[IMAGE_NULLBLOCK], 0, 0);
			else
			{
				g.SetColorizeImages(true);
				g.SetColor(Color(gPieceColors[mFxPiece->mColor][0], gPieceColors[mFxPiece->mColor][1],
					gPieceColors[mFxPiece->mColor][2]));
				g.DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0, 0, Rect(mFxPiece->mShape * 0x32, 0, 0x32, 0x32));
				g.SetColorizeImages(false);
			}
		}
		else
			g.DrawImage(mApp->mImages[IMAGE_SKULLS], 0, 0);
	}
	mFxImage->BitsChanged();
	mFxPiece->mImage = mFxImage;

	mProgress += mTimeTrial ? 0.5 : 1.0;
	if ((mProgress >= 3.01 && !mTimeTrial) || mProgress >= 3.49)
	{
		RecordForge(true);
		mProgress = 3.0;
		GameOver();
	}
	else
	{
		RecordForge(false);
		mNewPieceDelay = 0x19;
		mApp->PlaySample(SOUND_MELT);
		mCurPiece = NULL;
	}
}

// 0x4061d6
void Board::UseForgeClicked()
{
	if (mCurPiece != NULL)
		UseForge();
}

// 0x4062e5: every tile is lit: the board is won (with the points of the bonus)
bool Board::CheckAllLit()
{
	bool isAllLit = true;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (!mLit[aRow][aCol])
				isAllLit = false;

	if (isAllLit)
	{
		mScore += (int) (((mBoardsCleared + 4) * 250) * mRate);
		GameWon();
	}
	return isAllLit;
}

// 0x403c24: the hint costs points; it shows where the piece fits, or says why it does not
void Board::ShowHint()
{
	if (mState != STATE_PLAYING || mCurPiece == NULL || mTutorial)
		return;

	mScore = (int) (mScore - mRate * 50.0);
	if (mScore < 0)
		mScore = 0;

	if (mCurPiece->mBomb)
		ShowMessage("SKULL_BOMB");
	else if (mCurPiece->mSkull)
		ShowMessage("FOURWAY");
	else if (CountValid() != 0)
	{
		mHintOn = true;
		mHintTimer = 0xd2;
		MarkDirty();
	}
	else
		ShowMessage("NOMOVES");
}

// 0x403ee5: the rank for the score, and the score of the next one
void Board::ComputeRank()
{
	if (mLongestRun < mRun)
		mLongestRun = mRun;
	mNextRank = 0;
	mTotalSeconds = (GameTickCount() - mStartTick) / 1000 + mElapsed;

	StringVector aRanks = mApp->GetStringVector("gRankNames");
	for (int i = 21; i >= 0; i--)
	{
		if (gRankScores[i] <= mScore)
		{
			if (i < (int) aRanks.size())
				mRankName = aRanks[i];
			return;
		}
		mNextRank = gRankScores[i];
	}
}

// 0x403f5c: the forge overflowed (or the time ran out): the pieces shrink away
void Board::GameOver()
{
	mOverFade = 0;
	mOverTimer = 200;
	ComputeRank();
	ShowHelpBox();
	mApp->mSidePanel->SetFlashing(false);
	mApp->mTimesPlayed++;
	mRecord.WriteLong(mScore);
	mLightOn = false;
	mState = STATE_ENDING;
	RedrawAllCells();
	if (mCurPiece != NULL)
	{
		mWidgetManager->RemoveWidget(mCurPiece);
		delete mCurPiece;
		mCurPiece = NULL;
	}
	mApp->PlaySample(SOUND_ENDOFGAME);
}

// 0x406209: every tile of the board is lit
void Board::GameWon()
{
	ComputeRank();
	mAnimDir = -1;
	mHoverX = -1;
	mHoverY = -1;
	mRun = 0;
	mState = STATE_WON;
	mAnimPct = 0;
	mLightOn = false;
	mApp->PlaySample(SOUND_FINALGONG);
	mCurPiece->mColor = 7;
	mCurPiece->mBomb = false;
	mCurPiece->mSkull = true;
	mCurPiece->mFollowMouse = false;
	mCurPiece->SetVisible(false);
}

// 0x402292: the click on the statistics panel: the board is wiped and the next one fades in
void Board::StartNextBoard()
{
	RedrawAllCells();
	mWidgetManager->RemoveWidget(mButton);
	mApp->SafeDeleteWidget(mButton);
	mButton = NULL;
	mApp->PlaySample(SOUND_FINALVANISH2);
	mNextBoardFade = 0;
	mState = STATE_NEXT_BOARD;
	mHelpBox->mFadeDir = -1;
	memset(mLit, 0, sizeof(mLit));
	ComputeValid();
}

// 0x401014
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	if (mCurPiece != NULL)
	{
		theWidgetManager->AddWidget(mCurPiece);
		theWidgetManager->PutBehind(mCurPiece, mApp->mEffects);
	}
}

// 0x40105c
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mCurPiece != NULL)
		theWidgetManager->RemoveWidget(mCurPiece);
	if (mFxPiece != NULL)
		theWidgetManager->RemoveWidget(mFxPiece);
	if (mButton != NULL)
		theWidgetManager->RemoveWidget(mButton);
	theWidgetManager->RemoveWidget(mHelpBox);
}

// 0x40578f: the picture of the board is made when the size is known
void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mBgImage->Create(mWidth, mHeight);
	mBgImage->SetImageMode(false, false);
	{
		Graphics g(mBgImage);
		g.DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0x96, 0, 0x1ea, 0x1e0));
	}
	RedrawAllCells();

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mGrid[aRow][aCol] != NULL)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				aPiece->mX = ColToX(aPiece->mCol) + mX;
				aPiece->mY = RowToY(aPiece->mRow) + mY;
			}

	if (mTutorial && mTutStep == -1)
		AdvanceTutorial();
}

// 0x4017a0: any key stops the clock and covers the board, the next one goes on
void Board::KeyChar(SexyChar theChar)
{
	if (mTrialPause > 0)
		Resume(true);
	else
		Pause(true);
}

// 0x401707: the piece follows the mouse; the cell under its middle is the one it would go to
void Board::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
	Piece* aPiece = mCurPiece;
	if (aPiece == NULL)
		return;

	int aCol = XToCol((aPiece->mX - mX) + 0x25);
	int aRow = YToRow((aPiece->mY - mY) + 0x25);
	int aReason;
	if (!CanPlace(aPiece, aCol, aRow, &aReason) || aPiece->mBomb)
	{
		mHoverX = -1;
		mHoverY = -1;
	}
	else
	{
		mHoverX = aCol;
		mHoverY = aRow;
	}
	aPiece->mCol = aCol;
	aPiece->mRow = aRow;
}

// 0x4068f1: a click puts the piece down, or tells why it cannot be; the right button uses the forge
void Board::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);

	if (mTrialPause > 0)
	{
		Resume(true);
		return;
	}
	if (mCurPiece == NULL || mState != STATE_PLAYING)
		return;

	if (theClickCount < 0)
	{
		UseForgeClicked();
		return;
	}

	MouseMove(x, y);
	int aRow = mCurPiece->mRow;
	int aCol = mCurPiece->mCol;
	int aReason;
	if (CanPlace(mCurPiece, aCol, aRow, &aReason))
	{
		if (aCol > -1 && aCol < COLS && aRow > -1 && aRow < ROWS)
			PlaceCurrentPiece();
		MouseMove(x, y);
		return;
	}

	if (!mTutorial)
	{
		if (aReason == 3 && mMultiMoves < 2)
			ShowMessage("INVALID_MOVE_SOME_MISMATCH");
		else if (mNumValid == 0 && mDiscards == 0)
			ShowMessage("INVALID_MOVE_MUST_DISCARD");
		else if (mMoves != 0)
		{
			if (mMoves <= 5)
				ShowMessage("INVALID_MOVE_NOTHING_NEARBY");
		}
		else
			ShowMessage("INVALID_MOVE_MUST_START_BY_CENTER");
	}
	else
	{
		if (aReason == 2 || aReason == 3)
			ShowMessage("INVALID_MOVE_NON_ADJACENT_COLOR");
		else if (aReason == 1)
			ShowMessage("INVALID_MOVE_NO_ADJACENT_SYMBOL");
		else if (aReason == 4)
			ShowMessage("INVALID_MOVE_NON_EMPTY_SPOT");
		else if (aReason == 5)
			ShowMessage("INVALID_MOVE_BOMB");
		else
			return;
	}
	mApp->PlaySample(SOUND_BAD);
}

// 0x40186d: the board's buttons do not click
void Board::ButtonPress(int theId)
{
}

// 0x4053aa: 0 the panel: the next board; 1 the end of the tutorial; 2 the next tutorial step
void Board::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		StartNextBoard();
		break;
	case 1:
		mProgress = 0.0;
		mApp->ShowSelector();
		break;
	case 2:
		AdvanceTutorial();
		break;
	}
}
