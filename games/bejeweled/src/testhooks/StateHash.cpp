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
// Left out: images, fonts and sounds (only whether a banner image exists),
// mWhirlpoolSound, which depends on the audio device, and the LiveSplit observer's bookkeeping
// (mBoardSerial, mResumed), so FULL hashes stay equal to the frozen baselines. Nothing the hash reads is written by Draw (the
// banner's ColorizeText only writes image bits), so the draw schedule (-fast, -renderscale) does not
// change it; with script or -autoplay input the mouse fields are as deterministic as the clicks.
//
// LOGIC: the hand-written list below, the state later play can depend on; a narrower hash that a
// cosmetic change (glow, sparkle, vortex spiral) leaves alone. It stays the frozen baseline's list, since the oracle
// compares this column with the frozen baseline (build/baseline-stage0); the replays' CHECK and END take StateCodec's LOGIC
// hash (games/bejeweled/src/StateCodec.h), which is portable and leaves out the libm-set positions and
// mHoleRadius this one keeps. The board part of FULL is StateCodec's FULL encoding (port(variant):
// the same bytes the frozen baseline's field list hashed). Rule: a field is in the list if a later decision
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
//     mBackgroundChanged (dead), mOrphanedPieces (port bookkeeping; a stale selection is hashed below),
//     mBoardSerial and mResumed (the LiveSplit observer's bookkeeping; no game decision reads them).
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
// in the frozen baseline.
//
// Portability: no pointer, padding or map-order dependence in LOGIC (its only libm-derived values are
// swap positions, (int)-truncated). web/shell.html passes no test switches; the Node runner
// (games/bejeweled/tools/verify/wasmrun.js) runs the web build with them, and the oracle's ".wasm"
// cases compare its LOGIC column with the native game's.

#include "StateHash.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "StateCodec.h"
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

void HashPiece(StateHasher& theHasher, Board* theBoard, Piece* thePiece, bool full)
{
	if (full)
	{
		theHasher.Bytes(thePiece, Piece::SAVE_SIZE);  // the savegame.dat record (Piece.h)
		return;
	}

	theHasher.Int(thePiece->mColor);
	theHasher.Int(thePiece->mCol);
	theHasher.Int(thePiece->mRow);
	if (theBoard->mState != STATE_BLACK_HOLE)
	{
		theHasher.Double(thePiece->mX);
		theHasher.Double(thePiece->mY);
	}
	theHasher.Int(thePiece->mVelX);
	theHasher.Double(thePiece->mVelY);
	theHasher.Int(thePiece->mSwapOffX);
	theHasher.Int(thePiece->mSwapOffY);
	theHasher.Bool(thePiece->mDying);
	theHasher.Int(thePiece->mDyingFrame);
	theHasher.Double(thePiece->mHoleRadius);
}

// A piece pointer as its cell (row * cols + col), -1 for NULL, or rows * cols and the piece's fields for a
// piece that is not in the grid (a stale selection that NewBoard dropped, GAME_NOTES 3.6); 8x8: 64
void HashPieceRef(StateHasher& theHasher, Board* theBoard, Piece* thePiece, bool full)
{
	if (thePiece == NULL)
	{
		theHasher.Int(-1);
		return;
	}

	for (int aRow = 0; aRow < theBoard->mRows; aRow++)
	{
		for (int aCol = 0; aCol < theBoard->mCols; aCol++)
		{
			if (theBoard->mBoard[aRow][aCol] == thePiece)
			{
				theHasher.Int(aRow * theBoard->mCols + aCol);
				return;
			}
		}
	}

	theHasher.Int(theBoard->mRows * theBoard->mCols);
	HashPiece(theHasher, theBoard, thePiece, full);
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

// port(variant): a board's own RNG; nothing for a board on the global stream
void HashOwnRng(StateHasher& theHasher, Board* b)
{
	if (b->mOwnRng == NULL)
		return;
	theHasher.Int((int) b->mGameSeed);
	theHasher.Bool(b->mUserSeeded);
	theHasher.Bool(b->mTakesPendingSeed);
	theHasher.Int((int) b->mBoardDraws);
	uint32_t aState[MTRAND_N];
	int anIndex;
	b->mOwnRng->GetState(aState, &anIndex);
	theHasher.Bytes(aState, sizeof(aState));
	theHasher.Int(anIndex);
}

void HashBoardLogic(StateHasher& theHasher, Board* theBoard)
{
	theHasher.Int(theBoard->mUpdateCnt);  // board tick (Widget::mUpdateCnt)
	theHasher.Int(theBoard->mLevel);
	theHasher.Int(theBoard->mDisplayLevel);
	theHasher.Int(theBoard->mState);
	HashCells(theHasher, theBoard, false);

	theHasher.Bool(theBoard->mTimedMode);
	theHasher.Bool(theBoard->mIsNewGame);
	HashPieceRef(theHasher, theBoard, theBoard->mSelectedPiece, false);
	theHasher.Bool(theBoard->mShowTutorial);
	theHasher.Int(theBoard->mTutorialCol);
	theHasher.Int(theBoard->mTutorialRow);
	theHasher.Int(theBoard->mTutorialCol2);
	HashPieceRef(theHasher, theBoard, theBoard->mSwapPiece1, false);
	HashPieceRef(theHasher, theBoard, theBoard->mSwapPiece2, false);
	theHasher.Int(theBoard->mSwapAngle);
	theHasher.Int(theBoard->mSwapCenterX);
	theHasher.Int(theBoard->mSwapCenterY);
	theHasher.Int(theBoard->mBarWidth);
	theHasher.Int(theBoard->mPlayStartTick);
	theHasher.Int(theBoard->mPlaySeconds);
	theHasher.Int(theBoard->mGameDuration);
	theHasher.Int(theBoard->mIdleTicks);
	theHasher.Int(theBoard->mScore);
	theHasher.Double(theBoard->mPointMultiplier);
	theHasher.Int(theBoard->mDisplayScore);
	theHasher.Int(theBoard->mGemsCleared);
	theHasher.Int(theBoard->mMoveMatchCount);
	theHasher.Int(theBoard->mCascadeCount);
	theHasher.Int(theBoard->mGameOverCountdown);
	theHasher.Int(theBoard->mLongestCascade);
	theHasher.Int(theBoard->mBestMoveScore);
	theHasher.Int(theBoard->mClearedAtMoveStart);
	theHasher.Int(theBoard->mMoveScore);
	theHasher.Int(theBoard->mMoveScoreTimer);
	theHasher.Bool(theBoard->mNewGamePending);
	theHasher.Int(theBoard->mPauseCount);
	theHasher.Int(theBoard->mUserPauseCount);
	theHasher.Bool(theBoard->mStatsDisabled);
	theHasher.Int(theBoard->mSavedZero);
	theHasher.Int(theBoard->mLevelStartScore);
	theHasher.Int(theBoard->mLevelTargetScore);
	theHasher.Int(theBoard->mLevelPoints);
	theHasher.Double(theBoard->mTimeDrain);
	theHasher.Double(theBoard->mTimeDrainRate);
	theHasher.Double(theBoard->mTimeDrainAccel);
	theHasher.Int(theBoard->mBarColor);
	theHasher.Int(theBoard->mMouseDownX);
	theHasher.Int(theBoard->mMouseDownY);
	theHasher.Bool(theBoard->mMouseDown);
	theHasher.Int(theBoard->mInputDelay);
	theHasher.Int(theBoard->mWipeY);
	theHasher.Int(theBoard->mElectOffset);
	theHasher.Int(theBoard->mBlackHoleFrame);
	theHasher.Int(theBoard->mTimeDrainDelay);
	theHasher.Double(theBoard->mBlackHoleLevel);
	theHasher.Bool(theBoard->mLevelTextShown);
	HashOwnRng(theHasher, theBoard);
}

// Every Board field in declaration order (Board.h), pieces as raw bytes. port(variant): StateCodec's FULL
// encoding, the same bytes this function hashed field by field in the frozen baseline (the field table is the one list)
void HashBoardFull(StateHasher& h, Board* b)
{
	std::string aFull = StateCodec::Encode(b, StateCodec::PROFILE_FULL);
	h.Bytes(aFull.data(), aFull.size());
}

void HashApp(StateHasher& theHasher, WinDMApp* theApp)
{
	theHasher.Int((int) GetRandCount());
	theHasher.Bool(theApp->mTimedMode);
	theHasher.Bool(theApp->mShowInstructions);
	theHasher.Int(theApp->mTimesPlayed);
	theHasher.Bool(theApp->mHighScoreWidget != NULL);

	theHasher.Int((int) theApp->mDialogMap.size());
	for (DialogMap::iterator anItr = theApp->mDialogMap.begin(); anItr != theApp->mDialogMap.end(); ++anItr)
		theHasher.Int(anItr->first);
	if (gGameConfig.mPendingSeed != 0)
		theHasher.Int((int) gGameConfig.mPendingSeed);
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
