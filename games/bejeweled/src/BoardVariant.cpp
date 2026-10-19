// port(variant): Board members for code mods (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.3). Not part of
// the original game. Stage 1 (9.2): the rules snapshot, the layout and the variant flags. The layout is the
// original 8x8 whatever the rules say, and no translated function reads the flags yet, so every board plays
// as the original; later stages switch the generalised paths on.
// Stage 2: the board's RNG (6.2). A seed the player chose, any rule, or the seeds.perGame feature gives
// the board its own MTRand, which Board::Rand draws from; every other board draws from the global stream
// as the original does.
// Stage 3: replays (6.3-6.5). The recorder and playback members, the RNG start a recording begins with,
// the sidecar next to the save (7.3) that keeps a recording and a board's own RNG across a resume, and the
// scratch board StateCodec's round trip restores into. Stage 3b: the playback board a replay player
// (WatchController) makes, with its own MTRand from the segment's RNG start, and the RNG its LOAD goes on with.

#include "Board.h"
#include "GameConfig.h"
#include "WinDMApp.h"
#include "ReplayRecorder.h"
#include "StateCodec.h"
#include "Common.h"

#include <SDL.h>
#include <cstring>

using namespace Sexy;

// Called first in the constructor (0x4cb5bc), before SetupLevel and FillBoard use the layout and the RNG
void Board::InitVariant()
{
	mRules = gGameConfig.mRules;
	// The board shape rules (board.cols/rows/mask/levels) take effect in stage 7 (FillBoardVariant,
	// RefillMasked, DrawVariant); until then the layout keeps the original constants
	mLayout = BoardLayout();
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;

#ifndef BEJ_NO_VARIANTS
	mVariant = ComputeVariantFlags(mRules, gGameConfig.mFeatures);
	if (gGameConfig.mForceVariant)
	{
		// -forcevariant: every generalised path at default rules; the RNG source, the save format and the
		// tutorial stay original (mAny, mOwnRng and mNoTutorial are left alone)
		mVariant.mShape = true;
		mVariant.mMatch = true;
		mVariant.mSearch = true;
		mVariant.mDraw = true;
	}
#endif

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

// The end of a LoadGame (0x4cca98) that succeeded. The resumed game is not the one the board's seed would
// make: it goes on with the RNG it was saved with. The v1 save holds none, so the sidecar does
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.3; stage 5's v2 save keeps it in its RNGS chunk): a sidecar
// that goes with this save and holds an RNGS record gives the board that MTRand state with its seed, so a
// seeded game keeps its stream, its SEED line and (a seed the player chose) the -seeded table, and a quit
// and resume cannot reroll it. Without one (a game on the global stream, or a save from before stage 3)
// the game goes on on the global stream. Either way the pending seed stays for the next game. A recording
// then records LOAD and goes on with the sidecar's replay.
void Board::GameLoadedVariant(const Buffer& theSave)
{
	mTakesPendingSeed = false;
	// A replay's LOAD (stage 3b): no file of the user dir is read; the player gives the board the RNG the
	// recorded one went on with, the segment's RNGS (SetPlaybackRng)
	if (mPlaybackOnly)
		return;
	std::string aSave((const char*) theSave.GetDataPtr(), (size_t) theSave.GetDataLen());
	Replay::File aSidecar;
	bool hasSidecar = ReadSaveSidecar(aSave, &aSidecar);

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
std::string Board::GetSidecarFileName()
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
	if (mOwnRng != NULL)
		return RngSidecar(this, theSave);
	return "";
}

// SaveGame (0x4ccdfe), just before the save is written: a recording records SAVE and leaves the game so far
// next to the save; a board with its own RNG leaves that RNG. Any other save has no sidecar, so an old one
// (which could not match it anyway) goes. A game on the global stream without replays never had one: the
// erase finds nothing and changes nothing. The first save of a recording board replaces another board's
// save (a mode switch saved one game, the next is saved too): that game's recording is kept first.
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
	else if (mOwnRng != NULL)
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
// recorded game in it ended by being saved: it is kept as a finished replay first.
void Board::DeleteSaveSidecar()
{
	if ((mRecorder != NULL) && !mRecorder->Loaded())
		KeepSidecarGame();
	mApp->EraseFile(GetSidecarFileName());
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

	// The tutorial looks for a horizontal 3-run swap to the right in rows 4..6, columns 1..6
	bool canTutorial = ((theRules.mMoves.mTypes & GameRules::MOVE_SWAP) != 0) && (aMatch.mMinRun == 3) && !aMatch.mDiagonal &&
		(aFirst.mCols >= 8) && (aFirst.mRows >= 7);
	for (int aRow = 4; (aRow <= 6) && canTutorial; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
			canTutorial = canTutorial && aFirst.IsPlayable(aCol, aRow);
	}
	aFlags.mNoTutorial = !canTutorial;

	aFlags.mSpecials = hasSpecials;
	aFlags.mGold = theRules.mGold.mEnabled;
	aFlags.mUndo = theRules.mUndo.mEnabled;
	aFlags.mMatch = hasPatterns || (aMatch.mMinRun != 3) || hasSpecials || aFlags.mGold;
	aFlags.mSearch = hasPatterns || (aMatch.mMinRun != 3) || hasNonSwapMoves;
	aFlags.mMoves = (theRules.mMoves.mTypes & (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE)) != 0;
	aFlags.mInvalid = theRules.mMoves.mAllowInvalid;
	aFlags.mNoMoves = (theRules.mNoMoves.mNormal != GameRules::NOMOVES_GAME_OVER) || (theRules.mNoMoves.mTimed != GameRules::NOMOVES_FALL_OUT);
	aFlags.mDraw = aFlags.mShape || hasSpecials || aFlags.mGold || hasNonSwapMoves || aFlags.mNoTutorial;
	return aFlags;
}
