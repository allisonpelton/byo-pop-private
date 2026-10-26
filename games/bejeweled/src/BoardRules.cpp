// Board.cpp, part 1 (0x4c9a94..0x4cb5bb): geometry, pause/game-over helpers, board generation and
// move search, input, banner text, and the clearing / game-over / level-wipe / fall-out states.
// The Board methods from the constructor (0x4cb5bc) on are translated separately.
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (section 3).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "ImageFont.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "ReplayRecorder.h"

#include <cmath>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <utility>

using namespace Sexy;

// The game's float math helpers compute in double and round the result to float.
// 0x4c9a94 (atan2f; StartBlackHole calls it through the atan2(float, float) overload at 0x4c9ad1)
static float Atan2F(float theY, float theX)
{
	return (float)atan2((double)theY, (double)theX);
}

// 0x4c9ab7
static float SqrtF(float theValue)
{
	return (float)sqrt((double)theValue);
}

// 0x4c9aec
Piece::Piece()
{
	// port: the original leaves the padding and the vortex fields (+0x60..+0x77) uninitialised
	memset(this, 0, sizeof(Piece));
}

// 0x4c9b25
int Board::GetColX(int theCol)
{
	return theCol * CELL_SIZE + mLayout.mOriginX; // port(variant): was BOARD_X
}

// 0x4c9b37
int Board::GetRowY(int theRow)
{
	return theRow * CELL_SIZE + mLayout.mOriginY; // port(variant): was BOARD_Y
}

// 0x4c9b47
void Board::GameOver(int theShakeTicks)
{
	if (mRecorder != NULL) mRecorder->OnGameOver(); // port(variant): how the game ended (END), seen before the state changes
	mSelectedPiece = NULL;
	mShowTutorial = false;
	mApp->KillDialog(DIALOG_INSTRUCTIONS);
	mState = STATE_GAME_OVER;
	mGameDuration = (int)((uint)(mUpdateCnt - mPlayStartTick) / 100) + mPlaySeconds;
	mGameOverCountdown = theShakeTicks;
	mApp->mTimesPlayed++;
	UsePendingSeed(); // port(variant): a game over uses up a seed the player chose (BoardVariant.cpp)
}

// 0x4c9bab
void Board::RequestNewGame()
{
	if (mRecorder != NULL) mRecorder->OnNewGame(); // port(variant): NEW_GAME
	mTakesPendingSeed = false; // port(variant): New Game is no game over: an unplayed seed goes on to the next board
	GameOver(50);
	mNewGamePending = true;
}

// 0x4c9bbe
bool Board::IsGameInProgress()
{
	if (mPlaybackOnly) return false; // port(variant): a replay's board never saves, counts or deletes anything
	return !mIsNewGame && mState != STATE_GAME_OVER && mState != STATE_GAME_OVER_DONE;
}

// 0x4c9bda
void Board::Pause(bool theUserPause)
{
	if (mPlaybackOnly && !mPlaybackApplying) return; // port(variant): a replay's board pauses only as its replay says (WatchController)
	if (mRecorder != NULL) mRecorder->OnPause(theUserPause); // port(variant): PAUSE u
	if (mPauseCount == 0)
	{
		// Commit the whole seconds played since play (re)started; the remainder is lost
		mPlaySeconds += (int)((uint)(mUpdateCnt - mPlayStartTick) / 100);
		MarkDirty();
	}

	if (theUserPause)
	{
		if (mUserPauseCount == 0)
			MarkDirty();
		mUserPauseCount++;
	}

	mPauseCount++;
}

// 0x4c9c2d
void Board::Unpause(bool theUserPause)
{
	if (mPlaybackOnly && !mPlaybackApplying) return; // port(variant)
	if (mRecorder != NULL) mRecorder->OnUnpause(theUserPause); // port(variant): UNPAUSE u
	mInputDelay = 10;

	if (theUserPause && mUserPauseCount > 0)
	{
		if (--mUserPauseCount <= 0)
			MarkDirty();
	}

	if (mPauseCount > 0)
	{
		if (--mPauseCount == 0)
			mPlayStartTick = mUpdateCnt;
	}
}

// 0x4c9c7c
Piece* Board::GetPieceAt(int theX, int theY)
{
	if (GravityUp() && (theY >= mLayout.mClipBottom)) return NULL; // port(variant): no rising gem under the board's clip (4.16)
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL &&
				aPiece->mX <= theX && theX < aPiece->mX + GEM_SIZE &&
				aPiece->mY <= theY && theY < aPiece->mY + GEM_SIZE)
				return aPiece;
		}
	}

	return NULL;
}

// 0x4c9cf5
bool Board::HasMatch()
{
	if (mVariant.mMatch) return HasMatchVariant(); // port(variant): the match rules (BoardMatch.cpp)
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		int aCount = 0;
		int aColor = -1;
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				aColor = -1;
			else if (aPiece->mColor == aColor)
			{
				if (++aCount == 3)
					return true;
			}
			else
			{
				aCount = 1;
				aColor = aPiece->mColor;
			}
		}
	}

	for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
	{
		int aCount = 0;
		int aColor = -1;
		for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				aColor = -1;
			else if (aPiece->mColor == aColor)
			{
				if (++aCount == 3)
					return true;
			}
			else
			{
				aCount = 1;
				aColor = aPiece->mColor;
			}
		}
	}

	return false;
}

// 0x4c9d94
int Board::GetPanForX(int theX)
{
	// -800 at the left edge of the board .. 800 at the right edge
	int aLeft = GetColX(0);
	int aWidth = GetColX(mCols - 1) + GEM_SIZE - aLeft; // port(variant): was BOARD_COLS
	return (int)((double)(theX - aWidth / 2 - aLeft) / aWidth * 2.0 * 800.0);
}

// 0x4c9dda
bool Board::FindMove(int* theMove, int theMoveNum, bool horizontal, bool vertical)
{
	if (mVariant.mSearch) return FindMoveVariant(theMove, theMoveNum, horizontal, vertical); // port(variant): the match rules (BoardMatch.cpp)
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

	int aMoveCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			if (mBoard[aRow][aCol] == NULL)
				continue;

			for (int aDir = 0; aDir < 4; aDir++)
			{
				int aCol2 = aCol + aDirs[aDir][0];
				int aRow2 = aRow + aDirs[aDir][1];
				if (aCol2 < 0 || aCol2 >= mCols || aRow2 < 0 || aRow2 >= mRows) // port(variant): was BOARD_COLS, BOARD_ROWS
					continue;
				if (mBoard[aRow2][aCol2] == NULL) // port(variant): a hole is no swap target
					continue;

				// Only the gem swapped into (aCol, aRow) is measured, so each swap can be found from
				// both of its cells.
				std::swap(mBoard[aRow][aCol], mBoard[aRow2][aCol2]);
				int aColor = mBoard[aRow][aCol]->mColor;

				// port: the original NULL-tests the fixed cells [aRow][aLeft+1] / [anUp+1][aCol] in the
				// right/down scans instead of the scanned cell; identical on the full boards all callers pass
				int aLeft = aCol;
				while (aLeft > 0 && mBoard[aRow][aLeft - 1] != NULL && mBoard[aRow][aLeft - 1]->mColor == aColor)
					aLeft--;
				int aRight = aCol;
				while (aRight < mCols - 1 && mBoard[aRow][aRight + 1] != NULL && mBoard[aRow][aRight + 1]->mColor == aColor) // port(variant): was BOARD_COLS
					aRight++;
				int anUp = aRow;
				while (anUp > 0 && mBoard[anUp - 1][aCol] != NULL && mBoard[anUp - 1][aCol]->mColor == aColor)
					anUp--;
				int aDown = aRow;
				while (aDown < mRows - 1 && mBoard[aDown + 1][aCol] != NULL && mBoard[aDown + 1][aCol]->mColor == aColor) // port(variant): was BOARD_ROWS
					aDown++;

				std::swap(mBoard[aRow][aCol], mBoard[aRow2][aCol2]);

				if ((aRight - aLeft >= 2 && horizontal) || (aDown - anUp >= 2 && vertical))
				{
					if (aMoveCount == theMoveNum)
					{
						if (theMove != NULL)
						{
							theMove[0] = aCol;
							theMove[1] = aRow;
							theMove[2] = aCol2;
							theMove[3] = aRow2;
						}
						return true;
					}
					aMoveCount++;
				}
			}
		}
	}

	return false;
}

// 0x4c9fd0
void Board::MouseMove(int x, int y)
{
	if (mPlaybackOnly) return; // port(variant): a replay's board takes its input from the replay only
	if (mVariant.mScaled) mLayout.ToLogical(&x, &y); // port(variant): a scaled board's input in logical space (5.2)
	mMouseX = x;
	mMouseY = y;
	if (mRecorder != NULL) mRecorder->OnHover(x, y); // port(variant): HOVER (the optional hover channel)
}

// 0x4c9fe9
void Board::MouseLeave()
{
	if (mPlaybackOnly) return; // port(variant)
	mMouseX = -1;
	mMouseY = -1;
	if (mRecorder != NULL) mRecorder->OnHover(-1, -1); // port(variant): HOVER -1 -1
}

// 0x4c9ff8
void Board::StartSwap(Piece* thePiece1, Piece* thePiece2)
{
	if (abs(thePiece1->mCol - thePiece2->mCol) + abs(thePiece1->mRow - thePiece2->mRow) != 1)
	{
		mApp->PlaySample(SOUND_BAD);
		return;
	}
	if (mVariant.mUndo) TakeUndoSnapshot(); // port(variant): undo, the board before the move (BoardVariant.cpp)

	// Only the demonstrated move (the tutorial's left gem swapped right) dismisses the tutorial here
	if (mShowTutorial && thePiece1->mCol == mTutorialCol && thePiece1->mRow == mTutorialRow &&
		thePiece2->mCol == mTutorialCol2 && thePiece2->mRow == mTutorialRow)
	{
		mApp->mShowInstructions = false;
		mShowTutorial = false;
		mApp->KillDialog(DIALOG_INSTRUCTIONS);
	}

	mSwapAngle = 0;
	mSwapPiece1 = thePiece1;
	mSwapPiece2 = thePiece2;
	mSwapCenterX = (int)((thePiece2->mX + thePiece1->mX) * 0.5);
	mSwapCenterY = (int)((thePiece1->mY + thePiece2->mY) * 0.5);
	mSwapPiece1->mSwapOffX = (int)(mSwapPiece1->mX - mSwapCenterX);
	mSwapPiece1->mSwapOffY = (int)(mSwapPiece1->mY - mSwapCenterY);
	mSwapPiece2->mSwapOffX = (int)(mSwapPiece2->mX - mSwapCenterX);
	mSwapPiece2->mSwapOffY = (int)(mSwapPiece2->mY - mSwapCenterY);
	mState = STATE_SWAPPING;
}

// 0x4ca13e
void Board::GetBoardSize(int* theWidth, int* theHeight)
{
	*theWidth = GetColX(mCols - 1) + GEM_SIZE - GetColX(0); // port(variant): was BOARD_COLS
	*theHeight = GetRowY(mRows - 1) + GEM_SIZE - GetRowY(0); // port(variant): was BOARD_ROWS
}

// 0x4ca179
void Board::GetBoardCenter(int* theX, int* theY)
{
	*theX = (GetColX(mCols - 1) + GEM_SIZE - GetColX(0)) / 2 + GetColX(0); // port(variant): was BOARD_COLS
	*theY = (GetRowY(mRows - 1) + GEM_SIZE - GetRowY(0)) / 2 + GetRowY(0); // port(variant): was BOARD_ROWS
}

// 0x4ca1c8
void Board::ShowText(const SexyString& theText, int theDuration)
{
	delete mTextImage;
	delete mTextColorImage;

	ImageFont* aFont = mApp->mOverFont;
	mTextImage = new MemoryImage(mApp);
	mTextImage->Create(aFont->StringWidth(theText), aFont->GetHeight());
	mTextImage->SetImageMode(true, true);

	// 48 extra rows for ColorizeText's sine offset
	mTextColorImage = new MemoryImage(mApp);
	mTextColorImage->Create(mTextImage->GetWidth(), mTextImage->GetHeight() + 48);
	mTextColorImage->SetImageMode(true, true);

	Graphics g(mTextImage);
	g.SetColor(Color(255, 255, 255));
	g.SetFont(mApp->mOverFont);
	g.DrawString(theText, 0, mApp->mOverFont->GetAscent());

	mTextPhase = 0.0;
	mTextDuration = theDuration;
	mTextTimeLeft = theDuration;
	mShowingText = true;
}

// 0x4ca36e
void Board::ColorizeText()
{
	ulong* aSrcBits = mTextImage->GetBits();
	ulong* aDestBits = mTextColorImage->GetBits();
	int aWidth = mTextImage->GetWidth();
	int aSrcHeight = mTextImage->GetHeight();
	int aDestHeight = mTextColorImage->GetHeight();
	memset(aDestBits, 0, aDestHeight * aWidth * sizeof(ulong));

	// The text fades in over the first 75 ticks and out over the last 75
	int anElapsed = mTextDuration - mTextTimeLeft;
	int aFade = 255;
	if (anElapsed < 75)
		aFade = anElapsed * 255 / 75;
	else if (mTextTimeLeft < 75)
		aFade = mTextTimeLeft * 255 / 75;

	// Vertical rainbow: the hue advances 4 per output row (the binary derives it from the byte offset
	// into the palette), so it repeats every 64 rows
	ulong* aPalette = new ulong[aDestHeight];
	for (int i = 0; i < aDestHeight; i++)
		aPalette[i] = mApp->HSLToRGB((i * 4) % 256, 255, 148);

	for (int aX = 0; aX < aWidth; aX++)
	{
		// Each column is shifted down 0..48 px along a sine wave that scrolls with mTextPhase
		int anOffset = (int)((sin(aX * 0.024 + mTextPhase) + 1.0) * 24.0);
		for (int aY = 0; aY < aSrcHeight; aY++)
		{
			ulong aSrc = aSrcBits[aY * aWidth + aX];
			ulong aColor = aPalette[anOffset + aY];
			// Each channel is text * rainbow / 256; alpha is text alpha * fade / 256
			aDestBits[(anOffset + aY) * aWidth + aX] =
				(((((aColor >> 16) & 0xFF) * (aSrc & 0xFF0000)) & 0xFF000000) |
				 ((((aColor >> 8) & 0xFF) * (aSrc & 0xFF00)) & 0xFF0000) |
				 (((aColor & 0xFF) * (aSrc & 0xFF)) & 0xFF00)) >> 8 |
				((((aSrc >> 8) & 0xFF0000) * aFade) & 0xFF000000);
		}
	}

	delete [] aPalette;
	mTextColorImage->BitsChanged();

	// port: with HD banner art (the text was drawn on HD pixels too, see MemoryImage::BeginHDMirror), the
	// same rainbow on the HD pixels; the sine and the hue follow the position in 1x units
	if (mTextImage->HasMirrorableHDBits())
	{
		int s = mTextImage->mHDScale;
		int aHDWidth = aWidth * s;
		int aHDSrcHeight = aSrcHeight * s;
		int aHDDestHeight = aDestHeight * s;
		const uint32_t* aHDSrc = &mTextImage->mHDBits[0];
		std::vector<uint32_t> aHDDest((size_t) aHDWidth * aHDDestHeight, 0);
		std::vector<uint32_t> aHDPalette(aHDDestHeight);
		for (int i = 0; i < aHDDestHeight; i++)
			aHDPalette[i] = (uint32_t) mApp->HSLToRGB(((i * 4) / s) % 256, 255, 148);

		for (int aX = 0; aX < aHDWidth; aX++)
		{
			int anOffset = (int)((sin((aX / (double) s) * 0.024 + mTextPhase) + 1.0) * 24.0 * s);
			for (int aY = 0; aY < aHDSrcHeight; aY++)
			{
				uint32_t aSrc = aHDSrc[(size_t) aY * aHDWidth + aX];
				uint32_t aColor = aHDPalette[anOffset + aY];
				aHDDest[(size_t) (anOffset + aY) * aHDWidth + aX] =
					(((((aColor >> 16) & 0xFF) * (aSrc & 0xFF0000)) & 0xFF000000) |
					 ((((aColor >> 8) & 0xFF) * (aSrc & 0xFF00)) & 0xFF0000) |
					 (((aColor & 0xFF) * (aSrc & 0xFF)) & 0xFF00)) >> 8 |
					((((aSrc >> 8) & 0xFF0000) * aFade) & 0xFF000000);
			}
		}
		mTextColorImage->SetHDBits(&aHDDest[0], aHDWidth, aHDDestHeight, s);
	}
}

// 0x4ca58d
void Board::StartBlackHole()
{
	if (mWhirlpoolSound != NULL)
		mWhirlpoolSound->Release();
	mWhirlpoolSound = mApp->mSoundManager->GetSoundInstance(SOUND_WHIRLPOOL);
	if (mWhirlpoolSound != NULL)
		mWhirlpoolSound->Play(true, false);

	mBlackHoleLevel = 0.0;
	mLevelTextShown = false;
	mBlackHoleFrame = 0;
	mSelectedPiece = NULL;
	if (mVariant.mUndo) mUndoStack.clear(); // port(variant): undo never crosses a level

	// Polar coordinates of each gem's center around the board center
	int aCenterX;
	int aCenterY;
	GetBoardCenter(&aCenterX, &aCenterY);
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			int aDeltaX = (int)(aPiece->mX + 24.0 - aCenterX);
			int aDeltaY = (int)(24.0 + aPiece->mY - aCenterY);
			float aDeltaXF = (float)aDeltaX;
			aPiece->mHoleAngle = Atan2F((float)aDeltaY, aDeltaXF);
			aPiece->mHoleRadius = SqrtF((float)((double)aDeltaXF * aDeltaXF + (double)(aDeltaY * aDeltaY)));
			aPiece->mHoleAngVel = 0.0;
		}
	}

	mState = STATE_BLACK_HOLE;
}

// 0x4ca6b0
void Board::MouseDown(int x, int y, int theClickCount)
{
	if (mPlaybackOnly) return; // port(variant)
	if (mVariant.mScaled) mLayout.ToLogical(&x, &y); // port(variant)
	if (mUserPauseCount != 0)
	{
		Unpause(true);
		return;
	}

	if (mInputDelay > 0)
		return;

	mMouseDown = true;
	mMouseDownX = x;
	mMouseDownY = y;

	if (mState == STATE_GAME_OVER || mState == STATE_GAME_OVER_DONE || mState == STATE_BLACK_HOLE || mState == STATE_LEVEL_WIPE)
		return;
	if (mVariant.mMoves && MouseDownVariant(x, y, theClickCount)) return; // port(variant): twists and slides (BoardMoves.cpp)

	if (theClickCount <= 0)
	{
		mSelectedPiece = NULL;
		MarkDirty();
		if (mRecorder != NULL) mRecorder->OnDeselect(); // port(variant): DESELECT
		return;
	}

	Piece* aTarget = GetPieceAt(x, y);
	if (mSelectedPiece == NULL)
	{
		// Selecting works in every state that gets this far, not only while idle
		if (aTarget == NULL)
			return;
		mCursorFrame = 0;
		mSelectedPiece = aTarget;
		MarkDirty();
		mApp->PlaySample(SOUND_SELECT, GetPanForX((int)(mSelectedPiece->mX + 24.0)));
		if (mRecorder != NULL) mRecorder->OnSelect(aTarget); // port(variant): SELECT c r
		return;
	}

	if (mState != STATE_PLAYING)
		return;

	// A click 22..88 px from the selected gem's center, within 26 px of one axis, targets the
	// neighbour in that direction (ties go vertical) wherever the clicked gem is
	int aDeltaX = (int)(x - (mSelectedPiece->mX + 24.0));
	int aDeltaY = (int)(y - (mSelectedPiece->mY + 24.0));
	int aCol = mSelectedPiece->mCol;
	int aRow = mSelectedPiece->mRow;
	int aMajor = (abs(aDeltaX) > abs(aDeltaY)) ? abs(aDeltaX) : abs(aDeltaY);
	int aMinor = (abs(aDeltaX) < abs(aDeltaY)) ? abs(aDeltaX) : abs(aDeltaY);
	if (aMajor >= 22 && aMajor <= 88.2 && aMinor <= 26)
	{
		if (abs(aDeltaX) > abs(aDeltaY))
			aCol += (aDeltaX < 0) ? -1 : 1;
		else
			aRow += (aDeltaY < 0) ? -1 : 1;

		if (aCol >= 0 && aCol < mCols && aRow >= 0 && aRow < mRows) // port(variant): was BOARD_COLS, BOARD_ROWS
			aTarget = mBoard[aRow][aCol];
	}

	// Clicking the selected gem again just deselects it; a non-adjacent target plays bad2
	if (aTarget != NULL && aTarget != mSelectedPiece)
		StartSwap(mSelectedPiece, aTarget);
	mSelectedPiece = NULL;
	MarkDirty();
	if (mRecorder != NULL) mRecorder->OnTarget(aTarget); // port(variant): TARGET c r (-1 -1 for none)
}

// 0x4ca8b7
void Board::MouseUp(int x, int y, int theClickCount)
{
	if (mPlaybackOnly) return; // port(variant)
	mMouseDown = false;
	if (mVariant.mMoves) MouseUpVariant(x, y); // port(variant): a slide, or a twist the press left for the release (BoardMoves.cpp)
}

// 0x4ca8c1
void Board::MouseDrag(int x, int y)
{
	if (mPlaybackOnly) return; // port(variant)
	if (mVariant.mScaled) mLayout.ToLogical(&x, &y); // port(variant)
	if (mVariant.mMoves && MouseDragVariant(x, y)) return; // port(variant): a slide's drag (BoardMoves.cpp)
	if (!mMouseDown || mSelectedPiece == NULL || mState != STATE_PLAYING)
		return;

	int aDeltaX = x - mMouseDownX;
	int aDeltaY = y - mMouseDownY;
	if (abs(aDeltaX) < 16 && abs(aDeltaY) < 16)
		return;

	// Ties go vertical; dragging off the edge of the board just deselects
	int aCol = mSelectedPiece->mCol;
	int aRow = mSelectedPiece->mRow;
	if (abs(aDeltaX) > abs(aDeltaY))
	{
		if (aDeltaX > 0 && aCol < mCols - 1 && mBoard[aRow][aCol + 1] != NULL) // port(variant): was BOARD_COLS; a hole is no neighbour
			StartSwap(mSelectedPiece, mBoard[aRow][aCol + 1]);
		else if (aDeltaX < 0 && aCol > 0 && mBoard[aRow][aCol - 1] != NULL) // port(variant)
			StartSwap(mSelectedPiece, mBoard[aRow][aCol - 1]);
	}
	else
	{
		if (aDeltaY > 0 && aRow < mRows - 1 && mBoard[aRow + 1][aCol] != NULL) // port(variant): was BOARD_ROWS
			StartSwap(mSelectedPiece, mBoard[aRow + 1][aCol]);
		else if (aDeltaY < 0 && aRow > 0 && mBoard[aRow - 1][aCol] != NULL) // port(variant)
			StartSwap(mSelectedPiece, mBoard[aRow - 1][aCol]);
	}

	mSelectedPiece = NULL;
	MarkDirty();
	if (mRecorder != NULL) mRecorder->OnTarget((mState == STATE_SWAPPING) ? mSwapPiece2 : NULL); // port(variant): TARGET: the neighbour a swap started with, -1 -1 off the edge
}

// 0x4ca9d3
void Board::KeyChar(SexyChar theChar)
{
	if (mPlaybackOnly) return; // port(variant)
	if ((mUserPauseCount == 0) && IsUndoKey(theChar) && UndoKeysActive()) { RequestUndo(); return; } // port(variant): undo, not a pause (BoardVariant.cpp)
	if (mUserPauseCount > 0)
		Unpause(true);
	else
		Pause(true);
}

// 0x4ca9ed
void Board::UpdateClearing()
{
	bool aShrinkDone = true;
	for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
	{
		for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL || !aPiece->mDying)
				continue;

			if (mUpdateCnt % 3 == 0)
			{
				if (++aPiece->mDyingFrame >= WinDMApp::NUM_SHRINK_FRAMES)
				{
					if (aPiece == mSelectedPiece)
						mSelectedPiece = NULL;
					delete aPiece;
					mBoard[aRow][aCol] = NULL;
				}
				MarkDirty();
			}
			aShrinkDone = false;
		}
	}

	// The refill waits for a tick on which no dying gem was left
	if (!aShrinkDone)
		return;

	// Each pass moves every gem above a column's lowest gap down one row and spawns one new gem on
	// top of that column, 69 px above its highest gem; passes repeat until no column has a gap.
	// Colors are not re-rolled, so the refill can cascade.
	if (mVariant.mDirt) RefillDiagonal(); else // port(variant): dirt floors and diagonal slides (BoardShape.cpp)
	if (mVariant.mFall) RefillMasked(); // port(variant): over each column's cells, through holes, in the gravity direction (BoardShape.cpp)
	else
	{
		int aTopY = GetRowY(0);
		bool aChanged;
		do
		{
			aChanged = false;
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				int aMinY = aTopY;
				bool aGap = false;
				for (int aRow = BOARD_ROWS - 1; aRow >= 0; aRow--)
				{
					Piece* aPiece = mBoard[aRow][aCol];
					if (aPiece == NULL)
					{
						if (aGap)
							mBoard[aRow + 1][aCol] = NULL;
						aGap = true;
					}
					else
					{
						if (aPiece->mY < aMinY)
							aMinY = (int)aPiece->mY;
						if (aGap)
						{
							aPiece->mRow++;
							aPiece->mVelY = 1.0;
							mBoard[aRow + 1][aCol] = aPiece;
						}
					}
				}

				if (aGap)
				{
					aChanged = true;
					Piece* aPiece = new Piece();
					aPiece->mVelY = 0.3;
					aPiece->mRow = 0;
					aPiece->mCol = aCol;
					aPiece->mX = GetColX(aCol);
					aPiece->mY = aMinY - 69;
					aPiece->mColor = Rand() % gGameConfig.mGemColors;
					mBoard[0][aCol] = aPiece;
				}
			}
		}
		while (aChanged);
	}

	MarkDirty();
	mState = STATE_FALLING;
	mCascadeCount++;
}

// 0x4cab82
void Board::UpdateGameOver()
{
	MarkDirty();

	if (mGameOverCountdown > 0)
	{
		if (--mGameOverCountdown == 0)
			mApp->PlaySample(SOUND_EXPLODE);

		// Shake every 6 ticks; on the last tick also give every gem its explosion velocity
		if (mUpdateCnt % 6 == 0 || mGameOverCountdown == 0)
		{
			for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
			{
				for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
				{
					Piece* aPiece = mBoard[aRow][aCol];
					if (aPiece == NULL)
						continue;

					aPiece->mX = GetColX(aPiece->mCol) + Rand() % 9 - 4;
					aPiece->mY = GetRowY(aPiece->mRow) + Rand() % 9 - 4;
					if (mGameOverCountdown == 0)
					{
						aPiece->mVelY = Rand() % 8 - 6;
						aPiece->mVelX = Rand() % 7 - 3;
					}
				}
			}
		}
	}
	else
	{
		bool allGone = true;
		if (mVariant.mFall) allGone = GameOverFallVariant(); else // port(variant): the fall in the gravity direction (BoardShape.cpp)
		for (int aRow = 0; aRow < mRows; aRow++) // port(variant): was BOARD_ROWS
		{
			for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece == NULL)
					continue;

				aPiece->mX += aPiece->mVelX;
				aPiece->mY += aPiece->mVelY;
				aPiece->mVelY += 0.15;
				if (aPiece->mY < mLayout.mScreenBottom + GEM_SIZE) // port(variant): was mHeight
					allGone = false;
			}
		}

		if (allGone)
		{
			if (mNewGamePending)
				mApp->NewGame();
			else
			{
				mApp->PlaySample(SOUND_GAMEOVER);
				mState = STATE_GAME_OVER_DONE;
				mApp->ShowHighScoreWidget();
			}
		}
	}
}

// 0x4cad2f
void Board::DoHint()
{
	if (mRecorder != NULL) mRecorder->OnHint(); // port(variant): HINT (ShowHint's only call, inside its if; playback calls DoHint)
	mScore = (int)((double)mScore - mPointMultiplier * gGameConfig.mHintPenalty);
	if (mScore < 0)
		mScore = 0;

	int aMove[4];
	if (FindMove(aMove, 0, true, true))
	{
		// Sparkle and spin the gem the move would bring in
		Piece* aPiece = mBoard[aMove[3]][aMove[2]];
		aPiece->mSparkleCount = 58;
		aPiece->mSparkleFrame = 0;
		aPiece->mSpinFrame = 1;
	}

	mIdleTicks = 0;
}

// 0x4cae29
void Board::SetupLevel()
{
	Graphics g(mNextBackgroundImage);
	int aColorIdx = mLevel % 4;
	int aTextureIdx = mLevel % 5;
	mBackgroundChanged = true;

	// The two checker cells: the level's texture, tinted with the level's colors. Tile B is created
	// 102 px wide but only its left 51 px are drawn into.
	MemoryImage aTileA(mApp);
	aTileA.Create(51, 51);
	Graphics aTileAG(&aTileA);
	aTileAG.DrawImage(mApp->mImages[IMAGE_CHECKERTEXTURES], 0, 0, Rect(aTextureIdx * 102, 0, 51, 51));

	MemoryImage aTileB(mApp);
	aTileB.Create(102, 51);
	Graphics aTileBG(&aTileB);
	aTileBG.DrawImage(mApp->mImages[IMAGE_CHECKERTEXTURES], 0, 0, Rect(aTextureIdx * 102 + 51, 0, 51, 51));

	const int* aColorA = gBoardTileColors[aColorIdx][0];
	DDImage* anImageA = mApp->CreateColorizedImage(&aTileA, Color(aColorA[0], aColorA[1], aColorA[2]));
	const int* aColorB = gBoardTileColors[aColorIdx][1];
	DDImage* anImageB = mApp->CreateColorizedImage(&aTileB, Color(aColorB[0], aColorB[1], aColorB[2]));

	// 10x10 cells aligned with the board grid, starting one cell above and left of it
	if (mVariant.mChecker) DrawVariantChecker(&g, anImageA, anImageB); // port(variant): the level's shape, holes and all; the twilight backdrop (BoardShape.cpp)
	else
	for (int aRow = -1, aY = -35; aY < 425; aY += 51, aRow++)
		for (int aCol = -1, aX = 151; aX < 611; aX += 51, aCol++)
			g.DrawImage(((aRow + aCol) % 2 == 0) ? anImageA : anImageB, aX, aY);

	delete anImageA;
	delete anImageB;

	g.DrawImage(mApp->mImages[IMAGE_ALPHABACK], 0, 0);

	// Level parameters from data\diff.txt
	mTimeDrain = 0.0;
	double aLevel = mLevel;
	double aPoints;
	if (mTimedMode)
	{
		mPointMultiplier = mApp->mTimedMultPerLevel * aLevel + mApp->mTimedMultBase;
		aPoints = mApp->mTimedPointsPerLevel * aLevel + mApp->mTimedPointsBase;
	}
	else
	{
		mPointMultiplier = mApp->mNormalMultPerLevel * aLevel + mApp->mNormalMultBase;
		aPoints = mApp->mNormalPointsPerLevel * aLevel + mApp->mNormalPointsBase;
	}
	mLevelPoints = (int)(aPoints * mPointMultiplier);
	mTimeDrainRate = (mApp->mTimeDrainPerLevel * aLevel + mApp->mTimeDrainBase) * mPointMultiplier;
	mTimeDrainAccel = (aLevel * mApp->mTimeDrainAccelPerLevel + mApp->mTimeDrainAccelBase) * mPointMultiplier;
	if (mVariant.mGold) SetupLevelGold(); // port(variant): no gold yet, the level's time (BoardGold.cpp)
}

// 0x4cb182
void Board::FillBoard()
{
	if (mVariant.mFall) { FillBoardVariant(); return; } // port(variant): holes skipped, the gravity direction (BoardShape.cpp)
	mState = STATE_FALLING;
	mIdleTicks = 0;

	// port: the original drops the previous pieces without deleting them. Park them for ~Board, except
	// a stale mSelectedPiece: a later swap can put it back into the grid, so it stays unowned (leaked).
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			if (mBoard[aRow][aCol] != NULL && mBoard[aRow][aCol] != mSelectedPiece)
				mOrphanedPieces.push_back(mBoard[aRow][aCol]);
		}
	}
	memset(mBoard, 0, sizeof(mBoard));

	// Bottom row first; each gem starts 65..304 px above the one below it and gets colors rolled
	// until the board has no 3-in-a-row
	for (int aRow = BOARD_ROWS - 1; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = new Piece();
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = GetColX(aCol);
			int aBelowY = (aRow < BOARD_ROWS - 1) ? (int)mBoard[aRow + 1][aCol]->mY : 0;
			aPiece->mY = aBelowY - Rand() % 240 - 65;
			mBoard[aRow][aCol] = aPiece;
			do
			{
				aPiece->mColor = Rand() % gGameConfig.mGemColors;
			}
			while (HasMatch());
		}
	}
}

// 0x4cb284
void Board::UpdateLevelWipe()
{
	Graphics g(mBackgroundImage);
	g.ClipRect(0, mWipeY, mWidth, 3);
	g.DrawImage(mNextBackgroundImage, 0, 0);

	mBlackHoleLevel -= 0.015;
	if (mBlackHoleLevel >= 0.0)
	{
		if (mWhirlpoolSound != NULL)
			mWhirlpoolSound->SetVolume(mBlackHoleLevel);
	}
	else
	{
		mBlackHoleLevel = 0.0;
		if (mWhirlpoolSound != NULL)
		{
			mWhirlpoolSound->Release();
			mWhirlpoolSound = NULL;
		}
	}

	if (mUpdateCnt % 4 == 0)
		mElectOffset = Rand() % 128;

	MarkDirty();
	mWipeY += 3;
	if (mWipeY > mHeight)
	{
		mDisplayLevel = mLevel + 1;
		mTimeDrainDelay = gGameConfig.mLevelDrainDelay;
		FillBoard();
		// Unregistered copies with 15+ games played roll Rand() % 5 here for the register nag (not ported).
	}
}

// 0x4cb3bf
void Board::UpdateFallOut()
{
	if (mVariant.mFall) { UpdateFallOutVariant(); return; } // port(variant): the gravity direction (BoardShape.cpp)
	bool allGone = true;
	MarkDirty();

	for (int aCol = 0; aCol < mCols; aCol++) // port(variant): was BOARD_COLS
	{
		// One draw per column per tick, even for an empty column; on a hit the column's lowest
		// resting gem starts to fall (it moves from the next tick on)
		bool aRelease = Rand() % 15 == 0;
		for (int aRow = mRows - 1; aRow >= 0; aRow--) // port(variant): was BOARD_ROWS
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if (aPiece->mVelY == 0.0)
			{
				if (aRelease)
				{
					aPiece->mVelY = 1.0;
					aRelease = false;
				}
			}
			else
			{
				aPiece->mY += aPiece->mVelY;
				aPiece->mVelY += 0.18;
			}

			if (aPiece->mY < mLayout.mScreenBottom + 32) // port(variant): was mHeight
				allGone = false;
		}
	}

	if (allGone)
	{
		mTimeDrainDelay = gGameConfig.mLevelDrainDelay;
		FillBoard();
	}
}

// 0x4cb475
void Board::ShowHint()
{
	if (mPlaybackOnly) return; // port(variant)
	if (NoHintVariant()) return; // port(variant): no move to show (a noMoves "continue" board): no hint, no penalty (BoardVariant.cpp)
	if (mState == STATE_PLAYING && mPauseCount == 0)
		DoHint();
}
