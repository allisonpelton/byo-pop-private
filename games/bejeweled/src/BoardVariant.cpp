// port(variant): Board members for code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.3). Not part of
// the original game. Stage 1 (9.2): the rules snapshot, the layout and the variant flags. The layout is the
// original 8x8 whatever the rules say, and no translated function reads the flags yet, so every board plays
// as the original; later stages switch the generalised paths on.
// Stage 2: the board's RNG (6.2). A seed the player chose, any rule, or the seeds.perGame feature gives
// the board its own MTRand, which Board::Rand draws from; every other board draws from the global stream
// as the original does. Stage 2b: the board serial and the resumed mark the LiveSplit observer reads (4.15).
// Stage 3: replays (6.3-6.5). The recorder and playback members, the RNG start a recording begins with,
// the sidecar next to the save (7.3) that keeps a recording and a board's own RNG across a resume, and the
// scratch board StateCodec's round trip restores into. Stage 3b: the playback board a replay player
// (WatchController) makes, with its own MTRand from the segment's RNG start, and the RNG its LOAD goes on with.
// Stage 5: allow invalid moves and the noMoves rule (4.1): a swap that made no match stays (KeepInvalidMove), a
// board that settles without a move does what its mode's noMoves rule says (OnNoMovesVariant), and the moves
// that stood are counted. A variant board saves the v2 save (BoardSave.cpp), whose RNGS chunk keeps its RNG, so
// it leaves no RNG sidecar.
// Stage 6: undo (4.12, 6.7): a stack of StateCodec UNDO snapshots taken before each move, and the undo that
// restores the newest one (TakeUndoSnapshot ... Undo).
// Stage 7: board sizes and shapes (4.3, section 5): the layout of the rules' level 0 (InitVariant); the shape code
// itself is in BoardShape.cpp and BoardDrawVariant.cpp. And a fix for a mod's scoring tunables: the displayed
// score closes a large gap fast (RollScoreFast).

#include "Board.h"
#include "GameConfig.h"
#include "WinDMApp.h"
#include "ReplayRecorder.h"
#include "StateCodec.h"
#include "MTRand.h"
#include "SoundManager.h"
#include "Common.h"

#include <SDL.h>
#include <algorithm>
#include <cstring>

using namespace Sexy;

// The boards made in this process, for the LiveSplit observer (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15)
static uint32_t sBoardSerial = 0;

// Called first in the constructor (0x4cb5bc), before SetupLevel and FillBoard use the layout and the RNG
void Board::InitVariant(WinDMApp* theApp)
{
	mRules = gGameConfig.mRules;
	// The LiveSplit observer's bookkeeping, in every build (BEJ_NO_VARIANTS too)
	mBoardSerial = ++sBoardSerial;
	mResumed = false;
	// The board shape rules (board.cols/rows/mask/levels, stage 7): level 0's shape, which SetupLevel then draws.
	// BEJ_NO_VARIANTS plays every rule set on the original 8x8
#ifndef BEJ_NO_VARIANTS
	mLayout = mRules.LayoutForLevel(0);
#else
	mLayout = BoardLayout();
#endif
	mPendingLayout = mLayout;
	mLevelShape = 0;
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;
	mFastScoreRoll = mRules.ScoringTunablesChanged();
	mMatchSpec = MatchFinder::Spec::FromRules(mRules);

#ifndef BEJ_NO_VARIANTS
	mVariant = ComputeVariantFlags(mRules, gGameConfig.mFeatures);
	if (gGameConfig.mForceVariant)
	{
		// -forcevariant: every generalised path at default rules; the RNG source, the save format and the
		// tutorial stay original (mAny, mOwnRng and mNoTutorial are left alone). The noMoves rule's path too
		// (stage 5): its default "gameOver" and "fallOut" are the original branches. Not mInvalid: keeping a
		// swap is no default rule. The gravity paths too (stage 7c): the rules still say "down", so GravityUp()
		// is false and the flip and the GetPieceAt guard stay off. The dirt paths too (stage 7b): without dirt
		// cells RefillDiagonal is RefillMasked's passes, draw for draw, and no slide or layer applies.
		mVariant.mShape = true;
		mVariant.mMatch = true;
		mVariant.mSearch = true;
		mVariant.mDraw = true;
		mVariant.mNoMoves = true;
		mVariant.mGravity = true;
		mVariant.mFall = true;
		mVariant.mChecker = true;
		mVariant.mDirt = true;
	}
	// look.soundPitch (4.16): every sound the app plays while this board is the game's; the next board sets its own
	if (theApp->mSoundManager != NULL)
		theApp->mSoundManager->SetDefaultPitch(mRules.mLook.mSoundPitch);
#endif
	mMoveCount = 0;
	mInvalidMoves = 0;
	mUndosUsed = 0;
	mGravityFlips = 0;
	mGravityNoFlip = false;
#ifndef BEJ_NO_VARIANTS
	mDirtRules = mRules.HasDirtRule();
#else
	mDirtRules = false;
#endif
	mDiagonalLeftFirst = true;
	mDirtTiles[0] = NULL;
	mDirtTiles[1] = NULL;
	mDirtTilesLevel = -1;

	// The RNG source WinDMApp::NewGame chose (WinDMApp::ChooseBoardRng), taken once: a board made any other
	// way draws from the global stream
	mOwnRng = NULL;
	mGameSeed = gGameConfig.mNextBoardSeed;
	mUserSeeded = (mGameSeed != 0) && gGameConfig.mNextBoardUserSeeded;
	mRanked = false;
	mBoardDraws = 0;
	mTakesPendingSeed = mUserSeeded && (mGameSeed == gGameConfig.mPendingSeed);
	gGameConfig.mNextBoardSeed = 0;
	gGameConfig.mNextBoardUserSeeded = false;
	if (mGameSeed != 0)
		mOwnRng = new MTRand(mGameSeed);

	// A replay's board (WatchController, stage 3b): it always owns an MTRand, started from its segment's RNG
	// start (6.2): the seed, or the global stream's state the recording began with. It never draws from the
	// global stream, and never takes the pending seed. The player has checked the start (MTRand::SetState).
	const Replay::Segment* aPlayback = gGameConfig.mNextBoardPlayback;
	gGameConfig.mNextBoardPlayback = NULL;
	mPlaybackOnly = (aPlayback != NULL);
	if (aPlayback != NULL)
	{
		bool isSeed = (aPlayback->mRng.mKind == Replay::RNG_SEED);
		mGameSeed = isSeed ? aPlayback->mRng.mSeed : 0;
		mUserSeeded = isSeed && aPlayback->mUserSeeded;
		mTakesPendingSeed = false;
		delete mOwnRng;
		mOwnRng = new MTRand(isSeed ? mGameSeed : 4357);
		if (!isSeed)
			mOwnRng->SetState(aPlayback->mRng.mMt, aPlayback->mRng.mMti);
	}
#ifndef BEJ_NO_VARIANTS
	// ComputeVariantFlags says whether the rules need an own RNG; NewGame drew its seed (or took the
	// player's), and -forcevariant leaves the source alone
	mVariant.mOwnRng = (mOwnRng != NULL);
#endif

	// Replays: WinDMApp::NewGame attaches a recorder after the constructor; the stream it records from starts
	// here, before the constructor's first draw (6.2: a seed, or the global stream's state)
	mRecorder = NULL;
	mPlaybackApplying = false;
	mPlaybackLoad = NULL;
	mRngStart = Replay::RngStart();
	if (gGameConfig.mFeatures.mReplays && !mPlaybackOnly)
	{
		mRngStart.mKind = (mOwnRng != NULL) ? Replay::RNG_SEED : Replay::RNG_STATE;
		mRngStart.mSeed = mGameSeed;
		if (mOwnRng == NULL)
			GetRandMTState(mRngStart.mMt, &mRngStart.mMti);
	}
}

// A target for StateCodec's round trip (6.6): a board with nothing drawn, loaded, saved or shown, whose
// every field the snapshot then overwrites. It never takes input or updates (mPlaybackOnly, and no widget
// manager knows it), and it is never deleted (~Board would close the live board's instructions dialog):
// the test hook that makes it keeps one for the whole run.
Board::Board(WinDMApp* theApp, ScratchBoardTag)
{
	memset(mBoard, 0, sizeof(mBoard));
	mApp = theApp;
	mLevel = 0;
	mDisplayLevel = 1;
	mState = STATE_FALLING;
	mTimedMode = false;
	mWhirlpoolSound = NULL;
	mIsNewGame = true;
	mSelectedPiece = NULL;
	mCursorFrame = 0;
	mShowTutorial = false;
	mTutorialBob = 0;
	mTutorialBobDir = 1;
	mTutorialCol = -1;
	mTutorialRow = -1;
	mTutorialCol2 = -1;
	mSwapPiece1 = NULL;
	mSwapPiece2 = NULL;
	mSwapAngle = 0;
	mSwapCenterX = 0;
	mSwapCenterY = 0;
	mBarWidth = 0;
	mPlayStartTick = 0;
	mPlaySeconds = 0;
	mGameDuration = 0;
	mIdleTicks = 0;
	mScore = 0;
	mPointMultiplier = 0.0;
	mDisplayScore = 0;
	mGemsCleared = 0;
	mMoveMatchCount = 0;
	mCascadeCount = 0;
	mLastLandSoundTick = 0;
	mGameOverCountdown = 0;
	mLongestCascade = 0;
	mBestMoveScore = 0;
	mUnused218 = 0;
	mMoveScore = 0;
	mMoveScoreTimer = 0;
	mMoveScoreAlpha = 0.0;
	mHintGlowAlpha = 0.0;
	mHintGlowTimer = 0;
	mHintGlowDuration = 0;
	mNewGamePending = false;
	mPauseCount = 0;
	mUserPauseCount = 0;
	mPointsFont = theApp->mAlphaDigitsFont;
	mScoreFont = theApp->mScoreFont;
	mStatsDisabled = false;
	mUnused264 = 0;
	mLevelStartScore = 0;
	mLevelTargetScore = 0;
	mLevelPoints = 0;
	mTimeDrain = 0.0;
	mTimeDrainRate = 0.0;
	mTimeDrainAccel = 0.0;
	mBarColor = BAR_COLOR_NORMAL;
	mLastWarningTick = 0;
	mMouseX = -1;
	mMouseY = -1;
	mMouseDownX = 0;
	mMouseDownY = 0;
	mMouseDown = false;
	mInputDelay = 0;
	mBackgroundImage = NULL;
	mNextBackgroundImage = NULL;
	mBackgroundChanged = false;
	mTextImage = NULL;
	mTextColorImage = NULL;
	mShowingText = false;
	mTextPhase = 0.0;
	mTextDuration = 0;
	mTextTimeLeft = 0;
	mWipeY = 0;
	mElectOffset = 0;
	mBlackHoleFrame = 0;
	mTimeDrainDelay = 0;
	mBlackHoleLevel = 0.0;
	mLevelTextShown = false;

	mRules = gGameConfig.mRules;
	mLayout = BoardLayout();
	mPendingLayout = mLayout;
	mLevelShape = 0;
	mFastScoreRoll = false;
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;
	mOwnRng = NULL;
	mGameSeed = 0;
	mUserSeeded = false;
	mRanked = false;
	mBoardDraws = 0;
	mTakesPendingSeed = false;
	mRecorder = NULL;
	mPlaybackOnly = true;
	mPlaybackApplying = false;
	mPlaybackLoad = NULL;
	// No serial (it is never the app's mBoard, so the LiveSplit observer never sees it) and no row in the
	// snapshot that overwrites the rest
	mBoardSerial = 0;
	mResumed = false;
	mMoveCount = 0;
	mInvalidMoves = 0;
	mUndosUsed = 0;
}

// A seed the player chose stays pending until a game on it is played
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10, the user's decision of 2026-10-18): the board that got it
// uses it up with its first swap that makes a match (UpdateSwap, 0x4cd84f, where mIsNewGame is cleared) or
// its game over (GameOver, 0x4c9b47; not the one RequestNewGame starts, which clears mTakesPendingSeed
// first). Until then every board that replaces it gets the seed again: the second of the double NewGame in
// TitleScreenIsFinished, a mode switch, New Game. A board that resumed a save (GameLoadedVariant) never
// uses it up.
void Board::UsePendingSeed()
{
	if (!mTakesPendingSeed)
		return;
	mTakesPendingSeed = false;
	if (gGameConfig.mPendingSeed == mGameSeed)
		gGameConfig.mPendingSeed = 0;
}

// A move stood, where mIsNewGame is cleared (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10): a swap that made a
// match (UpdateSwap, 0x4cd84f), or a kept invalid move that counts as a move (KeepInvalidMove). The game has
// started, so it uses up a seed the player chose, and the move counts (the v2 save's MOVE chunk).
void Board::OnMoveMade()
{
	mMoveCount++;
	UsePendingSeed();
}

// UpdateSwap (0x4cd84f) with moves.allowInvalid: the swap made no match and stays (4.1). The two gems are in
// their new cells already. Instead of the swap back: the swap pieces go, moves.invalidPenalty is taken as
// DoHint takes the hint penalty (times the multiplier, the score floored at 0; in timed mode the bar shows it,
// so it costs time there), bad2 plays as for a swap back, and with moves.invalidCountsAsMove the game has
// started (mIsNewGame cleared: a quit saves it, a mode switch asks first) like after a match: the move counts and
// a seed the player chose is used up. The swap took its 36 ticks like any swap (the timed drain runs in them, as
// for a swap that goes back). Then STATE_FALLING: on the next tick UpdateFalling finds every gem at rest, no
// match, and decides as after any move, FindMove: STATE_PLAYING, or the board has no move left and the noMoves
// rule decides (a kept swap can take the last move away). A move without a run has no cascade, so that settle
// counts none (mLongestCascade). Replays record the move as any swap (SELECT, TARGET): no new command.
// The first-game tutorial: a swap that goes back leaves the board as it was, so its arrows stay right; a kept one
// changes the board, and the move the arrows show may make no match any more. Then the arrows and the instructions
// go, as at a game over (GameOver, 0x4c9b47): mShowInstructions stays, so the next game shows them again (the
// player has made no match yet). A tutorial move that still matches keeps them up until a match, as before.
void Board::KeepInvalidMove()
{
	mSwapPiece1 = NULL;
	mSwapPiece2 = NULL;
	mCascadeCount = 0;
	if (mShowTutorial && (mTutorialCol >= 0))
	{
		Piece*& aLeft = mBoard[mTutorialRow][mTutorialCol];
		Piece*& aRight = mBoard[mTutorialRow][mTutorialCol2];
		std::swap(aLeft, aRight);
		bool isStillMove = HasMatch();
		std::swap(aLeft, aRight);
		if (!isStillMove)
		{
			mShowTutorial = false;
			mApp->KillDialog(DIALOG_INSTRUCTIONS);
		}
	}
	if (mRules.mMoves.mInvalidPenalty != 0)
	{
		mScore = (int)((double)mScore - mPointMultiplier * mRules.mMoves.mInvalidPenalty);
		if (mScore < 0)
			mScore = 0;
	}
	mInvalidMoves++;
	if (mRules.mMoves.mInvalidCountsAsMove)
	{
		mIsNewGame = false;
		OnMoveMade();
	}
	mApp->PlaySample(SOUND_BAD);
	mState = STATE_FALLING;
	mGravityNoFlip = true; // its settle does not flip "alternate" (4.16)
}

// UpdateFalling (0x4cd687): the board settled with no match and no move left (4.1). The mode's noMoves rule:
// "gameOver" and "fallOut" are the original branches (normal: NO MOVES and the game over; timed: the fall-out
// and a new board), "continue" (which needs moves.allowInvalid) shows the banner NO_MATCHES_LEFT (default "NO
// MATCHES!", short enough for the board's width, like "NO MOVES!") and goes on: the player swaps on, every swap
// kept, until one makes a move again. A board with gold.enabled defaults to "fallOut" in
// normal mode too (GameRules).
void Board::OnNoMovesVariant()
{
	switch (mRules.NoMovesFor(mTimedMode))
	{
	case GameRules::NOMOVES_CONTINUE:
		ShowText(mApp->GetString("NO_MATCHES_LEFT", _S("NO MATCHES!")), 400);
		mState = STATE_PLAYING;
		break;
	case GameRules::NOMOVES_FALL_OUT:
		mState = STATE_FALL_OUT;
		break;
	default:
		ShowText(mApp->GetString("NO_MOVES"), 400);
		GameOver(200);
		break;
	}
}

// A board that plays without a move: only a noMoves "continue" board (OnNoMovesVariant), which needs
// moves.allowInvalid; under the original rules STATE_PLAYING always has a move (UpdateFalling). There Hint has
// nothing to show: ShowHint (0x4cb475) gives none, so DoHint takes no penalty for it, and the idle glow
// (Update, 0x4cdabc) does not invite one. A replay's HINT on such a board is one ShowHint would not give
// (WatchController). False at once on every other board, and in any other state (FindMove needs the full,
// settled board of STATE_PLAYING).
bool Board::NoHintVariant()
{
	return mVariant.mInvalid && (mState == STATE_PLAYING) && !FindMove(NULL, 0, true, true);
}

// The quick instructions (WinDMApp::ShowInstructionsDialog, 2.6): INSTRUCTIONDIALOG, or with a gravity rule
// (4.16) INSTRUCTIONDIALOG_TWILIGHT ("alternate") or _GRAVITY_UP. The engine's English defaults; a mod's strings
// override them.
SexyString Board::InstructionsText()
{
	int aDirection = mVariant.mGravity ? mRules.mBoard.mGravity : GameRules::GRAVITY_DOWN;
	if (aDirection == GameRules::GRAVITY_ALTERNATE)
		return mApp->GetString("INSTRUCTIONDIALOG_TWILIGHT", _S("Line up 3 or more gems by swapping neighbours. Gravity turns over after every move!"));
	if (aDirection == GameRules::GRAVITY_UP)
		return mApp->GetString("INSTRUCTIONDIALOG_GRAVITY_UP", _S("Line up 3 or more gems by swapping neighbours. New gems rise from below!"));
	return mApp->GetString("INSTRUCTIONDIALOG");
}

// Help (WinDMApp::ShowFullInstructionsDialog) with a gravity rule (4.16): the first paragraph of theText (the
// game's FULL_INSTRUCTIONS2, whose first paragraph has new gems drop in from above) gives way to
// FULL_INSTRUCTIONS_TWILIGHT or _GRAVITY_UP, of about its length, so the dialog keeps its size; the other
// paragraphs stay. English defaults; a mod's strings override them.
SexyString Board::FullInstructionsText(const SexyString& theText)
{
	int aDirection = mVariant.mGravity ? mRules.mBoard.mGravity : GameRules::GRAVITY_DOWN;
	SexyString aFirst;
	if (aDirection == GameRules::GRAVITY_ALTERNATE)
		aFirst = mApp->GetString("FULL_INSTRUCTIONS_TWILIGHT", _S("TWILIGHT - Swap neighbouring gems to line up 3 or more of a kind: ")
			_S("they vanish and score. Gravity turns over after every move, so one move's new gems fall in from above ")
			_S("and the next move's rise from below. Fill the Bonus Bar to reach the next level, where matches score more."));
	else if (aDirection == GameRules::GRAVITY_UP)
		aFirst = mApp->GetString("FULL_INSTRUCTIONS_GRAVITY_UP", _S("GRAVITY UP - Swap neighbouring gems to line up 3 or more of a ")
			_S("kind: they vanish and score, and new gems rise in from below. Fill the Bonus Bar to reach the next level, ")
			_S("where matches score more."));
	else
		return theText;
	size_t aBreak = theText.find(_S("\n\n"));
	return (aBreak == SexyString::npos) ? aFirst : aFirst + theText.substr(aBreak);
}

// The end of a LoadGame (0x4cca98) that succeeded. The resumed game is not the one the board's seed would
// make: it goes on with the RNG it was saved with. The v1 save holds none, so the sidecar does
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.3; stage 5's v2 save keeps it in its RNGS chunk): a sidecar
// that goes with this save and holds an RNGS record gives the board that MTRand state with its seed, so a
// seeded game keeps its stream, its SEED line and (a seed the player chose) the -seeded table, and a quit
// and resume cannot reroll it. Without one (a game on the global stream, or a save from before stage 3)
// the game goes on on the global stream. Either way the pending seed stays for the next game. A recording
// then records LOAD and goes on with the sidecar's replay. Stage 2b: the board is marked resumed for the
// LiveSplit observer (no run starts on it), a replay's board too. Stage 5: a variant board's v2 save holds its
// RNG and its MOVE counts (ReadVariantChunks); its sidecar, if any, only continues the recording.
void Board::GameLoadedVariant(const Buffer& theSave)
{
	// The LiveSplit observer counts no GO on this board (in every build, BEJ_NO_VARIANTS too)
	mResumed = true;
	mTakesPendingSeed = false;
	mUndoStack.clear(); // a save holds no undo snapshots (4.12)
	if (mVariant.mAny)
		ReadVariantChunks(theSave);
	// Stage 7: the loaded level's background, in its shape (BoardShape.cpp); not under -forcevariant, which keeps
	// the original's level-0 background
	if (mVariant.mAny && mVariant.mShape)
		RebuildLoadedBackground();
	// A replay's LOAD (stage 3b): no file of the user dir is read; the player gives the board the RNG the
	// recorded one went on with, the segment's RNGS (SetPlaybackRng)
	if (mPlaybackOnly)
		return;
	std::string aSave((const char*) theSave.GetDataPtr(), (size_t) theSave.GetDataLen());
	Replay::File aSidecar;
	bool hasSidecar = ReadSaveSidecar(aSave, &aSidecar);

	if (!mVariant.mAny)
	{
		MTRand* aRng = NULL;
		const Replay::RngState& aState = aSidecar.mTrailingRng;
		if (hasSidecar && aSidecar.mHasTrailingRng && (aState.mSeed != 0))
		{
			aRng = new MTRand(aState.mSeed);
			if (!aRng->SetState(aState.mMt, aState.mMti))
			{
				delete aRng;
				aRng = NULL;
			}
		}
		delete mOwnRng;
		mOwnRng = aRng;
		mGameSeed = (aRng != NULL) ? aState.mSeed : 0;
		mUserSeeded = (aRng != NULL) && aState.mUserSeeded;
		mRanked = (aRng != NULL) && aState.mRanked;
#ifndef BEJ_NO_VARIANTS
		mVariant.mOwnRng = (mOwnRng != NULL);
#endif
	}

	if (mRecorder != NULL)
		mRecorder->OnLoad(aSave, hasSidecar ? &aSidecar : NULL);
}

// LoadGame (0x4cca98) reads the save from the file, or a replay's board the bytes the replay's last SAVE
// produced (playback LOAD, 6.5; mPlaybackLoad, stage 3b)
bool Board::ReadSaveFile(Buffer* theBuffer)
{
	if (mPlaybackLoad == NULL)
		return mApp->ReadBufferFromFile(GetSaveGameFileName(), theBuffer, false);
	*theBuffer = *mPlaybackLoad;
	theBuffer->SeekFront();
	return true;
}

// SaveGame (0x4ccdfe) writes the save: a replay's board keeps the bytes instead (playback SAVE), any other
// first records SAVE and writes the sidecar next to the save (WriteSaveSidecar)
bool Board::WriteSaveFile(const Buffer& theBuffer)
{
	if (mPlaybackOnly)
	{
		mPlaybackSave = theBuffer;
		return true;
	}
	WriteSaveSidecar(theBuffer);
	return mApp->WriteBufferToFile(GetSaveGameFileName(), &theBuffer);
}

// "userdata\savegame.bjr" next to "userdata\savegame.dat", scoped like it (Board::GetSaveGameFileName)
std::string Board::GetSidecarFileName() const
{
	std::string aName = GetSaveGameFileName();
	size_t aDot = aName.find_last_of('.');
	return ((aDot != std::string::npos) ? aName.substr(0, aDot) : aName) + ".bjr";
}

// The sidecar, if there is one and it goes with theSave (its SVHA is theSave's hash)
bool Board::ReadSaveSidecar(const std::string& theSave, Replay::File* theSidecar)
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(GetSidecarFileName(), &aBuffer, false))
		return false;
	std::string aBytes((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
	return Replay::Read(aBytes, theSidecar) && theSidecar->mHasSaveHash &&
		(theSidecar->mSaveHash == Replay::SaveHash(theSave.data(), theSave.size()));
}

void Board::GetRngState(Replay::RngState* theRng)
{
	*theRng = Replay::RngState();
	if (mOwnRng != NULL)
	{
		theRng->mSeed = mGameSeed;
		theRng->mUserSeeded = mUserSeeded;
		theRng->mRanked = mRanked;
		mOwnRng->GetState(theRng->mMt, &theRng->mMti);
	}
	else
		GetRandMTState(theRng->mMt, &theRng->mMti);
}

// A playback LOAD (WatchController, stage 3b), right after LoadGame: the board goes on with the RNG of the
// segment's RNGS record, always as an own MTRand (a seed of 0 is the global stream's state at the recorded
// LOAD), with the seed, userSeeded and ranked the LOGIC hash takes. False, the board unchanged, for a state
// MTRand refuses.
bool Board::SetPlaybackRng(const Replay::RngState& theRng)
{
	MTRand* aRng = new MTRand(4357);
	if (!aRng->SetState(theRng.mMt, theRng.mMti))
	{
		delete aRng;
		return false;
	}
	delete mOwnRng;
	mOwnRng = aRng;
	mGameSeed = theRng.mSeed;
	mUserSeeded = (theRng.mSeed != 0) && theRng.mUserSeeded;
	mRanked = (theRng.mSeed != 0) && theRng.mRanked;
#ifndef BEJ_NO_VARIANTS
	mVariant.mOwnRng = true;
#endif
	return true;
}

Replay::EndInfo Board::GetEndInfo(int theReason, const MTRand* theRng)
{
	Replay::EndInfo anEnd;
	anEnd.mReason = theReason;
	anEnd.mEndTick = (uint32_t) mUpdateCnt;
	anEnd.mScore = mScore;
	anEnd.mLevel = mDisplayLevel;
	anEnd.mGems = mGemsCleared;
	anEnd.mLogicHash = StateCodec::LogicHash(this, theRng);
	anEnd.mBoardDraws = mBoardDraws;
	return anEnd;
}

// The sidecar of a save that is not recorded: the board's own RNG, for the resume to restore
static std::string RngSidecar(Board* theBoard, const std::string& theSave)
{
	Replay::File aFile;
	aFile.mHasTrailingRng = true;
	theBoard->GetRngState(&aFile.mTrailingRng);
	aFile.mHasSaveHash = true;
	aFile.mSaveHash = Replay::SaveHash(theSave.data(), theSave.size());
	aFile.mEnd = theBoard->GetEndInfo(Replay::END_SAVED);
	return Replay::Write(aFile);
}

// The sidecar for the web backup (WinDMApp::GetShutdownSaves). A recording too long for the backup's one
// localStorage item (ReplayRecorder::FitsBackup) leaves only the RNG there, as a game that is not recorded
// does: a resume from that backup goes on with the right RNG, as a fresh replay that holds the save.
std::string Board::SidecarSnapshot(const std::string& theSave)
{
	if ((mRecorder != NULL) && mRecorder->FitsBackup())
		return mRecorder->SnapshotSidecar(theSave);
	if ((mOwnRng != NULL) && !mVariant.mAny)
		return RngSidecar(this, theSave);
	return "";
}

// SaveGame (0x4ccdfe), just before the save is written: a recording records SAVE and leaves the game so far
// next to the save; a board with its own RNG leaves that RNG, unless it is a variant board, whose v2 save holds
// it (stage 5). Any other save has no sidecar, so an old one (which could not match it anyway) goes. A game on
// the global stream without replays never had one: the erase finds nothing and changes nothing. The first save
// of a recording board replaces another board's save (a mode switch saved one game, the next is saved too):
// that game's recording is kept first.
void Board::WriteSaveSidecar(const Buffer& theSave)
{
	std::string aSave((const char*) theSave.GetDataPtr(), (size_t) theSave.GetDataLen());
	std::string aSidecar;
	if (mRecorder != NULL)
	{
		if (mRecorder->FirstSidecar())
			KeepSidecarGame();
		aSidecar = mRecorder->EndSave(aSave);
	}
	else if ((mOwnRng != NULL) && !mVariant.mAny)
		aSidecar = RngSidecar(this, aSave);
	if (aSidecar.empty())
	{
		mApp->EraseFile(GetSidecarFileName());
		return;
	}
	if (!mApp->WriteBytesToFile(GetSidecarFileName(), aSidecar.data(), (unsigned long) aSidecar.size()))
		SDL_Log("replays: cannot write %s", GetSidecarFileName().c_str());
}

// The recorded game of the sidecar there is, as a finished replay, when it goes with the save there is
// (ReplayRecorder::KeepSidecarGame): its save is about to go
void Board::KeepSidecarGame()
{
	Buffer aSidecar;
	Buffer aSave;
	if (mApp->ReadBufferFromFile(GetSidecarFileName(), &aSidecar, false) && mApp->ReadBufferFromFile(GetSaveGameFileName(), &aSave, false))
		mRecorder->KeepSidecarGame(std::string((const char*) aSidecar.GetDataPtr(), (size_t) aSidecar.GetDataLen()),
			std::string((const char*) aSave.GetDataPtr(), (size_t) aSave.GetDataLen()));
}

// DeleteSaveGame (0x4cd130): the sidecar goes with the save. When the player declined the resume, a
// recorded game in it ended by being saved: it is kept as a finished replay first. So is one the resume did not
// go on with (ReplayRecorder::OnLoad: other rules, or an engine whose replays of these rules this one cannot
// continue): the new replay holds only the loaded save (LDSV), and the moves before it are not lost.
void Board::DeleteSaveSidecar()
{
	if ((mRecorder != NULL) && (!mRecorder->Loaded() || !mRecorder->ContinuedSidecar()))
		KeepSidecarGame();
	mApp->EraseFile(GetSidecarFileName());
}

// Undo (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.12, 6.7; stage 6). A board whose rules enable undo keeps
// StateCodec's UNDO encoding of itself from before each move, newest last, at most undo.depth of them: StartSwap
// (0x4c9ff8) takes one after its adjacency check, in STATE_PLAYING, where every gem is at rest; a swap that goes
// back takes its own back (UpdateSwap, 0x4cd84f), a kept invalid move keeps it; the vortex empties the stack
// (StartBlackHole, 0x4ca58d), so an undo never crosses a level; a save holds none, so a resumed game starts
// without (GameLoadedVariant).
void Board::TakeUndoSnapshot()
{
	if (mState != STATE_PLAYING)
		return;
	bool isAtRest = true;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mDying || (aPiece->mVelY != 0.0) || (aPiece->mX != GetColX(aCol)) || (aPiece->mY != GetRowY(aRow))))
				isAtRest = false;
		}
	}
	// Cannot happen in STATE_PLAYING; the oracle's variant check fails on this line
	if (!isAtRest)
		SDL_Log("undo: snapshot of a board not at rest (board tick %d)", mUpdateCnt);
	mUndoStack.push_back(StateCodec::Encode(this, StateCodec::PROFILE_UNDO));
	if ((int) mUndoStack.size() > mRules.mUndo.mDepth)
		mUndoStack.erase(mUndoStack.begin());
}

void Board::PopUndoSnapshot()
{
	if (!mUndoStack.empty())
		mUndoStack.pop_back();
}

// In play, unpaused, a move to take back, undo.max not reached, and in timed mode only with undo.inTimed
bool Board::CanUndo() const
{
	if (!mVariant.mUndo || (mState != STATE_PLAYING) || (mPauseCount != 0) || mUndoStack.empty())
		return false;
	if (mTimedMode && !mRules.mUndo.mInTimed)
		return false;
	return UndosLeft() != 0;
}

int Board::UndosLeft() const
{
	if (mRules.mUndo.mMax < 0)
		return -1;
	return std::max(0, mRules.mUndo.mMax - mUndosUsed);
}

// KeyChar (0x4ca9d3): z, u (either case) and Backspace undo instead of pausing, on a board whose rules have undo in
// its mode; on every other board they pause as any key does
bool Board::IsUndoKey(SexyChar theChar)
{
	return (theChar == 'z') || (theChar == 'Z') || (theChar == 'u') || (theChar == 'U') || (theChar == '\b');
}

bool Board::UndoKeysActive() const
{
	return mVariant.mUndo && (!mTimedMode || mRules.mUndo.mInTimed);
}

// The keys and the red orb (WinDMApp::ButtonDepress, BUTTON_UNDO). A refused undo plays bad2, as a swap of gems that
// are no neighbours does. Recorded as UNDO, which playback applies with Undo after the same gate (WatchController)
bool Board::RequestUndo()
{
	if (mPlaybackOnly)
		return false;
	if (!CanUndo())
	{
		mApp->PlaySample(SOUND_BAD);
		return false;
	}
	if (mRecorder != NULL)
		mRecorder->OnUndo();
	Undo();
	return true;
}

// The newest snapshot back: the grid (fresh gems at rest), the score and the stats, the counts of moves and kept
// invalid moves, the time only with undo.restoreTime, and the RNG unless undo.restoreRng is false (with it the same
// move again refills the same way: an undo is no reroll). Kept: the board tick, the pauses, the play time, the undos
// taken and whether the board still holds the seed the player chose (a match used it up). Then undo.penalty times
// the multiplier comes off the score, as DoHint takes the hint penalty; the selection, the popups, the move score
// and the hint glow go; the banner says UNDO.
void Board::Undo()
{
	std::string aSnapshot = mUndoStack.back();
	mUndoStack.pop_back();
	double aTimeDrain = mTimeDrain;
	double aTimeDrainRate = mTimeDrainRate;
	int aTimeDrainDelay = mTimeDrainDelay;
	bool takesPendingSeed = mTakesPendingSeed;
	uint32_t aBoardDraws = mBoardDraws;
	uint32_t aMt[MTRAND_N];
	int anMti = 0;
	bool hadOwnRng = (mOwnRng != NULL);
	if (hadOwnRng)
		mOwnRng->GetState(aMt, &anMti);

	// The restore replaces every gem: nothing may point at the old ones
	mSelectedPiece = NULL;
	mSwapPiece1 = NULL;
	mSwapPiece2 = NULL;
	std::string anError;
	if (!StateCodec::Decode(this, aSnapshot, &anError, StateCodec::PROFILE_UNDO))
		SDL_Log("undo: snapshot not restored: %s", anError.c_str()); // the board's own bytes: cannot happen

	if (!mRules.mUndo.mRestoreTime)
	{
		mTimeDrain = aTimeDrain;
		mTimeDrainRate = aTimeDrainRate;
		mTimeDrainDelay = aTimeDrainDelay;
	}
	if (!mRules.mUndo.mRestoreRng)
	{
		if (hadOwnRng && (mOwnRng != NULL))
			mOwnRng->SetState(aMt, anMti);
		mBoardDraws = aBoardDraws;
	}
	mTakesPendingSeed = takesPendingSeed;

	mScorePopups.clear();
	mMoveScoreTimer = 0;
	mMoveScoreAlpha = 0.0;
	mHintGlowTimer = 0;
	mHintGlowAlpha = 0.0;
	mIdleTicks = 0;
	if (mRules.mUndo.mPenalty != 0)
	{
		mScore = (int)((double)mScore - mPointMultiplier * mRules.mUndo.mPenalty);
		if (mScore < 0)
			mScore = 0;
	}
	mUndosUsed++;
	ShowText(mApp->GetString("UNDO", _S("UNDO")), 150); // 75 ticks in, 75 out (ColorizeText)
	MarkDirty();
}

VariantFlags Board::ComputeVariantFlags(const GameRules& theRules, const GameFeatures& theFeatures)
{
	VariantFlags aFlags;
	const GameRules::MatchRules& aMatch = theRules.mMatch;
	bool hasPatterns = !aMatch.mShapes.empty() || aMatch.mDiagonal || aMatch.mMergeCrossings;
	bool hasSpecials = theRules.mSpecials.mFlame.mEnabled || theRules.mSpecials.mLightning.mEnabled;
	bool hasNonSwapMoves = theRules.mMoves.mTypes != GameRules::MOVE_SWAP;

	aFlags.mAny = theRules.HasVariantRules();
	// The rules and features that give a board its own RNG even without a seed the player chose
	// (WinDMApp::ChooseBoardRng)
	aFlags.mOwnRng = aFlags.mAny || theFeatures.mSeedPerGame || theFeatures.mLeaderboard;

	BoardLayout aFirst = theRules.LayoutForLevel(0);
	aFlags.mShape = (theRules.ShapeCount() > 1) || !aFirst.IsOriginal();
	for (int aLevel = 0; aLevel < theRules.ShapeCount(); aLevel++)
		aFlags.mScaled = aFlags.mScaled || (theRules.LayoutForLevel(aLevel).mExtent > BoardLayout::VIEW_SIZE);

	// The tutorial looks for a horizontal 3-run swap to the right in rows 4..6, columns 1..6, and its text is about
	// rows of three: it is off on a board without such cells, with runs of another length or with diagonals (stage 9).
	// The board swaps whatever moves.type says (AddBuildWarnings), so that joins this test in the stage that plays it
	// (10), not before; a stage 1 mod set with twists kept the tutorial (oracle case rules_inert)
	bool canTutorial = (aFirst.mCols >= 8) && (aFirst.mRows >= 7);
	for (int aRow = 4; (aRow <= 6) && canTutorial; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
			canTutorial = canTutorial && aFirst.IsPlayable(aCol, aRow);
	}
	aFlags.mNoTutorial = !canTutorial || (aMatch.mMinRun != 3) || aMatch.mDiagonal;

	aFlags.mSpecials = hasSpecials;
	aFlags.mGold = theRules.mGold.mEnabled;
	aFlags.mUndo = theRules.mUndo.mEnabled;
	aFlags.mMatch = hasPatterns || (aMatch.mMinRun != 3) || hasSpecials || aFlags.mGold;
	aFlags.mSearch = hasPatterns || (aMatch.mMinRun != 3) || hasNonSwapMoves;
	aFlags.mMoves = (theRules.mMoves.mTypes & (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE)) != 0;
	aFlags.mInvalid = theRules.mMoves.mAllowInvalid;
	aFlags.mNoMoves = (theRules.mNoMoves.mNormal != GameRules::NOMOVES_GAME_OVER) || (theRules.mNoMoves.mTimed != GameRules::NOMOVES_FALL_OUT);
	aFlags.mDraw = aFlags.mShape || hasSpecials || aFlags.mGold || hasNonSwapMoves || aFlags.mNoTutorial;
	// Gravity (4.16): look.backdrop is no variant rule (mAny), but its tiles take the checker path
	aFlags.mGravity = theRules.mBoard.mGravity != GameRules::GRAVITY_DOWN;
	aFlags.mFall = aFlags.mShape || aFlags.mGravity || (theRules.mBoard.mFallAccel != GameRules::DEFAULT_FALL_ACCEL);
	aFlags.mChecker = aFlags.mShape || (theRules.mLook.mBackdrop == GameRules::BACKDROP_TWILIGHT);
	// Dirt (4.14): a mask with dirt is a shape, so mShape, mFall and mDraw are set already
	aFlags.mDirt = theRules.HasDirtRule();
	return aFlags;
}

// Update (0x4cdabc), before the score roll, on a board with a mod's scoring tunables (mFastScoreRoll). The roll
// moves the displayed score 3 times the multiplier a tick at most, which suits the original's points; with a
// mod's thousands a match it falls levels behind, and Update's level-up and TIME_UP tests read the displayed
// score: the next level, which starts at the real score, ends at once in TIME_UP (or a hint penalty of
// thousands gives free levels). A gap past the original's fastest band (150 times the multiplier) closes by an
// eighth a tick down to that band, which the original roll then closes as before.
void Board::RollScoreFast()
{
	int64_t aGap = (int64_t) mScore - mDisplayScore;
	int64_t aBand = (int) (mPointMultiplier * 150.0);
	if ((aGap > aBand) || (aGap < -aBand))
	{
		mDisplayScore = (int) (mDisplayScore + aGap / 8);
		MarkDirty();
	}
}
