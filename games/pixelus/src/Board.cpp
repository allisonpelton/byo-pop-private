#include "Board.h"
#include "PixelusApp.h"
#include "TileMaster.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "GameText.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "WidgetManager.h"
#include "Buffer.h"
#include "GameDialog.h"
#include "Compat2004.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

using namespace Sexy;

static const double PI = 3.14159265358979323846;

static int ReverseSide(int theSide)		// 0x4218e0
{
	switch (theSide)
	{
	case 0: return 2;
	case 1: return 3;
	case 2: return 0;
	default: return 1;
	}
}

// the cell's coordinate for a pixel offset in the grid, rounding down
static int FloorDiv(int theValue, int theDivisor)
{
	return theValue >= 0 ? theValue / theDivisor : -((-theValue + theDivisor - 1) / theDivisor);
}

// 0x407f10
Board::Board(PixelusApp* theApp, const std::string& theLevelName, int thePuzzleIndex)
{
	mApp = theApp;
	mLevelName = theLevelName;
	mPuzzleIndex = thePuzzleIndex;
	mGrid = new Grid(16, 16);
	mTileMaster = NULL;
	mTutorial = (theLevelName == "Tutorial");
	mHoverCol = mHoverRow = -1;
	mHoverValid = false;
	mAlpha = 0;
	mIntro = 0;
	mPauseFade = 0.0;
	mDoneTimer = 0;
	mCompleted = false;
	mIdleTicks = 0;
	mMusicDelay = -1;
	mErrorTicks = 0;
	memset(mFlash, 0, sizeof(mFlash));
	memset(mFlashWanted, 0, sizeof(mFlashWanted));
	mHintShown = false;
	mHintTicks = 0;
	mHintMove.mSide = mHintMove.mCursor = mHintMove.mPosition = 0;
	mHintMove.mRemove = false;

	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	mExpert = aUser != NULL && aUser->mFlag38;

	mResetButton = MakeStripButton(mApp, this, ID_RESET, IMAGE_RESET, 0x12, 0x172);
	if (!mExpert)
	{
		mHintButton = MakeStripButton(mApp, this, ID_HINT, IMAGE_HINT, 0x3f, 0x172);
		mUndoButton = MakeStripButton(mApp, this, ID_UNDO, IMAGE_UNDO, 0x6c, 0x172);
	}
	else
	{
		mHintButton = NULL;
		mUndoButton = NULL;
	}
	mMenuButton = MakeStripButton(mApp, this, ID_MENU, IMAGE_MENU, 0x16, 0x1b0);
	mBackButton = MakeStripButton(mApp, this, ID_BACK, mTutorial ? IMAGE_BACKTOCAVE : IMAGE_BACKTOTEMPLE, 6, -3);

	if (LoadPuzzle())
		Restore();
}

// 0x4045e0
Board::~Board()
{
	delete mGrid;
	delete mTileMaster;
	delete mResetButton;
	delete mHintButton;
	delete mUndoButton;
	delete mMenuButton;
	delete mBackButton;
}

// 0x4028c0
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
	theWidgetManager->AddWidget(mBackButton);
	if (mUndoButton != NULL)
		theWidgetManager->AddWidget(mUndoButton);
	if (mResetButton != NULL)
		theWidgetManager->AddWidget(mResetButton);
	if (mHintButton != NULL)
		theWidgetManager->AddWidget(mHintButton);
}

// 0x402950
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
	theWidgetManager->RemoveWidget(mBackButton);
	if (mUndoButton != NULL)
		theWidgetManager->RemoveWidget(mUndoButton);
	if (mResetButton != NULL)
		theWidgetManager->RemoveWidget(mResetButton);
	if (mHintButton != NULL)
		theWidgetManager->RemoveWidget(mHintButton);
}

// 0x402a50: the way back sits in the top left corner of the board
void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mApp->mImages[IMAGE_BACKTOTEMPLE]->GetWidth() / 3;
	int aHeight = mApp->mImages[IMAGE_BACKTOTEMPLE]->GetHeight();
	mBackButton->Resize(theX + 6, theY - 3, aWidth, aHeight);
}

// 0x408f60: the puzzle of the temple's list
bool Board::LoadPuzzle()
{
	const LevelData::Level* aLevel = mApp->mLevelData->GetLevel(mLevelName);
	if (aLevel == NULL || mPuzzleIndex < 0 || mPuzzleIndex >= (int) aLevel->mPuzzles.size())
		return false;
	const LevelData::Puzzle& aPuzzle = aLevel->mPuzzles[mPuzzleIndex];

	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile("levels/" + aPuzzle.mFile, &aBuffer))
		return false;
	std::string aText((const char*) aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	if (!mGrid->LoadFromText(aText))
		return false;

	mUuid = aPuzzle.mUuid;
	mApp->mLevelData->mCurrentLevel = mLevelName;
	mApp->mLevelData->mCurrentPuzzle = mPuzzleIndex;

	for (size_t i = 0; i < mGrid->mObjects.size(); i++)
		mGrid->mObjects[i]->mVariant = CrtRand() % 5;

	delete mTileMaster;
	mTileMaster = new TileMaster(mApp, GRID_X, GRID_Y, mGrid->mCols, mGrid->mRows);
	Aim(Grid::SIDE_UP, mGrid->mCols / 2);
	mTileMaster->SetTarget(Grid::SIDE_UP, mGrid->mCols / 2, true);
	mUndo.clear();
	mCompleted = false;
	mDoneTimer = 0;
	return true;
}

// 0x408e70: the moves the player had made in this puzzle are played again
void Board::Restore()
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (aUser == NULL)
		return;
	const ProgressManager::History* aHistory = mApp->mProgress->GetHistory(aUser->mId, mUuid);
	if (aHistory == NULL)
		return;

	std::string aMoves = aHistory->mMoves;
	for (size_t i = 0; i + 6 <= aMoves.size(); i += 6)
	{
		Grid::Move aMove;
		if (!Grid::ParseMove(aMoves.substr(i, 6), &aMove))
			break;
		mGrid->SetCursor(aMove.mSide, aMove.mCursor);
		if (aMove.mRemove)
		{
			int aPosition = (aMove.mSide == Grid::SIDE_UP || aMove.mSide == Grid::SIDE_DOWN) ? mGrid->mStuckRow : mGrid->mStuckCol;
			if (!mGrid->PullTile(true, true))
				break;
			RecordUndo(aMove.mSide, aMove.mCursor, aPosition, 1);
		}
		else
		{
			bool aLeaves = !mGrid->mCanPlace && mGrid->mSlidesOut;
			bool aBounces = !aLeaves && mGrid->IsBouncyAhead();
			if (!mGrid->PlaceTile(true))
				break;
			if (aLeaves || aBounces)
				mGrid->mSupply++;
			RecordUndo(aMove.mSide, aMove.mCursor, -1, (aLeaves || aBounces) ? 2 : 0);
		}
	}

	// every tile rests where it is
	for (size_t i = 0; i < mGrid->mObjects.size(); i++)
	{
		GridObject* anObject = mGrid->mObjects[i];
		anObject->mX = anObject->mCol * Grid::CELL_SIZE;
		anObject->mY = anObject->mRow * Grid::CELL_SIZE;
		anObject->mState = 0;
		anObject->mIdle = 100;
		anObject->mTrail = 0;
		if (!mGrid->InGrid(anObject->mRow, anObject->mCol) || mGrid->mCells[anObject->mRow][anObject->mCol].mObject != anObject)
			anObject->mDead = true;
	}
	if (mGrid->mDone)
	{
		// a puzzle that was finished is not kept; play it from the start
		mApp->mProgress->RemoveHistory(aUser->mId, mUuid);
		LoadPuzzle();
	}
	Aim(Grid::SIDE_UP, mGrid->mCols / 2);
	mTileMaster->SetTarget(Grid::SIDE_UP, mGrid->mCols / 2, true);
}

// 0x405e10: the moves so far are kept for the player (an empty puzzle needs no entry)
void Board::SaveProgress()
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (aUser == NULL || mUuid.empty())
		return;
	if (mGrid->mDone || mGrid->mHistory.empty())
	{
		mApp->mProgress->RemoveHistory(aUser->mId, mUuid);
		return;
	}

	ProgressManager::History aHistory;
	for (size_t i = 0; i < mGrid->mHistory.size(); i++)
		aHistory.mMoves += Grid::MoveToString(mGrid->mHistory[i]);
	for (int r = 0; r < mGrid->mRows; r++)
		for (int c = 0; c < mGrid->mCols; c++)
			if (mGrid->mCells[r][c].mObject != NULL && !mGrid->mCells[r][c].mObject->mFixed)
			{
				char aBuf[8];
				sprintf(aBuf, "%02d%02d", r, c);
				aHistory.mTiles += aBuf;
			}
	for (size_t i = 0; i < mUndo.size(); i++)
	{
		char aBuf[16];
		sprintf(aBuf, "%d%02d%02d%d", mUndo[i].mSide, mUndo[i].mCursor, mUndo[i].mPosition < 0 ? 99 : mUndo[i].mPosition, mUndo[i].mType);
		aHistory.mUndo += aBuf;
	}
	aHistory.mCount = "0";
	mApp->mProgress->SetHistory(aUser->mId, mUuid, aHistory);
}

// 0x432fa0: start the puzzle over
void Board::RestartPuzzle()
{
	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	if (aUser != NULL)
		mApp->mProgress->RemoveHistory(aUser->mId, mUuid);
	memset(mFlash, 0, sizeof(mFlash));
	mHintShown = false;
	mIntro = 0;
	mAlpha = 0;
	LoadPuzzle();
}

int Board::GetMoves() const
{
	return mGrid->mMoveCount;
}

// 0x424590 and 0x42aec0
void Board::Aim(int theSide, int theCursor)
{
	mGrid->SetCursor(theSide, theCursor);
	if (mTileMaster != NULL)
		mTileMaster->SetTarget(theSide, theCursor, false);
}

// 0x4083b0
void Board::RecordUndo(int theSide, int theCursor, int thePosition, int theType)
{
	UndoMove aMove;
	aMove.mSide = theSide;
	aMove.mCursor = theCursor;
	aMove.mPosition = thePosition;
	aMove.mType = theType;
	mUndo.push_back(aMove);
}

// 0x403fb0: take the last move back
void Board::Undo()
{
	if (mUndo.empty() || mGrid->mDone || mGrid->mHistory.empty())
		return;
	UndoMove aMove = mUndo.back();
	mUndo.pop_back();
	Aim(aMove.mSide, aMove.mCursor);

	switch (aMove.mType)
	{
	case 0:
	{
		GridObject* anObject = mGrid->mCells[mGrid->mStuckRow][mGrid->mStuckCol].mObject;
		if (mGrid->PullTile(false, true) && anObject != NULL)
		{
			anObject->mState = 1;
			anObject->mDir = ReverseSide(aMove.mSide);
			anObject->mTrailLen = 0;
		}
		break;
	}
	case 1:
	{
		int aRow = mGrid->mLandRow, aCol = mGrid->mLandCol;
		if (aMove.mSide == Grid::SIDE_UP || aMove.mSide == Grid::SIDE_DOWN)
			aRow = aMove.mPosition;
		else
			aCol = aMove.mPosition;
		int aStartCol = mGrid->mStartCol, aStartRow = mGrid->mStartRow;
		GridObject* anObject = mGrid->PutTile(aRow, aCol);
		if (anObject != NULL)
		{
			anObject->mVariant = CrtRand() % 5;
			LaunchTile(anObject, aStartCol, aStartRow, aMove.mSide, false);
		}
		break;
	}
	default:
		break;
	}

	mGrid->mHistory.pop_back();
	mGrid->mMoveCount--;
	mTileMaster->Pull();
	SaveProgress();
	mErrorTicks = 0;
}

// 0x42ec70: the tile leaves the edge cell and flies to its cell
void Board::LaunchTile(GridObject* theObject, int theStartCol, int theStartRow, int theDir, bool theBounce)
{
	theObject->mX = theStartCol * Grid::CELL_SIZE;
	theObject->mY = theStartRow * Grid::CELL_SIZE;
	theObject->mDir = theDir;
	theObject->mBounce = theBounce;
	theObject->mState = 2;
	theObject->mTrailLen = 0;
	theObject->mTrail = 0;
	theObject->mIdle = 0;
	theObject->mDead = false;
}

// 0x422040: has a position (in grid pixels) gone past the edge it moves towards?
bool Board::IsBeyondEdge(int theX, int theY, int theDir) const
{
	int aCol = FloorDiv(theX, Grid::CELL_SIZE);
	int aRow = FloorDiv(theY, Grid::CELL_SIZE);
	switch (theDir)
	{
	case 0: return aRow < 0;
	case 1: return aCol > mGrid->mCols - 1;
	case 2: return aRow > mGrid->mRows - 1;
	case 3: return aCol < 0;
	}
	return false;
}

// a click on a free cell throws a tile there
void Board::DoThrow()
{
	if (mGrid->mDone)
		return;
	int aSide = mGrid->mSide, aCursor = mGrid->mCursor;
	int aStartCol = mGrid->mStartCol, aStartRow = mGrid->mStartRow;
	bool aLeaves = !mGrid->mCanPlace && mGrid->mSlidesOut;
	bool aBounces = !aLeaves && mGrid->IsBouncyAhead();
	if (!mGrid->PlaceTile(true))
	{
		mApp->PlaySample(SOUND_ILLEGAL_NONE_LEFT_03B);
		return;
	}
	GridObject* anObject = mGrid->mObjects.back();
	anObject->mVariant = CrtRand() % 5;
	LaunchTile(anObject, aStartCol, aStartRow, aSide, aBounces);
	RecordUndo(aSide, aCursor, -1, (aLeaves || aBounces) ? 2 : 0);
	mTileMaster->Throw();
	mApp->PlaySample(SOUND_TILE_MOVING_02D);
	mIdleTicks = 0;
	SaveProgress();
}

// a click on a tile pulls it back
void Board::DoPull()
{
	if (mGrid->mDone)
		return;
	int aSide = mGrid->mSide, aCursor = mGrid->mCursor;
	int aPosition = (aSide == Grid::SIDE_UP || aSide == Grid::SIDE_DOWN) ? mGrid->mStuckRow : mGrid->mStuckCol;
	GridObject* anObject = mGrid->mCells[mGrid->mStuckRow][mGrid->mStuckCol].mObject;
	if (!mGrid->PullTile(true, false))
		return;
	if (anObject != NULL)
	{
		anObject->mState = 1;
		anObject->mDir = ReverseSide(aSide);
		anObject->mTrailLen = 0;
	}
	RecordUndo(aSide, aCursor, aPosition, 1);
	mTileMaster->Pull();
	mApp->PlaySample(SOUND_TILE_MOVING_02D);
	mIdleTicks = 0;
	SaveProgress();
}

// 0x423e50: a tile has come to rest
void Board::TileArrived(GridObject* theObject)
{
	const Grid::Cell& aCell = mGrid->mCells[theObject->mRow][theObject->mCol];
	bool aWanted = aCell.mWanted || aCell.mStart;
	mApp->PlaySample(aWanted ? SOUND_TILE_TARGET_02C : SOUND_TILE_NON_TARGET_02B);
	mFlash[theObject->mRow][theObject->mCol] = 40;
	mFlashWanted[theObject->mRow][theObject->mCol] = aWanted;
}

// 0x42f740 for every tile: the flight, the return and the fading of the trail
void Board::UpdateTiles()
{
	for (size_t i = 0; i < mGrid->mObjects.size(); i++)
	{
		GridObject* anObject = mGrid->mObjects[i];
		if (anObject->mDead)
			continue;

		if (anObject->mState == 0)
		{
			if (anObject->mTrail > 0)
			{
				anObject->mTrail -= 2;
				if (anObject->mTrail < 0)
					anObject->mTrail = 0;
			}
			if (anObject->mIdle < 1000)
				anObject->mIdle++;
		}
		else if (anObject->mTrail < 255)
		{
			anObject->mTrail += 10;
			if (anObject->mTrail > 200)
				anObject->mTrail = 200;
		}

		if (anObject->mState == 1)
		{
			// flying back to the thrower until it has left the grid
			mGrid->Step(&anObject->mX, &anObject->mY, anObject->mDir, 10);
			anObject->mTrailLen += 10;
			if (IsBeyondEdge(anObject->mX, anObject->mY, anObject->mDir))
			{
				anObject->mDead = true;
				anObject->mState = 0;
				anObject->mIdle = 0;
			}
		}
		else if (anObject->mState == 2)
		{
			int aTargetX = anObject->mCol * Grid::CELL_SIZE;
			int aTargetY = anObject->mRow * Grid::CELL_SIZE;
			if (anObject->mX != aTargetX || anObject->mY != aTargetY)
			{
				mGrid->Step(&anObject->mX, &anObject->mY, anObject->mDir, 10);
				anObject->mTrailLen += 10;
			}
			if (anObject->mX == aTargetX && anObject->mY == aTargetY)
			{
				if (!anObject->mBounce)
				{
					if (IsBeyondEdge(anObject->mX, anObject->mY, anObject->mDir))
					{
						// it left the grid: the tile goes back to the workbench (0x421ef0)
						anObject->mDead = true;
						mGrid->mSupply++;
					}
					else
					{
						anObject->mState = 0;
						anObject->mIdle = 0;
						TileArrived(anObject);
					}
				}
				else
				{
					// thrown back by a bouncy block (0x4251a0)
					anObject->mDir = ReverseSide(anObject->mDir);
					anObject->mState = 1;
					anObject->mTrailLen = 0;
					anObject->mBounce = false;
					mGrid->mSupply++;
					mGrid->ComputeLanding();
					mApp->PlaySample(SOUND_TILE_NON_TARGET_02B);
				}
			}
		}
	}
}

// 0x42ecf0: the colour of a tile: a block (0), a tile on a cell nobody wants (1), on a wanted cell (2), on a sticky spot (3)
int Board::TileKind(GridObject* theObject) const
{
	if (!mGrid->InGrid(theObject->mRow, theObject->mCol))
		return 1;
	const Grid::Cell& aCell = mGrid->mCells[theObject->mRow][theObject->mCol];
	if (aCell.mFixed)
		return 0;
	if (!aCell.mWanted)
		return 1;
	return aCell.mStart ? 3 : 2;
}

// 0x421bc0: the cell under a point of the screen
void Board::PixelToCell(int x, int y, int* theCol, int* theRow) const
{
	*theCol = FloorDiv(x - GRID_X, Grid::CELL_SIZE);
	*theRow = FloorDiv(y - GRID_Y, Grid::CELL_SIZE);
}

// 0x409bd0
void Board::Update()
{
	Widget::Update();
	MarkDirty();

	mIntro++;
	if (mAlpha < 255)
		mAlpha = std::min(255, mAlpha + 255 / 30 + 1);

	// the pause screen comes in and goes out
	if (mApp->IsPaused())
		mPauseFade = std::min(1.0, mPauseFade + 0.08);
	else
		mPauseFade = std::max(0.0, mPauseFade - 0.08);
	if (mApp->IsPaused())
		return;

	if (!mGrid->mDone)
		mGrid->mTime += 10;
	UpdateTiles();
	mTileMaster->Update();
	for (int r = 0; r < Grid::MAX_SIZE; r++)
		for (int c = 0; c < Grid::MAX_SIZE; c++)
			if (mFlash[r][c] > 0)
				mFlash[r][c]--;
	if (mErrorTicks > 0)
		mErrorTicks--;
	if (mHintShown && --mHintTicks <= 0)
		mHintShown = false;
	mIdleTicks++;

	// 150 updates after the puzzle appears (and no help screen over it) the music of the puzzles comes in
	if (mMusicDelay == -1 && mApp->mHelpScreen == NULL)
		mMusicDelay = 150;
	if (mMusicDelay > 0 && --mMusicDelay == 0 && !mGrid->mDone)
		StartMusic(mApp, MUSIC_CACHED_PIXELUS_IN_GAME, true);

	// the last tile went down: the picture shines for a while, then the puzzle-completed screen comes
	if (mGrid->mDone)
	{
		mDoneTimer++;
		if (mDoneTimer >= DONE_DELAY && !mCompleted)
		{
			mCompleted = true;
			Complete();
		}
	}
}

// the end of a puzzle: the player's record and the best known solution take the result (0x41b3c0 does the rest)
void Board::Complete()
{
	mApp->PuzzleCompleted(this);
}

// 0x402b00
void Board::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_SPACE)
		mApp->Pause(!mApp->IsPaused());
}

void Board::KeyUp(KeyCode theKey)
{
}

// 0x402b60
void Board::MouseLeave()
{
	mHoverCol = mHoverRow = -1;
	mHoverValid = false;
}

// 0x403370: the mouse aims
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (mApp->IsPaused() || mGrid->mDone)
		return;

	PixelToCell(x, y, &mHoverCol, &mHoverRow);
	mHoverValid = false;
	if (!mGrid->InGrid(mHoverRow, mHoverCol))
		return;

	int aCursor = 0;
	int aSide;
	if (!mGrid->HasObject(mHoverRow, mHoverCol))
		aSide = mGrid->FindThrowSide(mHoverRow, mHoverCol, mTileMaster->GetSide(), &aCursor);
	else
		aSide = mGrid->FindPullSide(mHoverRow, mHoverCol, mTileMaster->GetSide(), &aCursor);
	if (aSide >= 0)
	{
		Aim(aSide, aCursor);
		mHoverValid = true;
	}
}

// 0x408400 (point and click: control mode 2)
void Board::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (mApp->IsPaused())
	{
		mApp->Pause(false);
		return;
	}
	if (mGrid->mDone || theClickCount <= 0)
		return;

	int aCol, aRow;
	PixelToCell(x, y, &aCol, &aRow);
	if (!mGrid->InGrid(aRow, aCol))
		return;

	mHintShown = false;
	int aCursor = 0;
	if (!mGrid->HasObject(aRow, aCol))
	{
		int aSide = mGrid->FindThrowSide(aRow, aCol, mTileMaster->GetSide(), &aCursor);
		if (aSide >= 0)
		{
			Aim(aSide, aCursor);
			DoThrow();
			return;
		}
		mApp->PlaySample(SOUND_ILLEGAL_BLOCKED_03);
		return;
	}

	int aSide = mGrid->FindPullSide(aRow, aCol, mTileMaster->GetSide(), &aCursor);
	if (aSide >= 0)
	{
		Aim(aSide, aCursor);
		DoPull();
		return;
	}
	const GridObject* anObject = mGrid->mCells[aRow][aCol].mObject;
	mApp->PlaySample(anObject->mFixed ? SOUND_ILLEGAL_FIXED_01C : SOUND_ILLEGAL_NOTHING_01);
}

// 0x409160 (slot 5 of the listener): the buttons
void Board::ButtonDepress(int theId)
{
	if (mApp->IsPaused() && theId != ID_MENU)
		return;
	switch (theId)
	{
	case ID_UNDO:
		Undo();
		break;
	case ID_RESET:
		mApp->Pause(true);
		mApp->ShowResetDialog();
		break;
	case ID_MENU:
		mApp->ShowGameMenu();
		break;
	case ID_BACK:
		SaveProgress();
		mApp->LeaveBoard();
		break;
	case ID_HINT:
		mApp->Pause(true);
		mApp->ShowHint();
		break;
	}
}

// the next move of the best known solution when the moves so far are the start of it; an undo when they are not
bool Board::FindHint(Grid::Move* theMove, bool* theUndo)
{
	*theUndo = false;
	const ProgressManager::Best* aBest = mApp->mProgress->FindBest(mUuid);
	if (aBest == NULL)
		return false;
	const std::string& aSolution = aBest->mSolution;
	size_t aDone = mGrid->mHistory.size();
	for (size_t i = 0; i < aDone; i++)
	{
		if (aSolution.size() < (i + 1) * 6 || aSolution.compare(i * 6, 6, Grid::MoveToString(mGrid->mHistory[i])) != 0)
		{
			*theUndo = true;
			return true;
		}
	}
	if (aSolution.size() < (aDone + 1) * 6)
		return false;
	return Grid::ParseMove(aSolution.substr(aDone * 6, 6), theMove);
}

// 0x433050: the Tile Master shows where the next tile of the best solution goes
void Board::ShowHint()
{
	Grid::Move aMove;
	bool aUndo = false;
	if (!FindHint(&aMove, &aUndo) || aUndo)
		return;
	Aim(aMove.mSide, aMove.mCursor);
	mHintMove = aMove;
	mHintShown = true;
	mHintTicks = 200;
}

//////////////////////////////////////////////////////////////////////////
// Drawing

// 0x421450: the tiles still to throw lie on the bench below the grid
void Board::DrawWorkbench(Graphics* g)
{
	int aSupply = mGrid->mSupply;
	int aShown = std::min(aSupply, 16);
	g->SetColorizeImages(false);
	for (int i = 0; i < aShown; i++)
	{
		Image* aTile = mApp->mImages[IMAGE_TILEBLUE01 + ((i + 1) % 5)];
		g->DrawImage(aTile, GRID_X + i * Grid::CELL_SIZE - 1, 0x1c8 - 1);
	}
	if (aSupply > 16)
	{
		g->DrawImage(mApp->mImages[IMAGE_BAG], GRID_X + 16 * Grid::CELL_SIZE + 4, 0x1c8 - 8);
		char aText[16];
		sprintf(aText, "%d", aSupply - 16);
		g->SetFont(mApp->mFonts[FONT_GARAMOND12BOLD]);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aText, GRID_X + 16 * Grid::CELL_SIZE + 4 + 28, 0x1c8 + 8);
	}
}

// the picture of one tile (0x42f2e0)
void Board::DrawTile(Graphics* g, GridObject* theObject)
{
	int aKind = TileKind(theObject);
	int aX = GRID_X + theObject->mX - 1;
	int aY = GRID_Y + theObject->mY - 1;
	int aAlpha = mAlpha;
	Image* aImage = mApp->mImages[IMAGE_TILEDARKBLUE01 + aKind * 5 + theObject->mVariant];

	g->SetColorizeImages(aAlpha < 255);
	g->SetColor(Color(255, 255, 255, aAlpha));
	g->DrawImage(aImage, aX, aY);
	if (aKind == 3 && theObject->mState == 0)
		g->DrawImage(mApp->mImages[IMAGE_STICKYEDGE_20X20], aX + 1, aY + 1);

	// the shine of a tile that has just come to rest
	if (theObject->mIdle < 100)
	{
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aShine = (int) (sin(theObject->mIdle * PI / 100.0) * 120.0 * aAlpha / 255.0);
		g->SetColor(Color(255, 255, 255, aShine));
		g->DrawImage(mApp->mImages[IMAGE_GRAYTILE_20X20], aX, aY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	// the finished picture: the glow goes across the tiles from a corner (0x4224a0 and the double at +0x10)
	if (mGrid->mDone)
	{
		double aWave = -25.0 + mDoneTimer * 1.0;
		double aDistance = (theObject->mRow + theObject->mCol) - aWave;
		double aStrength = exp(-0.015 * aDistance * aDistance);
		if (aStrength > 0.0)
		{
			g->SetColorizeImages(true);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(255, 255, 0x80, (int) (aStrength * aAlpha)));
			g->DrawImage(mApp->mImages[IMAGE_GRAYTILE_20X20], aX, aY);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	}
	g->SetColorizeImages(false);
}

// 0x42ef70: the streak behind a tile that is moving (and fading after it)
void Board::DrawTrail(Graphics* g, GridObject* theObject)
{
	if (theObject->mTrail <= 0)
		return;
	static const int aTrailImages[4] = { IMAGE_TILETRAIL_UP, IMAGE_TILETRAIL_RIGHT, IMAGE_TILETRAIL_DOWN, IMAGE_TILETRAIL_LEFT };
	Image* anImage = mApp->mImages[aTrailImages[theObject->mDir & 3]];
	int aLength = std::min(anImage->GetWidth() > anImage->GetHeight() ? anImage->GetWidth() : anImage->GetHeight(), theObject->mTrailLen);
	if (aLength <= 0)
		return;

	int aX = GRID_X + theObject->mX;
	int aY = GRID_Y + theObject->mY;
	Rect aSource(0, 0, anImage->GetWidth(), anImage->GetHeight());
	switch (theObject->mDir)
	{
	case 0:		// moving up: the trail is below the tile
		aSource.mHeight = aLength;
		aY += 0;
		break;
	case 1:		// moving right: the trail is to the left
		aX -= aLength - Grid::CELL_SIZE;
		aSource.mX = anImage->GetWidth() - aLength;
		aSource.mWidth = aLength;
		aX += 0;
		break;
	case 2:		// moving down: the trail is above
		aY -= aLength - Grid::CELL_SIZE;
		aSource.mY = anImage->GetHeight() - aLength;
		aSource.mHeight = aLength;
		break;
	case 3:		// moving left: the trail is to the right
		aSource.mWidth = aLength;
		break;
	}

	Graphics aClipped(*g);
	aClipped.ClipRect(GRID_X, GRID_Y, mGrid->mCols * Grid::CELL_SIZE, mGrid->mRows * Grid::CELL_SIZE);
	aClipped.SetColorizeImages(true);
	aClipped.SetColor(Color(255, 255, 255, std::min(255, theObject->mTrail) * mAlpha / 255));
	aClipped.DrawImage(anImage, aX, aY, aSource);
}

// the marks that show where the next tile will go (0x422ab0, the second half)
void Board::DrawAim(Graphics* g)
{
	if (mGrid->mDone || mApp->IsPaused())
		return;
	bool aThrow = mGrid->mCanPlace || mGrid->mSlidesOut;
	bool aPull = mGrid->mCanPull;
	if (!aThrow && !aPull)
		return;

	int aSide = mGrid->mSide;
	// the images face right; the side is where the tile comes from
	double aAngle = 0.0;
	switch (aSide)
	{
	case 0: aAngle = 90.0; break;
	case 1: aAngle = 0.0; break;
	case 2: aAngle = 270.0; break;
	case 3: aAngle = 180.0; break;
	}
	aAngle *= PI / 180.0;

	bool aOnTile = mHoverValid && mGrid->InGrid(mHoverRow, mHoverCol) && mGrid->HasObject(mHoverRow, mHoverCol);
	int aTargetCol = mGrid->mLandCol, aTargetRow = mGrid->mLandRow;
	if (aOnTile)
	{
		aTargetCol = mGrid->mStuckCol;
		aTargetRow = mGrid->mStuckRow;
	}

	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, mAlpha));
	const int S = Grid::CELL_SIZE;

	if (aThrow && !aOnTile)
	{
		g->DrawImageRotated(mApp->mImages[IMAGE_AIM_TARGET_SHADOW], aTargetCol * S - 5 + GRID_X, aTargetRow * S - 5 + GRID_Y, aAngle);
		g->DrawImageRotated(mApp->mImages[IMAGE_AIM_TARGET], aTargetCol * S - 8 + GRID_X, aTargetRow * S - 8 + GRID_Y, aAngle);
	}

	// the dots along the line from the edge up to the target
	int aCol = mGrid->mStartCol, aRow = mGrid->mStartRow;
	int aDirection = aThrow ? aSide : aSide;
	int aGuard = 0;
	while ((aCol != aTargetCol || aRow != aTargetRow) && aGuard++ < 40)
	{
		g->DrawImageRotated(mApp->mImages[IMAGE_AIM_DOT_SHADOW], aCol * S - 2 + GRID_X, aRow * S - 2 + GRID_Y, aAngle);
		g->DrawImageRotated(mApp->mImages[IMAGE_AIM_DOT], aCol * S - 5 + GRID_X, aRow * S - 5 + GRID_Y, aAngle);
		mGrid->Step(&aCol, &aRow, aDirection, 1);
	}

	// the thrower's mark just outside the edge
	int aBaseCol = mGrid->mStartCol, aBaseRow = mGrid->mStartRow;
	mGrid->Step(&aBaseCol, &aBaseRow, ReverseSide(aSide), 1);
	g->DrawImageRotated(mApp->mImages[IMAGE_AIM_BASE_SHADOW], aBaseCol * S - 8 + GRID_X, aBaseRow * S - 8 + GRID_Y, aAngle);
	g->DrawImageRotated(mApp->mImages[IMAGE_AIM_BASE], aBaseCol * S - 11 + GRID_X, aBaseRow * S - 11 + GRID_Y, aAngle);

	// arrows from the hovered cell towards the thrower
	if (mHoverValid && (!aOnTile || aPull) && (mGrid->mSupply > 0 || aOnTile))
	{
		int aArrowCol = mHoverCol, aArrowRow = mHoverRow;
		int aStep = aOnTile ? 1 : 2;
		double aArrowAngle = aAngle + (aOnTile ? PI : 0.0);
		int aOffset = (mUpdateCnt / 2) % 10;
		int aGuard2 = 0;
		while (mGrid->InGrid(aArrowRow, aArrowCol) && aGuard2++ < 40)
		{
			mGrid->Step(&aArrowCol, &aArrowRow, ReverseSide(aSide), aStep);
			aStep = 2;
			int aX = aArrowCol * S + GRID_X + S / 2 - 20;
			int aY = aArrowRow * S + GRID_Y + S / 2 - 10;
			switch (aSide)
			{
			case 0: aY += aOffset; break;
			case 1: aX -= aOffset; break;
			case 2: aY -= aOffset; break;
			case 3: aX += aOffset; break;
			}
			if (!mGrid->InGrid(aArrowRow, aArrowCol))
				break;
			g->DrawImageRotated(mApp->mImages[IMAGE_AIM_ARROW], aX, aY, aArrowAngle);
		}
	}

	if (mHoverValid || mHintShown)
	{
		int aCursorCol = mHintShown ? aTargetCol : mHoverCol;
		int aCursorRow = mHintShown ? aTargetRow : mHoverRow;
		if (mGrid->InGrid(aCursorRow, aCursorCol))
			g->DrawImage(mApp->mImages[IMAGE_AIM_CURSOR], aCursorCol * S + GRID_X + S / 2 - 15, aCursorRow * S + GRID_Y + S / 2 - 15);
	}
	g->SetColorizeImages(false);
}

// 0x422ab0
void Board::DrawGrid(Graphics* g)
{
	const int S = Grid::CELL_SIZE;
	g->SetColorizeImages(mAlpha < 255);
	g->SetColor(Color(255, 255, 255, mAlpha));
	Image* aWanted = mApp->mImages[IMAGE_TILEBACK_WANTTILE_20X20];

	for (int r = 0; r < mGrid->mRows; r++)
		for (int c = 0; c < mGrid->mCols; c++)
		{
			const Grid::Cell& aCell = mGrid->mCells[r][c];
			int aX = GRID_X + c * S, aY = GRID_Y + r * S;
			if (aCell.mWanted)
				g->DrawImage(aWanted, aX - 1, aY - 1);
			if (aCell.mStart)
				g->DrawImage(mApp->mImages[IMAGE_STICKYSPOT_20X20], aX, aY);
		}

	// the trails lie under every tile
	for (size_t i = 0; i < mGrid->mObjects.size(); i++)
		if (!mGrid->mObjects[i]->mDead)
			DrawTrail(g, mGrid->mObjects[i]);
	// resting tiles first, the ones in flight over them
	for (int aPass = 0; aPass < 2; aPass++)
		for (size_t i = 0; i < mGrid->mObjects.size(); i++)
		{
			GridObject* anObject = mGrid->mObjects[i];
			if (anObject->mDead || (anObject->mState == 0) != (aPass == 0))
				continue;
			DrawTile(g, anObject);
		}

	// the glow where a tile has just landed
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int r = 0; r < mGrid->mRows; r++)
		for (int c = 0; c < mGrid->mCols; c++)
			if (mFlash[r][c] > 0)
			{
				int aAlpha = (int) (sin(mFlash[r][c] * PI / 40.0) * 255.0);
				g->SetColor(Color(255, 255, 255, aAlpha));
				Image* aGlow = mApp->mImages[mFlashWanted[r][c] ? IMAGE_TILE_GLOW : IMAGE_TILE_REDGLOW];
				g->DrawImage(aGlow, GRID_X + c * S + S / 2 - aGlow->GetWidth() / 2, GRID_Y + r * S + S / 2 - aGlow->GetHeight() / 2);
			}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);

	DrawAim(g);
}

// 0x404be0, the left side: the tutorial's text, or the move count and the records
void Board::DrawSidebar(Graphics* g)
{
	if (mTutorial)
	{
		g->DrawImage(mApp->mImages[mPuzzleIndex == 8 ? IMAGE_SIDEBAR_TUTALT : IMAGE_SIDEBAR_TUT], 0, 0);
		g->SetColor(Color(255, 255, 255));
		g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
		static const unsigned int aTutorialTexts[9] = { 0x50d760, 0x50d724, 0x50d6f0, 0x50d688, 0x50d628, 0x50d5e8, 0x50d5a8, 0x50d578, 0x50d530 };
		int aTop = mPuzzleIndex == 8 ? 0xd4 : 0xde;
		Rect aRect(0x11, aTop, 0x82, 0xbf);
		const SexyString& aText = mGrid->mDone ? GT(0x50c454) : GT(aTutorialTexts[std::max(0, std::min(8, mPuzzleIndex))]);
		WriteWordWrapped2004(g, aRect, aText, g->GetFont()->GetHeight() + 2, 0);
	}

	if (mExpert && (!mTutorial || mPuzzleIndex == 8))
		g->DrawImage(mApp->mImages[IMAGE_EXPERTMODE], 63, 372);

	if (mTutorial)
		return;

	char aText[32];
	g->SetColor(Color(255, 255, 255));
	sprintf(aText, "%d", mGrid->mMoveCount);
	g->SetFont(mApp->mFonts[FONT_VERDANA18BOLD]);
	g->DrawString(aText, 0x52 - g->StringWidth(aText) / 2, 0xee);

	UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
	const PuzzleRecord* aRecord = aUser != NULL ? aUser->GetRecord(mUuid) : NULL;
	g->SetFont(mApp->mFonts[FONT_ARIAL10]);
	int aY = 0x12c;
	if (aRecord != NULL)
	{
		g->DrawImage(mApp->mImages[IMAGE_PERSONALBEST], 0x1e, aY);
		sprintf(aText, "%d", aRecord->mMoves);
		g->DrawString(aText, 0x82 - g->StringWidth(aText), aY + 9);
		aY += 14;
	}
	g->DrawImage(mApp->mImages[IMAGE_WORLDRECORD], 0x1e, aY);
	sprintf(aText, "%d", mApp->mProgress->GetBestMoves(mUuid));
	g->DrawString(aText, 0x82 - g->StringWidth(aText), aY + 9);
}

// 0x404be0
void Board::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_PLAYFIELD], 0, 0);

	DrawSidebar(g);
	DrawGrid(g);
	DrawWorkbench(g);
	mTileMaster->Draw(g, mAlpha);

	if (mPauseFade > 0.0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, (int) (mPauseFade * 255)));
		g->DrawImage(mApp->mImages[IMAGE_PAUSECANVAS], GRID_X + mGrid->mCols * Grid::CELL_SIZE / 2 - 180, GRID_Y + mGrid->mRows * Grid::CELL_SIZE / 2 - 180);
		g->SetColor(Color(0, 0, 0, (int) (mPauseFade * 160)));
		g->FillRect(SIDEBAR_WIDTH, 0, mWidth - SIDEBAR_WIDTH, mHeight);
		g->SetColorizeImages(false);

		g->SetFont(mApp->mFonts[FONT_PRAITOR36]);
		g->SetColor(Color(255, 255, 255, (int) (mPauseFade * 255)));
		g->DrawString(GT(0x50c410), 0x195 - g->StringWidth(GT(0x50c410)) / 2, 0xbe);
		if (mApp->mPauseCount == 1)
		{
			g->SetFont(mApp->mFonts[FONT_PRAITOR24]);
			g->SetColor(Color(0, 0, 0, (int) (mPauseFade * 255)));
			g->DrawString(GT(0x50c3f4), 0x197 - g->StringWidth(GT(0x50c3f4)) / 2, 0xf0);
			g->SetColor(Color(0xfd, 0xd1, 0x5b, (int) (mPauseFade * 255)));
			g->DrawString(GT(0x50c3f4), 0x195 - g->StringWidth(GT(0x50c3f4)) / 2, 0xee);
		}
	}
}
