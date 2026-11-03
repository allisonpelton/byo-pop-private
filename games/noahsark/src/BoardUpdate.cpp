// Board.cpp, part 4: the per-tick update (WinNoah.exe 0x403d60, Board::vf18). One tick is 10 ms of the
// framework's update clock; mUpdateCnt (+0x40) counts them.

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

namespace
{

// move v towards 0 by theStep; a value within theSnap of 0 becomes 0
void Ease(int& v, int theStep, int theSnap)
{
	if (v > 0)
		v -= theStep;
	if (v < 0)
		v += theStep;
	if (abs(v) < theSnap)
		v = 0;
}

}

// Two cells swap their contents when the columns close in on the middle (0x403d60): the animal, its state flags, its
// fall, its offset from the cell and its pixel position (an animal on its way to the ark flies on from where it was).
// A takes B's contents, B takes A's. The offset in x the contents arrive with is B's own plus theOffsetXForA (A) and
// A's own plus theOffsetXForB (B), so the animal stays where it is drawn. The original's two loops differ in y: the
// left column takes B's offset (theSwapOffsetY), the right one keeps its own. The pixel position B receives is A's old
// one cut to whole pixels.
void Board::ExchangeContents(Piece* a, Piece* b, int theOffsetXForA, int theOffsetXForB, bool theSwapOffsetY)
{
	std::swap(a->mType, b->mType);
	std::swap(a->mRemoved, b->mRemoved);
	std::swap(a->mMoving, b->mMoving);
	std::swap(a->mFading, b->mFading);
	std::swap(a->mAlpha, b->mAlpha);
	std::swap(a->mFallSpeed, b->mFallSpeed);
	int aOffsetX = a->mOffsetX;
	int aOffsetY = a->mOffsetY;
	float aPixelX = (float) (int) a->mPixelX;
	float aPixelY = (float) (int) a->mPixelY;
	a->mOffsetX = b->mOffsetX + theOffsetXForA;
	if (theSwapOffsetY)
		a->mOffsetY = b->mOffsetY;
	a->mPixelX = b->mPixelX;
	a->mPixelY = b->mPixelY;
	b->mOffsetX = aOffsetX + theOffsetXForB;
	b->mOffsetY = aOffsetY;
	b->mPixelX = aPixelX;
	b->mPixelY = aPixelY;
}

// 0x403d60
void Board::Update()
{
	if (mPaused)
		return;
	if (!mApp->mHasFocus)
		return;

	mUpdateCnt++;
	NoahApp* anApp = mApp;
	// port(mod): the mods' keys (Mods.cpp) go to the board; a dialog that closed leaves no widget with the focus
	if (mApp->mWidgetManager->mFocusWidget == NULL)
		mApp->mWidgetManager->SetFocus(this);
	if (((anApp->mNextLevelWidget != NULL) || (anApp->mHighScoreWidget != NULL)) &&
		((mWaterY < -0x1d) || (anApp->mGameMode == GAMEMODE_STRATEGY)))
	{
		// between levels the sea goes on
		if (mUpdateCnt % 6 == 0)
		{
			UpdateFish();
			UpdateBubbles();
			MarkDirty();
		}
		return;
	}

	bool aPlayLanding = false;
	if (mUpdateCnt % 6 == 0)
		mAnimTick++;
	if ((mUpdateCnt % 3000 == 0) && (anApp->mGameMode == GAMEMODE_PUZZLE))
		mFloodSpeed++;
	if (mUpdateCnt % 3000 == 0)
		mSpawnRate++;

	if (mState == BOARD_ABANDON)
	{
		if (mAbandonAlpha < 0xff)
		{
			mAbandonAlpha += 4;
			if (mAbandonAlpha > 0xff)
				mAbandonAlpha = 0xff;
		}
		else if (mAbandonIndex < 0x50)
		{
			if (mUpdateCnt % 0x4e == 0)
				AbandonPair();
		}
		else
			EndRound();
	}
	else if ((mUpdateCnt % 3 == 0) && (mAbandonAlpha > 0))
	{
		mAbandonAlpha -= 10;
		if (mAbandonAlpha < 0)
			mAbandonAlpha = 0;
	}

	// Noah looks about now and then
	if ((mUpdateCnt % 0x24 == 0) && !mNoahWorried)
	{
		int aShore = mWaterY;
		int aLimit = mArkLevelY + 100;
		if (CrtRand() % 10 == 0)
		{
			int aChoices = (aLimit < aShore) ? 3 : 4;
			switch (CrtRand() % aChoices)
			{
			case 0: mNoahSprite = (aLimit < aShore) ? 0x3a : 0x40; break;
			case 1: mNoahSprite = (aLimit < aShore) ? 0x3c : 0x43; break;
			case 2: mNoahSprite = (aLimit < aShore) ? 0x3e : 0x45; break;
			case 3: mNoahSprite = 0x47; break;
			}
			mNoahFrame = 0;
		}
		if (CrtRand() % 3 == 0)
			mNoahFrame++;
		int aFrames = (mNoahSprite == 0x40) ? 3 : 2;
		mNoahFrame = abs(mNoahFrame % aFrames);
	}

	bool aSettled = true;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 8; y >= 0; y--)
		{
			Piece* p = mGrid[x][y];
			if (p == NULL)
				continue;

			// stampede: the animals ease back after the columns close in
			if ((p->mOffsetX != 0) && (mState == BOARD_PLAYING) && (anApp->mGameMode == GAMEMODE_STAMPEDE) && !p->mRemoved)
				Ease(p->mOffsetX, 2, 2);
			if ((p->mOffsetY > 0) && (mState == BOARD_PLAYING) && (anApp->mGameMode == GAMEMODE_STAMPEDE))
				Ease(p->mOffsetY, 2, 2);

			// blinking
			if ((p->mBlink >= 1) && (mUpdateCnt % 0x14 == 0))
			{
				p->mBlink++;
				if (p->mBlink == 4)
					p->mBlink = (CrtRand() % 5 != 0) ? 1 : 0;
			}
			else if ((p->mBlink == 0) && (CrtRand() % 10000 == 0))
				p->mBlink++;

			// puzzle: the two swapped animals slide into their places
			if ((anApp->mGameMode == GAMEMODE_PUZZLE) && ((mState == BOARD_SWAP) || (mState == BOARD_SWAP_BACK)) &&
				((p->mOffsetX != 0) || (p->mOffsetY != 0)))
			{
				if (p->mOffsetX < 0)
					p->mOffsetX += 2;
				else if (p->mOffsetX > 0)
					p->mOffsetX -= 2;
				if (p->mOffsetY < 0)
					p->mOffsetY += 2;
				else if (p->mOffsetY > 0)
					p->mOffsetY -= 2;
				aSettled = false;
			}

			if (!p->mMoving)
			{
				// strategy and puzzle: an animal with room below falls
				if (mGravity && ((p->mType < 0xd) || (p->mType == PIECE_BLOCK)) && !p->mRemoved && !p->mFading &&
					((mState == BOARD_PLAYING) || (mState == BOARD_ABANDON) || (mState == BOARD_SETTLE)) && (y < 8))
				{
					Piece* aBelow = mGrid[x][y + 1];
					if ((aBelow->mType == PIECE_EMPTY) || aBelow->mRemoved || aBelow->mFading ||
						(aBelow->mMoving && ((aBelow->mX != x) || (aBelow->mY != y + 1))))
					{
						// 0x40407c: the animal moves down a cell, and the cell it leaves gets a new piece that copies the one
						// below (its position, animal, offsets, flags and path); the old lower piece is deleted. The new
						// piece has the constructor's blink, scale and match fields, which matters: a blinking piece draws
						// no random number each update and one that is not does.
						Piece* aOldLower = mGrid[x][y + 1];
						Piece* aPartner = NULL;
						if ((aOldLower->mPartner != NULL) && (anApp->mGameMode != GAMEMODE_PUZZLE) && (anApp->mGameMode != GAMEMODE_STAMPEDE))
							aPartner = aOldLower->mPartner;
						Piece* aCopy = new Piece();
						aCopy->mX = aOldLower->mX;
						aCopy->mY = aOldLower->mY;
						aCopy->mDestX = aOldLower->mX;
						aCopy->mDestY = aOldLower->mY;
						aCopy->mPixelX = aOldLower->mPixelX;
						aCopy->mPixelY = aOldLower->mPixelY;
						aCopy->mType = aOldLower->mType;
						aCopy->mAlpha = aOldLower->mAlpha;
						aCopy->mOffsetX = aOldLower->mOffsetX;
						aCopy->mOffsetY = aOldLower->mOffsetY;
						aCopy->mSelected = aOldLower->mSelected;
						aCopy->mRemoved = aOldLower->mRemoved;
						aCopy->mFading = aOldLower->mFading;
						aCopy->mFallSpeed = aOldLower->mFallSpeed;
						aCopy->mFacingRight = aOldLower->mFacingRight;
						aCopy->mPathIndex = aOldLower->mPathIndex;
						aCopy->mMoving = aOldLower->mMoving;
						for (size_t i = 0; i < aOldLower->mPath.size(); i++)
						{
							if (aOldLower->mPath[i] != NULL)
								aCopy->mPath.push_back(new PathNode(*aOldLower->mPath[i]));
						}
						delete aOldLower;
						mGrid[x][y + 1] = mGrid[x][y];
						mGrid[x][y] = aCopy;
						Piece* aUpper = mGrid[x][y];
						Piece* aLower = mGrid[x][y + 1];
						if (aPartner != NULL)
						{
							aLower->mPartner = NULL;
							aUpper->mPartner = aPartner;
							aPartner->mPartner = aUpper;
						}
						if (aLower->mFallSpeed < -5)
							aLower->mFallSpeed = -6;
						aLower->mY = y + 1;
						aLower->mPixelY += 50.0f;
						aLower->mOffsetY -= 0x32;
						aUpper->mY = y;
						aSettled = false;
					}
				}
			}
			else if (p->mPartner != NULL)
			{
				Piece* aPartner = p->mPartner;
				if (!aPartner->mRemoved)
				{
					int n = (int) p->mPath.size();
					bool aArrived = true;
					if ((n > 0) && ((unsigned) p->mPathIndex < (unsigned) ((n >> 1) + 1)))
					{
						int aDX = CellLeft(aPartner->mX) + aPartner->mOffsetX - p->mOffsetX - CellLeft(p->mX);
						int aDY = CellTop(aPartner->mY) + aPartner->mOffsetY - p->mOffsetY - CellTop(p->mY);
						if ((abs(aDX) >= 5) || (abs(aDY) >= 5))
						{
							aArrived = false;
							if ((p->mOffsetX == 0) && (p->mOffsetY == 0))
							{
								if (p->mPathIndex < n)
								{
									PathNode* aNode = p->mPath[p->mPathIndex];
									if (aNode->mCellX < p->mX)
									{
										p->mOffsetX = 0x32;
										p->mX--;
									}
									else if (p->mX < aNode->mCellX)
									{
										p->mOffsetX = -0x32;
										p->mX++;
									}
									else if (aNode->mCellY < p->mY)
									{
										p->mOffsetY = 0x32;
										p->mY--;
									}
									else if (p->mY < aNode->mCellY)
									{
										p->mOffsetY = -0x32;
										p->mY++;
									}
									else
										p->mPathIndex++;
								}
							}
							else
							{
								if (p->mOffsetX > 0)
									p->mOffsetX -= 6;
								if (p->mOffsetX < 0)
									p->mOffsetX += 6;
								if (p->mOffsetY > 0)
									p->mOffsetY -= 6;
								if (p->mOffsetY < 0)
									p->mOffsetY += 6;
								if (abs(p->mOffsetX) < 6)
									p->mOffsetX = 0;
								if (abs(p->mOffsetY) < 6)
									p->mOffsetY = 0;
								if ((p->mOffsetX == 0) && (p->mOffsetY == 0))
									p->mPathIndex++;
							}
						}
					}
					if (aArrived)
					{
						// the two met: both are taken in
						p->mMoving = false;
						p->mRemoved = true;
						for (size_t i = 0; i < p->mPath.size(); i++)
							delete p->mPath[i];
						p->mPath.clear();
						p->mPixelX = (float) CellLeft(p->mX);
						p->mPixelY = (float) CellTop(p->mY);
						aPartner->mMoving = false;
						aPartner->mRemoved = true;
						for (size_t i = 0; i < aPartner->mPath.size(); i++)
							delete aPartner->mPath[i];
						aPartner->mPath.clear();
						aPartner->mPixelX = (float) CellLeft(aPartner->mX);
						aPartner->mPixelY = (float) CellTop(aPartner->mY);
						for (int i = 0; i < 4; i++)
						{
							int ax = (CrtRand() % 0x14) + 5 + (int) aPartner->mPixelX;
							int ay = (CrtRand() % 0x14) + 5 + (int) aPartner->mPixelY;
							AddSparkle(ax, ay);
						}
						if (mState != BOARD_ABANDON)
							anApp->PlaySample(p->mType + 1);
					}
				}
				else
				{
					p->mMoving = false;
					p->mRemoved = true;
					for (size_t i = 0; i < p->mPath.size(); i++)
						delete p->mPath[i];
					p->mPath.clear();
					p->mPixelX = (float) CellLeft(p->mX);
					p->mPixelY = (float) CellTop(p->mY);
				}
			}

			// falling pieces in the modes with gravity
			if (mGravity)
			{
				if ((p->mOffsetY < 0) && ((mState == BOARD_PLAYING) || (mState == BOARD_ABANDON) || (mState == BOARD_SETTLE)) &&
					!p->mMoving)
				{
					if (anApp->mGameMode == GAMEMODE_PUZZLE)
						mState = BOARD_SETTLE;
					int aFall = p->mFallSpeed;
					p->mOffsetY += (aFall < 0) ? 0 : aFall;
					if (mUpdateCnt % 4 == 0)
						p->mFallSpeed = aFall + 1;
					if (p->mOffsetY >= 0)
					{
						p->mOffsetY = 0;
						if ((p->mY > 7) || (mGrid[p->mX][p->mY + 1]->mType != PIECE_EMPTY &&
							!mGrid[p->mX][p->mY + 1]->mRemoved && !mGrid[p->mX][p->mY + 1]->mFading))
						{
							aPlayLanding = true;
							p->mFallSpeed = 1;
						}
					}
					aSettled = false;
				}
				if ((p->mOffsetY < 0) && ((mState == BOARD_PLAYING) || (mState == BOARD_ABANDON) || (mState == BOARD_SETTLE)) &&
					!p->mMoving)
					aSettled = false;
			}
		}
	}

	// puzzle: new animals drop into the holes at the top
	if (anApp->mGameMode == GAMEMODE_PUZZLE)
	{
		for (int x = 0; x < 9; x++)
		{
			Piece* p = mGrid[x][0];
			if (p->mType == PIECE_EMPTY)
			{
				int aKinds = anApp->mLevel + 3;
				if (aKinds < 0xe)
				{
					if (aKinds < 7)
						aKinds = 7;
				}
				else
					aKinds = 0xd;
				p->mType = CrtRand() % aKinds;
				p->mFallSpeed = 1;
				p->mY = 0;
				p->mPixelY = (float) CellTop(0);
				p->mOffsetY = -0x32;
				if (mGrid[x][1]->mOffsetY < 0)
					p->mOffsetY += mGrid[x][1]->mOffsetY;
				aSettled = false;
				p->mAlpha = 0xff;
			}
		}
	}

	// stampede: the waiting row rises and the board is pushed up; empty columns close in on the middle
	if ((anApp->mGameMode == GAMEMODE_STAMPEDE) && (mState == BOARD_PLAYING))
	{
		for (int x = 0; x < 9; x++)
		{
			int aStep = 10 - (mRowCount / 4);
			int aInterval = (aStep > 2) ? aStep : 3;
			if (mUpdateCnt % aInterval == 0)
				mNextRow[x]->mOffsetY -= 2;
		}
		if (mNextRow[0]->mOffsetY < 1)
			PushRow();
		for (int x = 5; x < 9; x++)
		{
			if (!ColumnEmpty(x) && ColumnEmpty(x - 1))
			{
				for (int y = 0; y < 9; y++)
					ExchangeContents(mGrid[x - 1][y], mGrid[x][y], 0x32, -0x32, false);
			}
		}
		for (int x = 3; x >= 0; x--)
		{
			if (!ColumnEmpty(x) && ColumnEmpty(x + 1))
			{
				for (int y = 0; y < 9; y++)
					ExchangeContents(mGrid[x + 1][y], mGrid[x][y], -0x32, -0x32, true);
			}
		}
	}

	if ((mState == BOARD_SETTLE) && aSettled)
	{
		if (!MarkRuns(true))
			mState = BOARD_PLAYING;
		else
		{
			if (MarkRuns(true))
			{
				aSettled = false;
				while (MarkRuns(true))
					;
			}
			anApp->PlaySample(SOUND_POP);
		}
	}
	if ((mState == BOARD_SWAP) && aSettled)
	{
		if (!MarkRuns(true))
		{
			mState = BOARD_SWAP_BACK;
			Piece* aFirst = NULL;
			Piece* aLast = NULL;
			for (int x = 0; x < 9; x++)
				for (int y = 0; y < 9; y++)
				{
					Piece* p = mGrid[x][y];
					if (p->mMatchSwap)
					{
						aLast = p;
						if (aFirst == NULL)
							aFirst = p;
					}
				}
			if ((aFirst == NULL) || (aLast == NULL))
				mState = BOARD_SETTLE;
			else
			{
				int aType = aFirst->mType;
				aFirst->mType = aLast->mType;
				aLast->mType = aType;
				if (aFirst->mX < aLast->mX)
				{
					aFirst->mOffsetX += 0x32;
					aLast->mOffsetX -= 0x32;
				}
				else if (aFirst->mX != aLast->mX)
				{
					aFirst->mOffsetX -= 0x32;
					aLast->mOffsetX += 0x32;
				}
				if (aFirst->mY < aLast->mY)
				{
					aSettled = false;
					aFirst->mOffsetY += 0x32;
					aLast->mOffsetY -= 0x32;
				}
				else
				{
					if (aFirst->mY != aLast->mY)
					{
						aFirst->mOffsetY -= 0x32;
						aLast->mOffsetY += 0x32;
					}
					aSettled = false;
				}
			}
		}
		else
		{
			while (MarkRuns(true))
				mState = BOARD_SETTLE;
			anApp->PlaySample(SOUND_POP);
		}
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
				mGrid[x][y]->mMatchSwap = false;
	}
	if ((mState == BOARD_SWAP_BACK) && aSettled)
		mState = BOARD_PLAYING;

	// hints and the end of the moves
	if ((mLink.size() < 2) && (mUpdateCnt - mLastMoveTick > 500) && (mState == BOARD_PLAYING) &&
		(anApp->mGameMode != GAMEMODE_STAMPEDE))
	{
		ShowHint();
		mLastMoveTick = mUpdateCnt;
	}
	if ((mUpdateCnt % 0x30 == 0) && (anApp->mGameMode != GAMEMODE_STAMPEDE) && !HasMove() && (AnimalCount() > 0) &&
		(mState != BOARD_LEVEL_DONE) && (mState != BOARD_WON) && !BoardCleared() && (mState != BOARD_ABANDON))
		TriggerNoMoves();

	if (mState == BOARD_GET_READY)
	{
		if (AnyWaiting() && (GameTickCount() - mStartTick > 400))
			ReleaseWaiting();
		if ((mWaterY < mWaterTargetY) && (anApp->mGameMode != GAMEMODE_STRATEGY))
		{
			mWaterY += 3;
			int aArk = mWaterY - 0x37;
			if (aArk > 0x61)
				aArk = 0x62;
			mArkY = aArk;
		}
		mShowGetReady = true;
		for (int x = 0; x < 9; x++)
		{
			for (int y = 0; y < 9; y++)
			{
				Piece* p = mGrid[x][y];
				if (CrtRand() % 100 == 0)
					p->mSelected = !p->mSelected;
				if ((CrtRand() % 0x6d6 == 0) && (p->mOffsetY < 0) && p->mPending && (p->mType < 0xd))
					anApp->PlaySample(p->mType + 1);
				if ((p->mOffsetY < 0) && p->mPending)
				{
					if ((mUpdateCnt & 1) == 0)
					{
						int aFall = p->mFallSpeed;
						int aStep = (aFall < 0) ? 0 : ((aFall < 0x24) ? aFall : 0x23);
						p->mOffsetY += aStep;
						p->mFallSpeed = aFall + 1;
						if (p->mOffsetY > 0)
						{
							p->mOffsetY = 0;
							p->mPending = false;
							p->mFallSpeed = 0x8c;
							if (mUpdateCnt % (CrtRand() % 6 + 1) == 0)
								aPlayLanding = true;
						}
					}
				}
				else if ((p->mOffsetY == 0) && !p->mPending && (p->mFallSpeed > 1))
					p->mFallSpeed--;
			}
		}
		if (!AnyFalling())
		{
			for (int x = 0; x < 9; x++)
				for (int y = 0; y < 9; y++)
					mGrid[x][y]->mSelected = false;
			if ((anApp->mGameMode == GAMEMODE_PUZZLE) && MarkRuns(true))
			{
				while (MarkRuns(true))
					;
				anApp->PlaySample(SOUND_POP);
			}
			else
				mState = BOARD_PLAYING;
			if (anApp->mGameMode == GAMEMODE_STAMPEDE)
			{
				ClearLinkList();
				ClearLink();
			}
			if (anApp->mGameMode != GAMEMODE_STRATEGY)
				mWaterY = mWaterTargetY;
		}
	}

	// animals that were taken in fly to the ark; the abandoned fade away
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			if (!p->mRemoved)
			{
				if (p->mFading)
				{
					// 0x405440: the distances are whole numbers (__ftol) and the length a double (x87 at 53 bits)
					double aTargetX = mWidth + 0x32;
					double aTargetY = mHeight + 0x32;
					double dx = (double) abs((int) (aTargetX - p->mPixelX));
					double dy = (double) abs((int) (aTargetY - p->mPixelY));
					double aDist = sqrt(dx * dx + dy * dy);
					if ((aTargetX <= p->mPixelX) || (aTargetY <= p->mPixelY))
					{
						p->mFading = false;
						p->mType = PIECE_EMPTY;
					}
					else
					{
						double sx = (dx / aDist) * 5.3;
						if (sx < 1.0)
							sx = 1.0;
						p->mPixelX = (float) (p->mPixelX + sx);
						double sy = (dy / aDist) * 5.3;
						if (sy < 1.0)
							sy = 1.0;
						p->mPixelY = (float) (p->mPixelY + sy);
					}
					// the x87 tests of 0.0 also take a NaN (a distance under one pixel divides 0 by 0) for zero
					if ((p->mPixelX == 0.0f) || (p->mPixelY == 0.0f) || (p->mPixelX != p->mPixelX) || (p->mPixelY != p->mPixelY))
					{
						p->mFading = false;
						p->mType = PIECE_EMPTY;
					}
				}
			}
			else
			{
				// 0x405408: the same arithmetic as above, towards the ark
				int aTargetX = mArkX + 0x42;
				int aTargetY = mArkY + 0x42;
				double dx = (double) abs((int) (aTargetX - (double) p->mPixelX));
				double dy = (double) abs((int) (aTargetY - (double) p->mPixelY));
				double aDist = sqrt(dx * dx + dy * dy);
				double aScale = p->mScale - 0.2;
				if (aScale < 35.0)
					aScale = 35.0;
				p->mScale = aScale;
				if (aDist < 100.0)
				{
					aScale = aScale - 0.2;
					if (aScale < 35.0)
						aScale = 35.0;
					p->mScale = aScale;
				}
				if (aDist < 150.0)
					p->mAlpha = (int) (aDist) * 2 + 0x69;
				if (((double) aTargetX < p->mPixelX) || (p->mPixelY != (float) aTargetY))
				{
					int sx = ((double) aTargetX < p->mPixelX) ? -1 : 1;
					int sy = ((float) aTargetY < p->mPixelY) ? -1 : 1;
					double nx = (dx / aDist) * sx * 5.3 + p->mPixelX;
					if (!(nx > (double) aTargetX))          // x87: a NaN counts as not above
						nx = (double) aTargetX;
					p->mPixelX = (float) nx;
					if ((float) aTargetY != p->mPixelY)
					{
						double ny = (dy / aDist) * sy * 5.3 + p->mPixelY;
						if (sy < 0)
						{
							if (!(ny > (double) aTargetY))
								ny = (double) aTargetY;
						}
						else if ((double) aTargetY <= ny)
							ny = (double) aTargetY;
						p->mPixelY = (float) ny;
					}
					if ((nx == 0.0) || (nx != nx) || (p->mPixelY == 0.0f) || (p->mPixelY != p->mPixelY))
					{
						p->mRemoved = false;
						p->mType = PIECE_EMPTY;
					}
				}
				else
				{
					p->mRemoved = false;
					p->mType = PIECE_EMPTY;
				}
			}

			// at the end of a level the rest sink
			if (((mUpdateCnt & 1) == 0) && (p->mAlpha < 0xff) && (p->mAlpha > 0) &&
				((mState == BOARD_LEVEL_DONE) || (mState == BOARD_WON) || (mState == BOARD_LOST)))
			{
				p->mAlpha--;
				p->mOffsetY++;
			}
			else if ((p->mAlpha < 0xff) && ((mState == BOARD_LEVEL_DONE) || (mState == BOARD_WON) || (mState == BOARD_LOST)))
				p->mOffsetY++;
		}
	}

	// the score on the display counts up to the real one
	if ((mUpdateCnt & 1) == 0)
	{
		int aShown = anApp->mShownScore;
		int aReal = anApp->mGameScore;
		if (aShown < aReal)
		{
			if (aReal - aShown > 100)
				anApp->mShownScore = aShown + 0x32;
			if (anApp->mGameScore - anApp->mShownScore > 10)
				anApp->mShownScore += 5;
			if (anApp->mShownScore < anApp->mGameScore)
				anApp->mShownScore++;
		}
		else if (aReal < aShown)
			anApp->mShownScore = aReal;
	}

	// lightning
	if (mUpdateCnt % 0xc == 0)
	{
		if (!mLightning)
		{
			if ((mState == BOARD_PLAYING) && (anApp->mGameMode != GAMEMODE_STRATEGY))
			{
				double aPercent = ((double) (mWaterY - mArkLevelY) / (double) (0x1a2 - mArkLevelY)) * 100.0;
				int aChance = (aPercent < 75.0) ? 1 : 0;
				if (aPercent < 50.0)
					aChance = 4;
				if (aPercent < 35.0)
					aChance = 10;
				if (aPercent < 20.0)
					aChance = 0xf;
				if (aPercent < 10.0)
					aChance = 0x14;
				if (CrtRand() % 0xfa < aChance)
				{
					mLightning = true;
					anApp->PlaySample(SOUND_THUNDER3 + (CrtRand() & 1));
					mState314 = mUpdateCnt;
				}
			}
		}
		else if ((CrtRand() & 1) == 0)
			mLightning = false;
	}
	if (mUpdateCnt % 0xc == 0)
	{
		if (mThunderCue)
			mThunderCue = false;
		int aMod;
		bool aWarn = false;
		if ((mWaterY < mArkLevelY + 10) && (mState == BOARD_PLAYING))
		{
			aMod = mUpdateCnt % 0x3c;
			aWarn = true;
		}
		else if ((mArkLevelY + 0x14 > mWaterY) && (mState == BOARD_PLAYING))
		{
			aMod = mUpdateCnt % 0x78;
			aWarn = true;
		}
		else
			aMod = 1;
		if (aWarn && (aMod == 0))
		{
			anApp->PlaySample(SOUND_WARNING);
			mThunderCue = true;
		}
	}
	if ((mStormTimer > 0) && (mUpdateCnt % 6 == 0))
		mStormTimer--;
	if (mUpdateCnt % 4 == 0)
		mWavePhaseB++;
	if (mUpdateCnt % 8 == 0)
		mWavePhaseA++;
	if (mUpdateCnt % 0xc == 0)
		mBackdropTick++;
	if ((mUpdateCnt % 6 == 0) && (mState == BOARD_ABANDON) && (AnimalCount() > 0) && HasMove() && (mArkLevelY < mWaterY))
		mState = BOARD_PLAYING;

	// the flood
	// port(mod): "tide" scales the pace, and 0 stops the water
	int aFloodPeriod = 0x22 - mFloodSpeed / 6;
	if (mTidePercent != 100)
		aFloodPeriod = (mTidePercent > 0) ? std::max(1, aFloodPeriod * 100 / mTidePercent) : 0;
	bool aFloodStops = (anApp->mGameMode == GAMEMODE_STRATEGY) || (anApp->mGameMode == GAMEMODE_STAMPEDE) ||
		(aFloodPeriod == 0) || (mUpdateCnt % aFloodPeriod != 0) || (mState != BOARD_PLAYING) || (AnimalCount() < 1);
	if (aFloodStops)
	{
		if (mState == BOARD_NO_MOVES)
		{
			if (mUpdateCnt % 6 == 0)
				mArkStage++;
			if (mArkStage == 4)
				mState = BOARD_WON;
		}
		else if (mState == BOARD_WON)
		{
			if (mUpdateCnt % 6 == 0)
				mArkStage++;
			if (mHeight + 10 < mWaterY)
				mWaterY = mHeight + 10;
			if (BoardCleared() && (anApp->mGameMode == GAMEMODE_ACTION))
			{
				int aBonus = anApp->mLevel / 2;
				if (aBonus < 1)
					aBonus = 1;
				mBonusPoints += aBonus * 2;
				anApp->mGameScore += aBonus * 2;
			}
			if ((mWaterY + 0x14 <= mArkLevelY) && (anApp->mGameMode != GAMEMODE_STRATEGY) && (anApp->mGameMode != GAMEMODE_PUZZLE))
			{
				mState = BOARD_LEVEL_DONE;
				mArkLeaving = true;
				AddFish(1, true);
				if (BoardCleared())
					anApp->ShowNextLevel();
			}
			if (((anApp->mGameMode == GAMEMODE_STRATEGY) || !BoardCleared()) && !anApp->mBonusRound &&
				(anApp->mHighScoreWidget == NULL))
			{
				int aWater = mWaterY;
				mArkLeaving = true;
				mWaterY = aWater - 1;
				mArkY--;
				if (aWater - 1 < -0x13)
				{
					GameOver();
					mState = BOARD_LEVEL_DONE;
				}
			}
			else if (anApp->mGameMode != GAMEMODE_STRATEGY)
				mWaterY--;
		}
		else if (mState == BOARD_LOST)
		{
			if (mNoahFrame > 10)
			{
				mState = BOARD_WON;
				mArkStage++;
			}
		}
		else if ((mState == BOARD_LEVEL_DONE) && (mWaterY > -0x1e) &&
			((anApp->mGameMode != GAMEMODE_STRATEGY) || !BoardCleared()))
		{
			mArkLeaving = true;
			if ((mUpdateCnt & 1) == 0)
			{
				mWaterY--;
				mArkY--;
			}
			mArkLeaving = true;
			if (mUpdateCnt % 6 == 0)
				mArkStage++;
			if (!BoardCleared() && !mSetUp3ac)
			{
				for (int x = 0; x < 9; x++)
					for (int y = 0; y < 9; y++)
						mGrid[x][y]->mAlpha = 0xfa;
				mSetUp3ac = true;
			}
			if (mWaterY % 0x1e == 0)
				AddFish(1, true);
			if (mWaterY < -0x1d)
			{
				if (!BoardCleared() && !anApp->mBonusRound && (anApp->mNextLevelWidget == NULL) &&
					(anApp->mHighScoreWidget == NULL))
					GameOver();
				else if (anApp->mBonusRound && (anApp->mNextLevelWidget == NULL))
					anApp->ShowNextLevel();
			}
		}
	}
	else
	{
		int aWater = mWaterY;
		mWaterY = aWater - 1;
		if (aWater - 1 < mArkLevelY + 0x14)
			ShowHintLine(2);
	}
	if (mArkLeaving && ((mUpdateCnt & 1) == 0))
		mArkX--;
	if (mNoahWorried && (mUpdateCnt % 0xe == 0))
		mNoahFrame++;
	if ((mWaterY < 0x109) && (mState == BOARD_PLAYING))
		mNoahWorried = true;
	if (mUpdateCnt % 6 == 0)
	{
		UpdateFish();
		UpdateBubbles();
	}
	if (mUpdateCnt % 3 == 0)
		UpdateTexts();
	UpdateSparkles();
	if (mState == BOARD_LOST)
	{
		mFade += 2;
		if (mFade > 0xff)
			mFade = 0xff;
	}
	if (mUpdateCnt % 0xc == 0)
	{
		if (mHintIndex > 0 || true)
		{
			if (mComboShown > 0)
				mComboShown--;
			if (mHintTimer > 0)
				mHintTimer--;
		}
	}
	if (mHintTimer < 1)
		mHintAlpha = std::max(0, mHintAlpha - 4);
	else
		mHintAlpha = std::min(0xff, mHintAlpha + 4);

	if ((mWaterY <= mArkLevelY) && ((mState == BOARD_PLAYING) || (mState == BOARD_ABANDON)))
		EndRound();

	mPulseA += mPulseADir * 5;
	if ((mPulseA < 0) || (mPulseA > 0xab))
	{
		mPulseADir = -mPulseADir;
		mPulseA = std::max(0, mPulseA);
		mPulseA = std::min(0xab, mPulseA);
	}
	mPulseB += mPulseBDir * 5;
	if ((mPulseB < 100) || (mPulseB > 0xfa))
	{
		mPulseBDir = -mPulseBDir;
		mPulseB = std::max(100, mPulseB);
		mPulseB = std::min(0xff, mPulseB);
	}

	if (BoardCleared() && (mState == BOARD_PLAYING) && (mArkLevelY < mWaterY) && (anApp->mGameMode < GAMEMODE_PUZZLE))
	{
		if (anApp->mGameMode == GAMEMODE_STRATEGY)
		{
			ClearLink();
			mState = BOARD_LEVEL_DONE;
			anApp->ShowNextLevel();
		}
		else
		{
			EndRound();
			anApp->PlaySample(SOUND_LEVELUP);
		}
	}
	if (aPlayLanding)
		anApp->PlaySample(SOUND_FALLINGCLICK);
	MarkDirty();
}
