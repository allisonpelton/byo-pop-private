// Board.cpp, part 3: the mouse (WinNoah.exe 0x40e340 MouseDown, 0x40f310 MouseUp, 0x40f3c0 MouseMove,
// 0x40f710 MouseDrag) and the trail preview (0x402ec0).

#include "Board.h"
#include "NoahApp.h"
#include "Compat2004.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "ModFeatures.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// 0x402ec0: the path from one cell to another as trail nodes, one per cell, left in mTrail. (The original also works out each
// node's kind of bend, 0..3, or the last one's arrow direction, and stores it in the node, where nothing reads it: the drawing
// takes both from the neighbouring nodes. The port leaves that out.)
bool Board::BuildTrail(int theFromX, int theFromY, int theToX, int theToY)
{
	ClearLink();
	for (size_t i = 0; i < mTrail.size(); i++)
		delete mTrail[i];
	mTrail.clear();

	if (!FindPath(theFromX, theFromY, theToX, theToY, 0x32, true))
		return false;
	if (mPathSegments < 0)
		return false;
	mPathCells[mPathSegments][0] = theToX;
	mPathCells[mPathSegments][1] = theToY;

	int aCurX = theFromX;
	int aCurY = theFromY;
	int aCornerIndex = 0;
	int aCornerX = theFromX;
	int aCornerY = theFromY;
	int aDX = 0;
	int aDY = 0;
	bool aDone = false;
	while (!aDone)
	{
		if ((aCurX == theToX) && (aCurY == theToY))
			aDone = true;
		else
		{
			if ((aCurX == aCornerX) && (aCurY == aCornerY))
			{
				aCornerX = mPathCells[aCornerIndex][0];
				aCornerY = mPathCells[aCornerIndex][1];
				aCornerIndex++;
			}
			aDX = 0;
			aDY = 0;
			if (aCurX < aCornerX)
				aDX = 1;
			else if (aCornerX < aCurX)
				aDX = -1;
			if (aCurY < aCornerY)
				aDY = 1;
			else if (aCornerY < aCurY)
				aDY = -1;
		}

		PathNode* aNode = new PathNode();
		memset(aNode, 0, sizeof(PathNode));
		aNode->mPixelX = CellLeft(aCurX);
		aNode->mPixelY = CellTop(aCurY);
		aNode->mCellX = aCurX;
		aNode->mCellY = aCurY;
		mTrail.push_back(aNode);

		aCurX += aDX;
		aCurY += aDY;
	}
	return true;
}

// 0x40f3c0
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (mPaused)
		return;
	if (mState != BOARD_PLAYING)
		return;
	if ((mApp->mNextLevelWidget != NULL) || (mApp->mHighScoreWidget != NULL))
		return;

	int aCellX = CellAtX(x);
	int aCellY = CellAtY(y);
	if ((aCellX < 0) || (aCellX > 8) || (aCellY < 0) || (aCellY > 8))
	{
		if (mLink.size() < 2)
			ClearLink();
		mMouseX = x;
		mMouseY = y;
		return;
	}

	bool aSameCell = (CellAtX(mMouseX) == aCellX) && (CellAtY(mMouseY) == aCellY);
	if (!aSameCell && (mSelected != NULL))
	{
		int aMode = mApp->mGameMode;
		if ((aMode != GAMEMODE_PUZZLE) && (aMode != GAMEMODE_STAMPEDE))
		{
			Piece* aPiece = mGrid[aCellX][aCellY];
			if ((mSelected != aPiece) && (mSelected->mType == aPiece->mType) && (mLink.size() < 2) && !aPiece->mRemoved && !aPiece->mMoving)
			{
				bool aInList = std::find(mLink.begin(), mLink.end(), aPiece) != mLink.end();
				if (Reachable(mSelected, aPiece) && !aInList)
				{
					ClearLink();
					if (BuildTrail(mSelected->mX, mSelected->mY, aCellX, aCellY))
					{
						aPiece->mSelected = true;
						mMouseY = y;
						mMouseX = x;
						return;
					}
				}
			}
		}
	}
	if (!aSameCell)
	{
		if ((mApp->mGameMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
		{
			HighlightGroup(aCellX, aCellY);
			mMouseY = y;
			mMouseX = x;
			return;
		}
		ClearLink();
	}
	mMouseY = y;
	mMouseX = x;
}

// Starts the walk of a joined pair: the first piece gets the path, the second the same path reversed
void Board::LinkPieces(Piece* theFirst, Piece* theSecond)
{
	ClearLinkList();
	mLink.push_back(theFirst);
	mLink.push_back(theSecond);
	for (size_t i = 0; i < theFirst->mPath.size(); i++)
		delete theFirst->mPath[i];
	theFirst->mPath.clear();
	for (size_t i = 0; i < theSecond->mPath.size(); i++)
		delete theSecond->mPath[i];
	theSecond->mPath.clear();
	for (size_t i = 0; i < mTrail.size(); i++)
	{
		PathNode* aNode = new PathNode(*mTrail[i]);
		theFirst->mPath.push_back(aNode);
	}
	for (int i = (int) theFirst->mPath.size() - 1; i >= 0; i--)
	{
		PathNode* aNode = new PathNode(*theFirst->mPath[i]);
		theSecond->mPath.push_back(aNode);
	}
	theFirst->mPartner = theSecond;
	theSecond->mPartner = theFirst;
	theFirst->mFacingRight = true;
	theSecond->mFacingRight = false;
}

// 0x40e340. port(mod): the undo mod takes the board as it was before a click that starts a move (Mods.cpp)
void Board::MouseDown(int x, int y, int theClickCount)
{
	std::string aBefore;
	int aStateBefore = mState;
	bool aWatch = ModUndoBegin(aBefore);
	MouseDownImpl(x, y, theClickCount);
	if (aWatch)
		ModUndoEnd(aBefore, aStateBefore);
}

void Board::MouseDownImpl(int x, int y, int theClickCount)
{
	if ((mApp->mNextLevelWidget != NULL) || (mApp->mHighScoreWidget != NULL))
		return;

	// a right click puts the picked animal back
	if ((theClickCount < 0) && (mApp->mGameMode != GAMEMODE_STAMPEDE))
	{
		if (mSelected != NULL)
		{
			mSelected = NULL;
			mApp->PlaySample(SOUND_DESELECT);
		}
		ClearLinkList();
		ClearLink();
		return;
	}

	// before the level starts, the first click lets the animals settle
	if ((mState == BOARD_GET_READY) && !AnyWaiting())
	{
		bool aAnyPending = false;
		for (int i = 0; i < 9; i++)
			for (int j = 0; j < 9; j++)
				if (mGrid[i][j]->mPending)
					aAnyPending = true;
		if (!aAnyPending)
		{
			for (int i = 0; i < 9; i++)
			{
				for (int j = 0; j < 9; j++)
				{
					mGrid[i][j]->mSelected = false;
					mGrid[i][j]->mFlash = false;
					mGrid[i][j]->mFallSpeed = 1;
				}
			}
			mState = BOARD_PLAYING;
			mApp->ResyncEvent('r');
			// fix snapWater: the get-ready's own end brings the water to its level (BoardUpdate.cpp), a click that ends it
			// did not, which left the water a pixel off for the rest of the level
			if (ModFixOn("snapWater") && (mApp->mGameMode != GAMEMODE_STRATEGY))
				mWaterY = mWaterTargetY;
		}
		ClearLinkList();
		ClearLink();
	}
	if (mState != BOARD_PLAYING)
		return;

	mLastInputTick = mUpdateCnt;
	MouseMove(x, y);
	int aCellX = CellAtX(x);
	int aCellY = CellAtY(y);
	if ((aCellX < 0) || (aCellX > 8) || (aCellY < 0) || (aCellY > 8))
	{
		if (mSelected != NULL)
		{
			mSelected->mSelected = false;
			mSelected = NULL;
			mApp->PlaySample(SOUND_DESELECT);
		}
		return;
	}

	int aMode = mApp->mGameMode;
	if (aMode != GAMEMODE_PUZZLE)
	{
		if ((aMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
		{
			if (mLink.empty())
				return;
			RemoveGroup();   // 0x40e340: the group the move highlighted, the clicked animal in it once
			return;
		}

		Piece* aClicked = mGrid[aCellX][aCellY];
		if (!aClicked->mFlash)
			ClearHint();
		ConnectPair();
		if (mSelected == NULL)
		{
			if ((aClicked->mType < 0xd) && !aClicked->mRemoved && !aClicked->mFading && !aClicked->mMoving)
			{
				mSelected = aClicked;
				mApp->PlaySample(SOUND_SELECT);
				return;
			}
			mSelected = NULL;
			return;
		}

		if ((mSelected != aClicked) && (mSelected->mType == aClicked->mType) && !aClicked->mRemoved &&
			!aClicked->mFading && !aClicked->mMoving)
		{
			if (mState == BOARD_PLAYING)
			{
				if (Reachable(mSelected, aClicked))
				{
					if (mLink.size() < 2)
					{
						// join them: the path was built by the mouse move
						LinkPieces(mSelected, aClicked);
						mApp->PlaySample(SOUND_SELECT);
						ClearLink();
						ConnectPair();
						mLastMoveTick = mUpdateCnt;
						ClearHint();
						return;
					}
				}
				if (CountEmpty() < 8)
					ShowHintLine(1);
			}
			mSelected = aClicked;
			mApp->PlaySample(SOUND_SELECT);
			return;
		}
		if ((aClicked->mType < 0xd) && !aClicked->mMoving && !aClicked->mRemoved)
		{
			mSelected = aClicked;
			mApp->PlaySample(SOUND_SELECT);
			return;
		}
		mApp->PlaySample(SOUND_DESELECT);
		mSelected = NULL;
		return;
	}

	// puzzle mode: the first click picks an animal, a click on a neighbour swaps the two
	if (mSelected == NULL)
	{
		if (mState != BOARD_PLAYING)
			return;
		Piece* aClicked = mGrid[aCellX][aCellY];
		if ((aClicked->mType < 0xd) && !aClicked->mRemoved && !aClicked->mFading && !aClicked->mMoving)
		{
			mSelected = aClicked;
			mApp->PlaySample(SOUND_SELECT);
			return;
		}
		mSelected = NULL;
		return;
	}
	if (mState != BOARD_PLAYING)
		return;
	PuzzleSwap(aCellX, aCellY);
}

// puzzle mode: the picked animal and the one at the cell trade places when the cell is its neighbour (the click's second step
// and the drag's); a cell farther away only drops the pick. Returns whether they traded.
bool Board::PuzzleSwap(int theCellX, int theCellY)
{
	int aDX = mSelected->mX - theCellX;
	int aDY = mSelected->mY - theCellY;
	if (sqrtf((float) (aDY * aDY + aDX * aDX)) != 1.0f)
	{
		mSelected = NULL;
		return false;
	}
	Piece* aOther = mGrid[theCellX][theCellY];
	int aType = mSelected->mType;
	mSelected->mType = aOther->mType;
	aOther->mType = aType;
	if (aDX < 0)
	{
		mSelected->mOffsetX += 0x32;
		aOther->mOffsetX -= 0x32;
	}
	else if (aDX > 0)
	{
		mSelected->mOffsetX -= 0x32;
		aOther->mOffsetX += 0x32;
	}
	if (aDY < 0)
	{
		mSelected->mOffsetY += 0x32;
		aOther->mOffsetY -= 0x32;
	}
	else if (aDY > 0)
	{
		mSelected->mOffsetY -= 0x32;
		aOther->mOffsetY += 0x32;
	}
	mSelected->mInSwap = true;
	aOther->mInSwap = true;
	mSelected = NULL;
	mState = BOARD_SWAP;
	return true;
}

// 0x40f310
void Board::MouseUp(int x, int y)
{
	if ((mApp->mNextLevelWidget == NULL) && (mApp->mHighScoreWidget == NULL))
	{
		if ((mLink.size() > 1) && mDragging)
			ConnectPair();
		if ((mApp->mGameMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
			HighlightGroup(CellAtX(x), CellAtY(y));
		mDragging = false;
	}
}

void Board::MouseUp(int x, int y, int theClickCount)
{
	(void) theClickCount;
	MouseUp(x, y);
}

// 0x40f710: dragging: in action and strategy a link from the picked animal to another of its kind, in puzzle the picked animal
// onto a neighbour (the two trade places). The two statements that note the mouse position are the original's 0x40f3a0.
void Board::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	mMouseX = x;
	mMouseY = y;
	if ((mState != BOARD_PLAYING) && (mApp->mGameMode == GAMEMODE_PUZZLE))
		return;
	if (mApp->mGameMode == GAMEMODE_STAMPEDE)
		return;
	mLastInputTick = mUpdateCnt;
	MouseMove(x, y);
	int aCellX = CellAtX(x);
	int aCellY = CellAtY(y);
	if ((aCellX < 0) || (aCellX > 8) || (aCellY < 0) || (aCellY > 8))
		return;

	Piece* aPiece = mGrid[aCellX][aCellY];
	if (!aPiece->mFlash)
		ClearHint();
	if (mSelected == aPiece)
		return;
	if (aPiece->mType > 0xc)
		return;

	if (mApp->mGameMode != GAMEMODE_PUZZLE)
	{
		if (mSelected == NULL)
			return;
		if (mSelected->mType != aPiece->mType)
			return;
		if (aPiece->mRemoved || aPiece->mFading || aPiece->mMoving)
			return;
		if (mState != BOARD_PLAYING)
			return;

		bool aInList = std::find(mLink.begin(), mLink.end(), aPiece) != mLink.end();
		// no link yet: an animal it cannot reach is treated like one already in the link, which changes nothing
		if (!aInList && mLink.empty() && !Reachable(mSelected, aPiece))
			aInList = true;
		if (aInList)
		{
			// moving back over the first animal of the pair undoes the link
			if (mLink.size() > 1)
			{
				Piece* aFirst = mLink.front();
				Piece* aSecond = mLink[1];
				if (aFirst == aPiece)
				{
					aFirst->mPartner = NULL;
					aSecond->mPartner = NULL;
					aFirst->mFacingRight = false;
					aSecond->mFacingRight = false;
					mDragging = false;
					mSelected = aFirst;
					ClearLink();
					ClearLinkList();
					mApp->PlaySample(SOUND_DESELECT);
				}
			}
			return;
		}

		if (!mLink.empty())
		{
			// a link is there: it is taken back when the new animal is reachable from its first animal
			Piece* aFirst = mLink.front();
			Piece* aLast = mLink.back();
			if (!Reachable(aFirst, aPiece))
				return;
			aFirst->mPartner = NULL;
			aFirst->mFacingRight = false;
			aLast->mPartner = NULL;
			aLast->mFacingRight = false;
			mSelected = aFirst;
			ClearLink();
			ClearLinkList();
		}
		// the path's result is not looked at: the pair is joined with whatever path was made
		BuildTrail(mSelected->mX, mSelected->mY, aCellX, aCellY);
		LinkPieces(mSelected, aPiece);
		mSelected = aPiece;
		mApp->PlaySample(SOUND_SELECT);
		mLastMoveTick = mUpdateCnt;
		ClearHint();
		mDragging = true;
		return;
	}

	// puzzle mode
	if (mSelected == NULL)
	{
		if (mDragging)
			return;
		if (mState != BOARD_PLAYING)
			return;
		if ((aPiece->mType < 0xd) && !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving)
		{
			mSelected = aPiece;
			mApp->PlaySample(SOUND_SELECT);
			return;
		}
		mSelected = NULL;
		return;
	}
	if (mState != BOARD_PLAYING)
		return;
	if (PuzzleSwap(aCellX, aCellY))
		mDragging = true;
}
