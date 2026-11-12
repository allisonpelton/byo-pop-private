// The board's rules: the grid, the pieces, placing and forging, and how a game goes on and ends (see Board.h).

#include "Board.h"
#include "WinAlchApp.h"
#include "Piece.h"
#include "platform/GameOptions.h"
#include "Wizard.h"
#include "BoardOverlay.h"
#include "Sidebar.h"
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
Board::Board(WinAlchApp* theApp)
{
	mApp = theApp;
	mState = STATE_PLAYING;
	mSavedZero = 0;
	mCurPiece = NULL;
	mDiscardPiece = NULL;
	mButton = NULL;

	// the original seeds its generator with two draws of the C library's rand()
	unsigned long aSeed = (unsigned long) (CrtRand() << 16);
	aSeed |= (unsigned long) (CrtRand() & 0xffff);
	SeedMTRand2004(mRand, aSeed);

	// the header of the record: what an internet score would be checked with
	mEncodedGame.WriteByte(1);
	mEncodedGame.WriteString("");
	mEncodedGame.WriteString("");
	mEncodedGame.WriteLong((long) aSeed);
	mEncodedGame.WriteBoolean(mApp->mTimeTrial);
	mEncodedGame.WriteNumBits(mApp->mSkillLevel, 2);
	mEncodedGame.WriteLong((long) time(NULL));

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
	mGameOverFade = 0;
	mGameOverTimer = 0;

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
	mDiscardImage = new MemoryImage(mApp);
	mDiscardImage->Create(WinAlchApp::SYMBOL_SIZE, WinAlchApp::SYMBOL_SIZE);
	mDiscardImage->SetImageMode(true, true);

	mTutorial = mApp->mTutorialMode;
	mTutorialStep = mTutorial ? -1 : 0;
	mTutorialCount = 0;
	mStartTick = GameTickCount();
	mElapsed = 0;
	mTotalSeconds = 0;
	mPauseCount = 0;
	mCoverCount = 0;
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
	mTutorialWaiting = false;
	mRate = 1.0;
	mLastShape = -1;
	mLastColor = -1;
	mWizardShown = false;

	SetBoardVars();

	mWizard = new Wizard(mApp);

	// a game starts with a blank block in the middle of the grid
	if (!mApp->mTutorialMode)
	{
		Piece* aPiece = new Piece(mApp);
		aPiece->mCol = FIRST_COL;
		aPiece->mRow = FIRST_ROW;
		aPiece->mX = GetColX(FIRST_COL) + mX;
		aPiece->mY = GetRowY(FIRST_ROW) + mY;
		aPiece->mSkull = true;
		mGrid[FIRST_ROW][FIRST_COL] = aPiece;
		mLit[FIRST_ROW][FIRST_COL] = true;
	}

	NextMove();
}

// 0x4026fa
Board::~Board()
{
	if (mApp->mSidebar != NULL)
		mApp->mSidebar->FlashLava(false);

	// a game that was nearly lost counts as played
	if (mProgress >= 2.9 && mState != STATE_GAME_OVER)
		mApp->mTimesPlayed++;

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			delete mGrid[aRow][aCol];
	delete mCurPiece;
	delete mDiscardPiece;
	delete mButton;
	delete mWizard;
	delete mBgImage;
	delete mDiscardImage;
	delete mLightBase;
	delete mLightLit;
	delete mLightOut;
}

// 0x4014f3
int Board::GetColAt(int theX)
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
int Board::GetRowAt(int theY)
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
void Board::SetBoardVars()
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
	mProgRate = (aCleared * mApp->mForgeRates[1] + mApp->mForgeRates[0]) * mRate;
	mProgDecay = (aCleared * mApp->mForgeRates[3] + mApp->mForgeRates[2]) * mRate;

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
Piece* Board::MakeRandomPiece()
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
			switch (mTutorialStep)
			{
			case 2:
				if (mTutorialCount == 0)
					aPiece->mColor = 1;
				else if (mTutorialCount == 1)
					aPiece->mShape = 1;
				else if (mTutorialCount == 2)
				{
					aPiece->mColor = 2;
					aPiece->mShape = 2;
				}
				else if (mTutorialCount == 3)
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
				aPiece->mColor = 3;   // 0x4013bb jumps to the store at 0x40149b with edx = 3: no random draw
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
void Board::NextMove()
{
	if (mCurPiece != NULL)
	{
		if (mWidgetManager != NULL)
			mWidgetManager->RemoveWidget(mCurPiece);
		delete mCurPiece;
	}
	mCurPiece = MakeRandomPiece();
	if (mWidgetManager != NULL)
	{
		mWidgetManager->AddWidget(mCurPiece);
		mWidgetManager->PutBehind(mCurPiece, mApp->mBoardOverlay);
		mCurPiece->mShrink = 0x14;
		mCurPiece->mShrinkDir = -1;
	}
	DetermineValidMoves();
}

// 0x40153a: reasons it does not fit: 0 outside, 1 no piece next to the cell, 2 and 3 a neighbour of another colour
// and symbol (3: some others match), 4 the cell is taken, 5 (a bomb) there is nothing to blast
bool Board::ValidMove(Piece* thePiece, int theCol, int theRow, int* theReason)
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
void Board::DetermineValidMoves()
{
	mNumValid = 0;
	int aReason;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			mValid[aRow][aCol] = ValidMove(mCurPiece, aCol, aRow, &aReason);
			if (mValid[aRow][aCol])
				mNumValid++;
		}

	if (mCurPiece != NULL && mApp->mSkillLevel == 1 && !mTimeTrial)
		mCurPiece->mStuck = (mNumValid == 0);
}

// 0x4017ba
int Board::GetNumPossibleMoves()
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mValid[aRow][aCol])
				aCount++;
	return aCount;
}

// 0x4017dd
void Board::ResetBoard()
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
void Board::ShowWizard()
{
	mWizard->mFadeDir = 1;
	mWidgetManager->AddWidget(mWizard);
	mWizard->Resize(mX, mY, 0xae, 0xbc);
	mWizardShown = true;
}

// 0x4010e2: the player did something and the game is not over
bool Board::IsInProgress()
{
	return !mPristine && mState != STATE_GAME_OVER && !mTutorial;
}

// 0x401130: the clock stops (and in time trial mode the board is covered)
void Board::Pause(bool isCovered)
{
	if (mPauseCount == 0)
	{
		mElapsed += (GameTickCount() - mStartTick) / 1000;
		MarkDirty();
	}
	if (isCovered)
	{
		if (mCoverCount == 0)
			MarkDirty();
		mCoverCount++;
	}
	mPauseCount++;
}

// 0x40118b
void Board::Unpause(bool isCovered)
{
	if (isCovered)
	{
		if (mCoverCount > 0)
		{
			mCoverCount--;
			if (mCoverCount < 1)
				MarkDirty();
		}
	}
	if (mPauseCount > 0)
	{
		mPauseCount--;
		if (mPauseCount == 0)
			mStartTick = GameTickCount();
	}
}

// 0x4011d5: the index of the cell among the valid ones
void Board::EncodeMoveNum(int theIndex)
{
	mEncodedGame.WriteNumBits(theIndex, BitsRequired(mNumValid));
}

// 0x4011fb: a forge is move 0
void Board::EncodeDiscard(bool isLastOne)
{
	EncodeMoveNum(0);
	mEncodedGame.WriteNumBits(isLastOne ? 1 : 0, 1);
}

// 0x402f42
void Board::EncodeMove(int theCol, int theRow)
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			if (mValid[aRow][aCol])
				aCount++;
			if (aCol == theCol && aRow == theRow)
			{
				EncodeMoveNum(aCount);
				return;
			}
		}
}

// port: the wizard's message whose three texts (header, text, footer) are thePrefix + "1".."3", or from theFirst on
void Board::GiveHelp(const char* thePrefix, int theFirst)
{
	std::string aPrefix = thePrefix;
	char aNum[3][2] = {{(char) ('0' + theFirst), 0}, {(char) ('1' + theFirst), 0}, {(char) ('2' + theFirst), 0}};
	SexyString aHeader = mApp->GetString(aPrefix + aNum[0]);
	SexyString aLines = mApp->GetString(aPrefix + aNum[1]);
	SexyString aFooter = mApp->GetString(aPrefix + aNum[2]);
	mApp->GiveHelp(aHeader, aLines, aFooter);
}

// 0x40187c: a line of the statistics, the label on the left and the value on the right
void Board::DrawColumn(Graphics* g, const Rect& theRect, int theDX, int theDY, const SexyString& theLabel,
	const SexyString& theValue)
{
	g->DrawString(theLabel, theRect.mX + 0x10 + theDX, g->GetFont()->GetAscent() + theRect.mY + theDY);
	int aWidth = g->GetFont()->StringWidth(theValue);
	g->DrawString(theValue, (theRect.mWidth - aWidth) + theRect.mX - 0x10 + theDX,
		g->GetFont()->GetAscent() + theRect.mY + theDY);
}

// 0x4019d1: a piece in its cell (the blank block as it is, a symbol in its colour)
void Board::DrawAliasedPiece(Graphics* g, Piece* thePiece, int theX, int theY)
{
	if (thePiece->mSkull)
	{
		g->DrawImage(mApp->mImages[IMAGE_NULLBLOCK], theX, theY);
		return;
	}

	g->SetColorizeImages(true);
	g->SetColor(Color(gPieceColors[thePiece->mColor][0], gPieceColors[thePiece->mColor][1],
		gPieceColors[thePiece->mColor][2]));
	g->DrawImage(mApp->mImages[IMAGE_SYMBOLS], theX + 1, theY + 1, Rect(thePiece->mShape * WinAlchApp::SYMBOL_SIZE, 0, WinAlchApp::SYMBOL_SIZE, WinAlchApp::SYMBOL_SIZE));
	g->SetColorizeImages(false);
}

// 0x401a82: a cell of the picture of the board: the backdrop, the level's overlay, the tile if it was used and the
// piece. A lit tile has a half-opaque rim where it borders an unlit one.
void Board::RedrawBackground(int theCol, int theRow)
{
	Graphics g(mBgImage);
	int aX = GetColX(theCol);
	int aY = GetRowY(theRow);
	g.ClipRect(aX, aY, CELL, CELL);
	g.DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(WinAlchApp::BOARD_X, 0, WinAlchApp::BOARD_WIDTH, WinAlchApp::SCREEN_HEIGHT));

	int aLevel = mBoardsCleared % 9;
	if (aLevel > 0)
		g.DrawImage(mApp->mImages[IMAGE_LEVEL2 + aLevel - 1], GRID_X, GRID_Y);

	if (mLit[theRow][theCol])
	{
		int aDX = aX;
		int aDY = aY;
		int aWidth = CELL;
		int aHeight = CELL;
		// the original tests the left and top neighbours from column and row 2 on, the right and bottom ones from the last
		// column and row back: the first column and row get no rim where they border an unlit tile (fix "rims")
		int aFirst = Platform::FixOn("rims") ? 0 : 1;
		if (theCol > aFirst && !mLit[theRow][theCol - 1])
		{
			aDX++;
			aWidth = 0x33;
		}
		if (theCol < 8 && !mLit[theRow][theCol + 1])
			aWidth--;
		if (theRow > aFirst && !mLit[theRow - 1][theCol])
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
		DrawAliasedPiece(&g, aPiece, aX, aY);
}

// 0x40204a
void Board::RedrawBackground()
{
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			RedrawBackground(aCol, aRow);
}

// 0x401d15: the piece goes into the cell; a tile that was not lit starts to glow in
void Board::PutPiece(Piece* thePiece, int theCol, int theRow)
{
	bool wasLit = mLit[theRow][theCol];
	mLit[theRow][theCol] = true;
	mGrid[theRow][theCol] = NULL;
	RedrawBackground(theCol, theRow);

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

		int aX = -GetColX(theCol);
		int aY = -GetRowY(theRow);
		{
			Graphics g(mLightLit);
			g.DrawImage(mBgImage, aX, aY);
		}
		{
			Graphics g(mLightBase);
			g.DrawImage(mApp->mImages[IMAGE_BACKDROP], aX, aY, Rect(WinAlchApp::BOARD_X, 0, WinAlchApp::BOARD_WIDTH, WinAlchApp::SCREEN_HEIGHT));
		}
		{
			Graphics g(mLightOut);
			g.DrawImage(mApp->mImages[IMAGE_BACKDROP], aX, aY, Rect(WinAlchApp::BOARD_X, 0, WinAlchApp::BOARD_WIDTH, WinAlchApp::SCREEN_HEIGHT));
		}
	}

	mGrid[theRow][theCol] = thePiece;
	thePiece->mShrink = 0;
	thePiece->mShrinkDir = 0;
	thePiece->mCol = theCol;
	thePiece->mRow = theRow;
	thePiece->mX = GetColX(theCol) + mX;
	thePiece->mY = GetRowY(theRow) + mY;
	RedrawBackground(theCol, theRow);
}

// 0x406347: the piece is put down where it hovers
void Board::MakeMove()
{
	bool isBombUsed = false;
	int aCol = mCurPiece->mCol;
	int aRow = mCurPiece->mRow;
	EncodeMove(aCol, aRow);

	// the tutorial's row has to be finished in the one place
	if (mTutorial && mTutorialStep == 4 && (aRow != 5 || aCol != 8))
	{
		GiveHelp("TUTORIAL_CLEAR_ROW", 2);
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
		PutPiece(mCurPiece, aCol, aRow);

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
		mApp->mBoardOverlay->FloatPoints(mCurPiece->mX + 0x19, (mCurPiece->mY - mY) - 7, aScore);

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
		mBombX = GetColX(aCol) - 6;
		mBombY = GetRowY(aRow) - 0x1e;
		mBombFrame = 0;
		for (int i = 0; i < 6; i++)
		{
			double anAngle = i * 1.0471966666666666;
			BombSpark aSpark;
			aSpark.mLife = 0x19;
			aSpark.mX = GetColX(aCol) + 0x19;
			aSpark.mY = GetRowY(aRow) + 0x19;
			aSpark.mVX = cos(anAngle) * 6.0;
			aSpark.mVY = sin(anAngle) * 6.0;
			mSparks.push_back(aSpark);
		}
		isBombUsed = true;
		mApp->PlaySample(SOUND_SKULL);
	}

	mApp->mSidebar->FlashLava(false);
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
	RedrawBackground(aCol, aRow);
	if (aCol > 0 && mLit[aRow][aCol - 1])
		RedrawBackground(aCol - 1, aRow);
	if (aCol < 8 && mLit[aRow][aCol + 1])
		RedrawBackground(aCol + 1, aRow);
	if (aRow > 0 && mLit[aRow - 1][aCol])
		RedrawBackground(aCol, aRow - 1);
	if (aRow < 7 && mLit[aRow + 1][aCol])
		RedrawBackground(aCol, aRow + 1);

	mWidgetManager->RemoveWidget(mCurPiece);
	if (isBombUsed)
		delete mCurPiece;
	mCurPiece = NULL;

	if (!mTutorial)
		NextMove();
	else
	{
		mTutorialCount++;
		switch (mTutorialStep)
		{
		case 0:
		case 7:
		case 9:
		case 0xb:
		case 0xc:
			NextTutorial();
			break;
		case 2:
			if (mTutorialCount != 4)
				NextMove();
			else
				NextTutorial();
			break;
		case 4:
			break;
		default:
			NextMove();
			break;
		}
	}

	if (checkAllLit)
		CheckWin();

	if (isBombUsed && mCurPiece != NULL)
		mCurPiece->mDelay = 0x32;
}

// 0x405e31: the piece is thrown into the forge (a second piece of the same colours could not be thrown now)
void Board::SkipMove()
{
	if (mState != STATE_PLAYING || mCurPiece == NULL)
		return;

	if (mTutorial && mTutorialStep != 6)
	{
		GiveHelp("TUTORIAL_NO_DISCARD");
		return;
	}

	if (mDiscardPiece != NULL)
	{
		mWidgetManager->RemoveWidget(mDiscardPiece);
		delete mDiscardPiece;
		mDiscardPiece = NULL;
	}

	if (mLongestRun < mRun)
		mLongestRun = mRun;
	mDiscards++;
	mRun = 0;
	mPristine = false;
	if (mApp->mSidebar->mOnTarget)
		mApp->mSidebar->mHold = 0x1e;

	mDiscardPiece = mCurPiece;
	mCurPiece->mFollowMouse = false;
	mCurPiece->SetVisible(true);
	mDiscardPiece->mFlyX = (double) mDiscardPiece->mX;
	mDiscardPiece->mFlyY = (double) mDiscardPiece->mY;
	mDiscardPiece->mFlyVX = (38.0 - mDiscardPiece->mFlyX) * 0.05;
	mDiscardPiece->mFlyVY = (320.0 - mDiscardPiece->mFlyY) * 0.05;
	mDiscardPiece->mSpeed = sqrt(mDiscardPiece->mFlyVY * mDiscardPiece->mFlyVY + mDiscardPiece->mFlyVX * mDiscardPiece->mFlyVX);
	mCurPiece = NULL;

	// the piece's picture melts later: it is made here in a picture of its own
	mDiscardImage->Clear();
	{
		Graphics g(mDiscardImage);
		if (!mDiscardPiece->mBomb)
		{
			if (mDiscardPiece->mSkull)
				g.DrawImage(mApp->mImages[IMAGE_NULLBLOCK], 0, 0);
			else
			{
				g.SetColorizeImages(true);
				g.SetColor(Color(gPieceColors[mDiscardPiece->mColor][0], gPieceColors[mDiscardPiece->mColor][1],
					gPieceColors[mDiscardPiece->mColor][2]));
				g.DrawImage(mApp->mImages[IMAGE_SYMBOLS], 0, 0, Rect(mDiscardPiece->mShape * WinAlchApp::SYMBOL_SIZE, 0, WinAlchApp::SYMBOL_SIZE, WinAlchApp::SYMBOL_SIZE));
				g.SetColorizeImages(false);
			}
		}
		else
			g.DrawImage(mApp->mImages[IMAGE_SKULLS], 0, 0);
	}
	mDiscardImage->BitsChanged();
	mDiscardPiece->mImage = mDiscardImage;

	mProgress += mTimeTrial ? 0.5 : 1.0;
	if ((mProgress >= 3.01 && !mTimeTrial) || mProgress >= 3.49)
	{
		EncodeDiscard(true);
		mProgress = 3.0;
		GameOver();
	}
	else
	{
		EncodeDiscard(false);
		mNewPieceDelay = 0x19;
		mApp->PlaySample(SOUND_MELT);
		mCurPiece = NULL;
	}
}

// 0x4061d6
void Board::Discard()
{
	if (mCurPiece != NULL)
		SkipMove();
}

// 0x4062e5: every tile is lit: the board is won (with the points of the bonus)
bool Board::CheckWin()
{
	bool isAllLit = true;
	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (!mLit[aRow][aCol])
				isAllLit = false;

	if (isAllLit)
	{
		mScore += (int) (((mBoardsCleared + 4) * 250) * mRate);
		ClearBoard();
	}
	return isAllLit;
}

// 0x403c24: the hint costs points; it shows where the piece fits, or says why it does not
void Board::ShowPossibleMoves()
{
	if (mState != STATE_PLAYING || mCurPiece == NULL || mTutorial)
		return;

	mScore = (int) (mScore - mRate * 50.0);
	if (mScore < 0)
		mScore = 0;

	if (mCurPiece->mBomb)
		GiveHelp("SKULL_BOMB");
	else if (mCurPiece->mSkull)
		GiveHelp("FOURWAY");
	else if (GetNumPossibleMoves() != 0)
	{
		mHintOn = true;
		mHintTimer = 0xd2;
		MarkDirty();
	}
	else
		GiveHelp("NOMOVES");
}

// 0x403ee5: the rank for the score, and the score of the next one
void Board::GetGameStats()
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
	mGameOverFade = 0;
	mGameOverTimer = 200;
	GetGameStats();
	ShowWizard();
	mApp->mSidebar->FlashLava(false);
	mApp->mTimesPlayed++;
	mEncodedGame.WriteLong(mScore);
	mLightOn = false;
	mState = STATE_ENDING;
	RedrawBackground();
	if (mCurPiece != NULL)
	{
		mWidgetManager->RemoveWidget(mCurPiece);
		delete mCurPiece;
		mCurPiece = NULL;
	}
	mApp->PlaySample(SOUND_ENDOFGAME);
}

// 0x406209: every tile of the board is lit
void Board::ClearBoard()
{
	GetGameStats();
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
void Board::NextBoard()
{
	RedrawBackground();
	mWidgetManager->RemoveWidget(mButton);
	mApp->SafeDeleteWidget(mButton);
	mButton = NULL;
	mApp->PlaySample(SOUND_FINALVANISH2);
	mNextBoardFade = 0;
	mState = STATE_NEXT_BOARD;
	mWizard->mFadeDir = -1;
	memset(mLit, 0, sizeof(mLit));
	DetermineValidMoves();
}

// 0x401014
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	if (mCurPiece != NULL)
	{
		theWidgetManager->AddWidget(mCurPiece);
		theWidgetManager->PutBehind(mCurPiece, mApp->mBoardOverlay);
	}
}

// 0x40105c
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mCurPiece != NULL)
		theWidgetManager->RemoveWidget(mCurPiece);
	if (mDiscardPiece != NULL)
		theWidgetManager->RemoveWidget(mDiscardPiece);
	if (mButton != NULL)
		theWidgetManager->RemoveWidget(mButton);
	theWidgetManager->RemoveWidget(mWizard);
}

// 0x40578f: the picture of the board is made when the size is known
void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mBgImage->Create(mWidth, mHeight);
	mBgImage->SetImageMode(false, false);
	{
		Graphics g(mBgImage);
		g.DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(WinAlchApp::BOARD_X, 0, WinAlchApp::BOARD_WIDTH, WinAlchApp::SCREEN_HEIGHT));
	}
	RedrawBackground();

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
			if (mGrid[aRow][aCol] != NULL)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				aPiece->mX = GetColX(aPiece->mCol) + mX;
				aPiece->mY = GetRowY(aPiece->mRow) + mY;
			}

	if (mTutorial && mTutorialStep == -1)
		NextTutorial();
}

// 0x4017a0: any key stops the clock and covers the board, the next one goes on
void Board::KeyChar(SexyChar theChar)
{
	if (mCoverCount > 0)
		Unpause(true);
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

	int aCol = GetColAt((aPiece->mX - mX) + 0x25);
	int aRow = GetRowAt((aPiece->mY - mY) + 0x25);
	int aReason;
	if (!ValidMove(aPiece, aCol, aRow, &aReason) || aPiece->mBomb)
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

	if (mCoverCount > 0)
	{
		Unpause(true);
		return;
	}
	if (mCurPiece == NULL || mState != STATE_PLAYING)
		return;

	if (theClickCount < 0)
	{
		Discard();
		return;
	}

	MouseMove(x, y);
	int aRow = mCurPiece->mRow;
	int aCol = mCurPiece->mCol;
	int aReason;
	if (ValidMove(mCurPiece, aCol, aRow, &aReason))
	{
		if (aCol > -1 && aCol < COLS && aRow > -1 && aRow < ROWS)
			MakeMove();
		MouseMove(x, y);
		return;
	}

	if (!mTutorial)
	{
		if (aReason == 3 && mMultiMoves < 2)
			GiveHelp("INVALID_MOVE_SOME_MISMATCH");
		else if (mNumValid == 0 && mDiscards == 0)
			GiveHelp("INVALID_MOVE_MUST_DISCARD");
		else if (mMoves != 0)
		{
			if (mMoves <= 5)
				GiveHelp("INVALID_MOVE_NOTHING_NEARBY");
		}
		else
			GiveHelp("INVALID_MOVE_MUST_START_BY_CENTER");
	}
	else
	{
		if (aReason == 2 || aReason == 3)
			GiveHelp("INVALID_MOVE_NON_ADJACENT_COLOR");
		else if (aReason == 1)
			GiveHelp("INVALID_MOVE_NO_ADJACENT_SYMBOL");
		else if (aReason == 4)
			GiveHelp("INVALID_MOVE_NON_EMPTY_SPOT");
		else if (aReason == 5)
			GiveHelp("INVALID_MOVE_BOMB");
		// any other reason (0, off the grid) says nothing, but the click still sounds (0x406ec7 is the common end)
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
		NextBoard();
		break;
	case 1:
		mProgress = 0.0;
		mApp->ShowIntroScreen();
		break;
	case 2:
		NextTutorial();
		break;
	}
}
