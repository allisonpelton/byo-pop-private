// Board.cpp, part 3: the mouse (WinNoah.exe 0x40e340 MouseDown, 0x40f310 MouseUp, 0x40f3c0 MouseMove,
// 0x40f710 MouseDrag) and the trail preview (0x402ec0).

#include "Board.h"
#include "NoahApp.h"
#include "Compat2004.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// 0x402ec0: the path from (x1,y1) to (x2,y2) as trail nodes, one per cell, each with the corner shape it
// needs (kind 0..3) or, for the last, the direction of the arrow. Leaves them in mTrail.
bool Board::BuildTrail(int x1, int y1, int x2, int y2)
{
	ClearLink();
	for (size_t i = 0; i < mTrail.size(); i++)
		delete mTrail[i];
	mTrail.clear();

	if (!FindPath(x1, y1, x2, y2, 0x32, true))
		return false;
	if (mPathSegments < 0)
		return false;
	mPathCells[mPathSegments][0] = x2;
	mPathCells[mPathSegments][1] = y2;

	int aCurX = x1;
	int aCurY = y1;
	int aCornerIndex = 0;
	int aCornerX = x1;
	int aCornerY = y1;
	int aPrevDX = 0;
	int aPrevDY = 0;
	int aDX = 0;
	int aDY = 0;
	bool aDone = false;
	while (!aDone)
	{
		int aOldDX = aDX;
		if ((aCurX == x2) && (aCurY == y2))
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

		// the corner this cell turns: 0 straight or none, 1 .. 3 the three kinds of bend
		int aKind = 0;
		if ((aDX == 0) || (aOldDX == 0))
		{
			if ((aDY == 0) || (aPrevDY == 0))
			{
				if (((aDX < 1) || (aPrevDY >= 0)) && ((aOldDX >= 0) || (aDY < 1)))
				{
					if (((aDX < 1) || (aPrevDY < 1)) && ((aOldDX >= 0) || (aDY >= 0)))
					{
						if (((aDX < 0) && (aPrevDY < 0)) || ((aOldDX > 0) && (aDY > 0)))
							aKind = 3;
						else if (((aDX < 0) && (aPrevDY > 0)) || ((aOldDX > 0) && (aDY < 0)))
							aKind = 2;
					}
					else
						aKind = 1;
				}
				else
					aKind = 0;
			}
			else
				aKind = 1;
		}
		int aArrow = 0;
		if (aDX < 0)
			aArrow = 2;
		else if (aDX < 1)
		{
			if (aDY < 0)
				aArrow = 1;
			else if (aDY > 0)
				aArrow = 3;
		}
		aNode->mUnused14[0] = 0;
		aNode->mUnused14[1] = aDone ? aArrow : aKind;
		mTrail.push_back(aNode);

		aPrevDX = aDX;
		aPrevDY = aDY;
		aCurX += aDX;
		aCurY += aDY;
		(void) aPrevDX;
		// the original keeps the previous step in two more locals; the corner tests above read them
		aOldDX = aDX;
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
			Piece* p = mGrid[aCellX][aCellY];
			if ((mSelected != p) && (mSelected->mType == p->mType) && (mLink.size() < 2) && !p->mRemoved && !p->mMoving)
			{
				bool aInList = std::find(mLink.begin(), mLink.end(), p) != mLink.end();
				if (Reachable(mSelected, p) && !aInList)
				{
					ClearLink();
					if (BuildTrail(mSelected->mX, mSelected->mY, aCellX, aCellY))
					{
						p->mSelected = true;
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
			mLink.push_back(mGrid[aCellX][aCellY]);
			RemoveGroup();
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
	int aDX = mSelected->mX - aCellX;
	int aDY = mSelected->mY - aCellY;
	if (sqrtf((float) (aDY * aDY + aDX * aDX)) != 1.0f)
	{
		mSelected = NULL;
		return;
	}
	Piece* aOther = mGrid[aCellX][aCellY];
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
	mSelected->mMatchSwap = true;
	aOther->mMatchSwap = true;
	mSelected = NULL;
	mState = BOARD_SWAP;
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

// 0x40f710: dragging a link from one animal to another
void Board::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	if (!AnyWaiting() && false)
		return;
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

	Piece* p = mGrid[aCellX][aCellY];
	if (!p->mFlash)
		ClearHint();
	if (mSelected == p)
		return;
	if (p->mType > 0xc)
		return;
	if (mApp->mGameMode == GAMEMODE_PUZZLE)
		return;
	if (mSelected == NULL)
		return;
	if (mSelected->mType != p->mType)
		return;
	if (p->mRemoved || p->mFading || p->mMoving)
		return;
	if (mState != BOARD_PLAYING)
		return;

	bool aInList = std::find(mLink.begin(), mLink.end(), p) != mLink.end();
	if (!aInList)
	{
		if (mLink.size() < 2)
		{
			if (!Reachable(mSelected, p))
				aInList = true;  // fall through to the "no longer reachable" branch below
		}
	}
	if (aInList)
	{
		// moving back over the first animal of the pair undoes the link
		if (mLink.size() > 1)
		{
			Piece* aFirst = mLink.front();
			Piece* aLast = mLink.back();
			if (aFirst == p)
			{
				aFirst->mPartner = NULL;
				aLast->mPartner = NULL;
				aFirst->mFacingRight = false;
				aLast->mFacingRight = false;
				mDragging = false;
				mSelected = aFirst;
				ClearLink();
				ClearLinkList();
				mApp->PlaySample(SOUND_DESELECT);
			}
		}
		return;
	}

	if (mLink.size() >= 1)
	{
		Piece* aFirst = mLink.front();
		Piece* aLast = mLink.back();
		if (!Reachable(aFirst, p))
			return;
		aFirst->mPartner = NULL;
		aFirst->mFacingRight = false;
		aLast->mPartner = NULL;
		aLast->mFacingRight = false;
		mSelected = aFirst;
		ClearLink();
		ClearLinkList();
	}
	if (BuildTrail(mSelected->mX, mSelected->mY, aCellX, aCellY))
	{
		LinkPieces(mSelected, p);
		mDragging = true;
		mSelected = mLink.front();
	}
}
