// port: test hook for -tracehash=<file>. WinDMApp::UpdateFrames appends one line after every update:
//
//   <update> <board state> <logic hash> <full hash>
//
// <board state> is Board::mState (-1 without a board), so a run's state changes can be read from the
// file. The hashes let two builds be compared tick by tick: a difference shows up at the first update
// where it happens. Both are 64-bit FNV-1a (16 hex digits), values written little-endian: int as 4
// bytes, bool as 1, double as its IEEE-754 bit pattern (8 bytes); pointers are written as grid cells.
//
// FULL (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.6): everything, for the oracle. The raw 0x78 Piece bytes
// (what savegame.dat stores, padding included) of every cell, every Board field in declaration order
// (cosmetic ones too: glow, hover, banner, popups, tutorial bob, sound throttles), the global RNG state
// (draw count and state words), and the app fields below. Same platform only (raw doubles from libm).
// Left out: images, fonts and sounds (only whether a banner image exists), mUnusedString, and
// mWhirlpoolSound, which depends on the audio device. Nothing the hash reads is written by Draw (the
// banner's ColorizeText only writes image bits), so the draw schedule (-fast, -renderscale) does not
// change it; with script or -autoplay input the mouse fields are as deterministic as the clicks.
//
// LOGIC: the hand-written list below, the state later play can depend on; a narrower hash that a
// cosmetic change (glow, sparkle, vortex spiral) leaves alone. Stage 3's StateCodec LOGIC hash, which
// native-vs-wasm and replays will compare, replaces it. Rule: a field is in the list if a later decision
// of the game (a state change, a score, a Rand() draw, the handling of an input, what a save or load
// writes) can depend on it, or if it is a Board field that savegame.dat stores. Left out on purpose:
//   - Piece: mGlowing, mGlowAlpha, mGlowFrame (hover glow), mSpinFrame, mSparkleCount, mSparkleFrame
//     (hint sparkle), mHoleAngle and mHoleAngVel (vortex spiral), the padding, and mX/mY while the vortex
//     runs (state 7: positions are only drawn there, MouseDown returns early and the gems are deleted
//     when the vortex ends; LoadGame snaps them back to the grid). They are drawn and never read by play.
//   - Board: mCursorFrame, mTutorialBob(Dir), mMoveScoreAlpha, mHintGlowAlpha, mHintGlowTimer,
//     mHintGlowDuration (hint-button glow), mScorePopups, the banner (mShowingText, mTextPhase,
//     mTextDuration, mTextTimeLeft, the text images), mMouseX/Y (hover only: they feed the glow),
//     mLastLandSoundTick and mLastWarningTick (sound throttles), images, fonts and sounds,
//     mBackgroundChanged (dead), mOrphanedPieces (port bookkeeping; a stale selection is hashed below).
// Kept although they look cosmetic:
//   - gem positions outside state 7: GetPieceAt reads them for a click in states 0..4 and 9 (also in
//     mid-swap), and falls, the fall-out and the game-over explosion end on them;
//   - mHoleRadius: the vortex ends when every radius is below 0, and the radius sets the dying frames;
//   - mBlackHoleFrame, mBlackHoleLevel, mBarColor, mLevelTextShown: savegame.dat stores them;
//   - mMoveScoreTimer: in the LOGIC field list of games/bejeweled/docs/CODE_MODS_DESIGN.md 6.6.
// App state (both hashes): the global Rand() draw count (Sexy::GetRandCount), the mode,
// mShowInstructions and mTimesPlayed (both decide the tick-150 tutorial), the open dialog ids and whether
// the game-over HighScoreWidget exists (they route and pause input); FULL adds the title screen.
// port(variant), set seed (both hashes, after the rest): a pending seed the player chose, and a board's own
// RNG (seed, the player's or not, whether it still holds the pending seed, Board::Rand count, MTRand state
// words and index). They are hashed only when present, so an original game, which has neither, hashes as
// in the stage-0 build.
//
// Portability: no pointer, padding or map-order dependence in LOGIC (its only libm-derived values are
// swap positions, (int)-truncated), but nothing runs it on the web build yet: web/shell.html passes no
// test switches. Stage 4's Node/wasm runner has to accept them to compare LOGIC across platforms.

#include "StateHash.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "Common.h"

#include <cstring>

using namespace Sexy;

namespace
{

class StateHasher
{
public:
	uint64_t mHash;

	StateHasher() : mHash(14695981039346656037ULL) {}

	void Byte(uint8_t theByte)
	{
		mHash ^= theByte;
		mHash *= 1099511628211ULL;
	}

	void Bytes(const void* theData, size_t theSize)
	{
		const uint8_t* aData = (const uint8_t*) theData;
		for (size_t i = 0; i < theSize; i++)
			Byte(aData[i]);
	}

	void Int(int theValue)
	{
		uint32_t aValue = (uint32_t) theValue;
		for (int i = 0; i < 4; i++)
			Byte((uint8_t) (aValue >> (i * 8)));
	}

	void Bool(bool theValue)
	{
		Byte(theValue ? 1 : 0);
	}

	void Double(double theValue)
	{
		uint64_t aBits;
		memcpy(&aBits, &theValue, sizeof(aBits));
		for (int i = 0; i < 8; i++)
			Byte((uint8_t) (aBits >> (i * 8)));
	}
};

void HashPiece(StateHasher& h, Board* theBoard, Piece* thePiece, bool full)
{
	if (full)
	{
		h.Bytes(thePiece, Piece::SAVE_SIZE);  // the savegame.dat record (Piece.h)
		return;
	}

	h.Int(thePiece->mColor);
	h.Int(thePiece->mCol);
	h.Int(thePiece->mRow);
	if (theBoard->mState != STATE_BLACK_HOLE)
	{
		h.Double(thePiece->mX);
		h.Double(thePiece->mY);
	}
	h.Int(thePiece->mVelX);
	h.Double(thePiece->mVelY);
	h.Int(thePiece->mSwapOffX);
	h.Int(thePiece->mSwapOffY);
	h.Bool(thePiece->mDying);
	h.Int(thePiece->mDyingFrame);
	h.Double(thePiece->mHoleRadius);
}

// A piece pointer as its cell (row * cols + col), -1 for NULL, or rows * cols and the piece's fields for a
// piece that is not in the grid (a stale selection that FillBoard dropped, GAME_NOTES 3.6); 8x8: 64
void HashPieceRef(StateHasher& h, Board* theBoard, Piece* thePiece, bool full)
{
	if (thePiece == NULL)
	{
		h.Int(-1);
		return;
	}

	for (int aRow = 0; aRow < theBoard->mRows; aRow++)
	{
		for (int aCol = 0; aCol < theBoard->mCols; aCol++)
		{
			if (theBoard->mBoard[aRow][aCol] == thePiece)
			{
				h.Int(aRow * theBoard->mCols + aCol);
				return;
			}
		}
	}

	h.Int(theBoard->mRows * theBoard->mCols);
	HashPiece(h, theBoard, thePiece, full);
}

void HashCells(StateHasher& h, Board* b, bool full)
{
	for (int aRow = 0; aRow < b->mRows; aRow++)
	{
		for (int aCol = 0; aCol < b->mCols; aCol++)
		{
			Piece* aPiece = b->mBoard[aRow][aCol];
			h.Bool(aPiece != NULL);
			if (aPiece != NULL)
				HashPiece(h, b, aPiece, full);
		}
	}
}

// port(variant): a board's own RNG (stage 2); nothing for a board on the global stream
void HashOwnRng(StateHasher& h, Board* b)
{
	if (b->mOwnRng == NULL)
		return;
	h.Int((int) b->mGameSeed);
	h.Bool(b->mUserSeeded);
	h.Bool(b->mTakesPendingSeed);
	h.Int((int) b->mBoardDraws);
	uint32_t aState[MTRAND_N];
	int anIndex;
	b->mOwnRng->GetState(aState, &anIndex);
	h.Bytes(aState, sizeof(aState));
	h.Int(anIndex);
}

void HashBoardLogic(StateHasher& h, Board* b)
{
	h.Int(b->mUpdateCnt);  // board tick (Widget::mUpdateCnt)
	h.Int(b->mLevel);
	h.Int(b->mDisplayLevel);
	h.Int(b->mState);
	HashCells(h, b, false);

	h.Bool(b->mTimedMode);
	h.Bool(b->mIsNewGame);
	HashPieceRef(h, b, b->mSelectedPiece, false);
	h.Bool(b->mShowTutorial);
	h.Int(b->mTutorialCol);
	h.Int(b->mTutorialRow);
	h.Int(b->mTutorialCol2);
	HashPieceRef(h, b, b->mSwapPiece1, false);
	HashPieceRef(h, b, b->mSwapPiece2, false);
	h.Int(b->mSwapAngle);
	h.Int(b->mSwapCenterX);
	h.Int(b->mSwapCenterY);
	h.Int(b->mBarWidth);
	h.Int(b->mPlayStartTick);
	h.Int(b->mPlaySeconds);
	h.Int(b->mGameDuration);
	h.Int(b->mIdleTicks);
	h.Int(b->mScore);
	h.Double(b->mPointMultiplier);
	h.Int(b->mDisplayScore);
	h.Int(b->mGemsCleared);
	h.Int(b->mMoveMatchCount);
	h.Int(b->mCascadeCount);
	h.Int(b->mGameOverCountdown);
	h.Int(b->mLongestCascade);
	h.Int(b->mBestMoveScore);
	h.Int(b->mUnused218);
	h.Int(b->mMoveScore);
	h.Int(b->mMoveScoreTimer);
	h.Bool(b->mNewGamePending);
	h.Int(b->mPauseCount);
	h.Int(b->mUserPauseCount);
	h.Bool(b->mStatsDisabled);
	h.Int(b->mUnused264);
	h.Int(b->mLevelStartScore);
	h.Int(b->mLevelTargetScore);
	h.Int(b->mLevelPoints);
	h.Double(b->mTimeDrain);
	h.Double(b->mTimeDrainRate);
	h.Double(b->mTimeDrainAccel);
	h.Int(b->mBarColor);
	h.Int(b->mMouseDownX);
	h.Int(b->mMouseDownY);
	h.Bool(b->mMouseDown);
	h.Int(b->mInputDelay);
	h.Int(b->mWipeY);
	h.Int(b->mElectOffset);
	h.Int(b->mBlackHoleFrame);
	h.Int(b->mTimeDrainDelay);
	h.Double(b->mBlackHoleLevel);
	h.Bool(b->mLevelTextShown);
	HashOwnRng(h, b);
}

// Every Board field in declaration order (Board.h), pieces as raw bytes
void HashBoardFull(StateHasher& h, Board* b)
{
	h.Int(b->mUpdateCnt);  // board tick (Widget::mUpdateCnt)
	h.Int(b->mLevel);
	h.Int(b->mDisplayLevel);
	HashCells(h, b, true);
	h.Int(b->mState);
	h.Bool(b->mTimedMode);
	h.Bool(b->mIsNewGame);
	HashPieceRef(h, b, b->mSelectedPiece, true);
	h.Int(b->mCursorFrame);
	h.Bool(b->mShowTutorial);
	h.Int(b->mTutorialBob);
	h.Int(b->mTutorialBobDir);
	h.Int(b->mTutorialCol);
	h.Int(b->mTutorialRow);
	h.Int(b->mTutorialCol2);
	HashPieceRef(h, b, b->mSwapPiece1, true);
	HashPieceRef(h, b, b->mSwapPiece2, true);
	h.Int(b->mSwapAngle);
	h.Int(b->mSwapCenterX);
	h.Int(b->mSwapCenterY);
	h.Int(b->mBarWidth);
	h.Int(b->mPlayStartTick);
	h.Int(b->mPlaySeconds);
	h.Int(b->mGameDuration);
	h.Int(b->mIdleTicks);
	h.Int(b->mScore);
	h.Double(b->mPointMultiplier);
	h.Int(b->mDisplayScore);
	h.Int(b->mGemsCleared);
	h.Int(b->mMoveMatchCount);
	h.Int(b->mCascadeCount);
	h.Int(b->mLastLandSoundTick);
	h.Int(b->mGameOverCountdown);
	h.Int(b->mLongestCascade);
	h.Int(b->mBestMoveScore);
	h.Int(b->mUnused218);
	h.Int(b->mMoveScore);
	h.Int(b->mMoveScoreTimer);
	h.Double(b->mMoveScoreAlpha);
	h.Double(b->mHintGlowAlpha);
	h.Int(b->mHintGlowTimer);
	h.Int(b->mHintGlowDuration);
	h.Bool(b->mNewGamePending);
	h.Int(b->mPauseCount);
	h.Int(b->mUserPauseCount);
	h.Int((int) b->mScorePopups.size());
	for (std::list<ScorePopup>::iterator anItr = b->mScorePopups.begin(); anItr != b->mScorePopups.end(); ++anItr)
	{
		h.Int(anItr->mX);
		h.Int(anItr->mY);
		h.Int(anItr->mLife);
		h.Int(anItr->mUnused);
		h.Int(anItr->mPoints);
	}
	h.Bool(b->mStatsDisabled);
	h.Int(b->mUnused264);
	h.Int(b->mLevelStartScore);
	h.Int(b->mLevelTargetScore);
	h.Int(b->mLevelPoints);
	h.Double(b->mTimeDrain);
	h.Double(b->mTimeDrainRate);
	h.Double(b->mTimeDrainAccel);
	h.Int(b->mBarColor);
	h.Int(b->mLastWarningTick);
	h.Int(b->mMouseX);
	h.Int(b->mMouseY);
	h.Int(b->mMouseDownX);
	h.Int(b->mMouseDownY);
	h.Bool(b->mMouseDown);
	h.Int(b->mInputDelay);
	h.Bool(b->mBackgroundChanged);
	h.Bool(b->mTextImage != NULL);
	h.Bool(b->mTextColorImage != NULL);
	h.Bool(b->mShowingText);
	h.Double(b->mTextPhase);
	h.Int(b->mTextDuration);
	h.Int(b->mTextTimeLeft);
	h.Int(b->mWipeY);
	h.Int(b->mElectOffset);
	h.Int(b->mBlackHoleFrame);
	h.Int(b->mTimeDrainDelay);
	h.Double(b->mBlackHoleLevel);
	h.Bool(b->mLevelTextShown);
	h.Int((int) b->mOrphanedPieces.size());
	HashOwnRng(h, b);
}

void HashApp(StateHasher& h, WinDMApp* theApp)
{
	h.Int((int) GetRandCount());
	h.Bool(theApp->mTimedMode);
	h.Bool(theApp->mShowInstructions);
	h.Int(theApp->mTimesPlayed);
	h.Bool(theApp->mHighScoreWidget != NULL);

	h.Int((int) theApp->mDialogMap.size());
	for (DialogMap::iterator anItr = theApp->mDialogMap.begin(); anItr != theApp->mDialogMap.end(); ++anItr)
		h.Int(anItr->first);
	if (gGameConfig.mPendingSeed != 0)
		h.Int((int) gGameConfig.mPendingSeed);
}

}

void Sexy::HashGameState(WinDMApp* theApp, uint64_t* theLogicHash, uint64_t* theFullHash)
{
	StateHasher aLogic;
	HashApp(aLogic, theApp);
	aLogic.Bool(theApp->mBoard != NULL);
	if (theApp->mBoard != NULL)
		HashBoardLogic(aLogic, theApp->mBoard);
	*theLogicHash = aLogic.mHash;

	StateHasher aFull;
	HashApp(aFull, theApp);
	std::string aRandState = GetRandState();
	aFull.Bytes(aRandState.data(), aRandState.size());
	aFull.Bool(theApp->mTitleScreen != NULL);
	aFull.Bool(theApp->mBoard != NULL);
	if (theApp->mBoard != NULL)
		HashBoardFull(aFull, theApp->mBoard);
	*theFullHash = aFull.mHash;
}
