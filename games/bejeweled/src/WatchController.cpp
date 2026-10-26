// port(variant): the replay player (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11, 6.8); see
// WatchController.h. Not part of the original game.

#include "WatchController.h"
#include "WinDMApp.h"
#include "Board.h"
#include "StateCodec.h"
#include "Sha256.h"
#include "HighScoreWidget.h"
#include "WidgetManager.h"
#include "MusicInterface.h"
#include "MTRand.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

using namespace Sexy;

namespace
{

const char* EndReasonName(int theReason)
{
	static const char* const aNames[] = {"abandoned", "no moves", "time up", "new game", "gold time up", "saved", "arrow escaped"};
	return ((theReason >= 0) && (theReason < Replay::NUM_END_REASONS)) ? aNames[theReason] : "?";
}

std::string RngText(const Replay::RngStart& theStart, bool isUserSeeded)
{
	if (theStart.mKind == Replay::RNG_SEED)
		return "seed " + std::to_string(theStart.mSeed) + (isUserSeeded ? " user" : "");
	return "global stream state";
}

std::string RngText(const Replay::RngState& theRng)
{
	if (theRng.mSeed == 0)
		return "global stream state";
	return "seed " + std::to_string(theRng.mSeed) + (theRng.mUserSeeded ? " user" : "");
}

bool SameRng(const Replay::RngState& a, const Replay::RngState& b)
{
	return (a.mSeed == b.mSeed) && (a.mUserSeeded == b.mUserSeeded) && (a.mRanked == b.mRanked) && (a.mMti == b.mMti) &&
		(memcmp(a.mMt, b.mMt, sizeof(a.mMt)) == 0);
}

std::string BufferBytes(const Buffer& theBuffer)
{
	if (theBuffer.GetDataLen() <= 0)
		return "";
	return std::string((const char*) theBuffer.GetDataPtr(), (size_t) theBuffer.GetDataLen());
}

void Print(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

}

WatchController::WatchController(WinDMApp* theApp, const std::string& theFileName)
{
	mApp = theApp;
	mFileName = theFileName;
	mHasBytes = false;
	mIsRead = false;
	mStatus = STATUS_IDLE;
	mSwappedIn = false;
	mSavedTimedMode = false;
	mBoard = NULL;
	mSegment = 0;
	mEvent = 0;
	mLastState = -1;
	mCheckDue = -1;
	mBoardEnded = false;
	mNewGameApplied = false;
	mHasCarriedStats = false;
	mPrevSeeded = false;
	mLastSaveExact = false;
	mChecks = 0;
	mEventsApplied = 0;
	mSpeed = 0;
	mPaused = false;
	mStepping = false;
	mStepUpdates = 0;
}

// The file, read and checked before anything changes: no larger than a replay may be, well-formed
// (Replay::Read), RULE canonical rules the game makes (GameRules::CheckRecorded: what the mod path makes of some
// mods, and diff.txt values every level can play with), playable by this build (Replay::CheckPlayable, which also
// checks the save bytes of an LDSV; the original rules from any engine), RLID the SHA-256 of RULE, every RNG state
// one MTRand takes
bool WatchController::Load(std::string* theError)
{
	mIsRead = false;
	std::string aBytes;
	if (mHasBytes)
		aBytes.swap(mBytes); // the verifier's (SetBytes)
	else
	{
		FILE* aFile = fopen(mFileName.c_str(), "rb");
		if (aFile == NULL)
		{
			*theError = "cannot open the file";
			return false;
		}
		// At most one byte more than a replay may have, which Replay::Read then refuses
		char aChunk[65536];
		size_t aCount;
		while ((aBytes.size() <= (size_t) Replay::MAX_FILE_SIZE) && ((aCount = fread(aChunk, 1, sizeof(aChunk), aFile)) > 0))
			aBytes.append(aChunk, aCount);
		fclose(aFile);
	}

	std::string anError;
	bool isRead = Replay::Read(aBytes, &mFile, &anError);
	std::string().swap(aBytes);
	if (!isRead)
	{
		*theError = "not a well-formed replay: " + anError;
		return false;
	}
	mIsRead = true;
	// The rules first (stage 5): a replay of the original rules plays whichever engine recorded it
	if (!mFile.mHasRules || !mRules.FromCanonical(mFile.mRules, &anError))
	{
		*theError = mFile.mHasRules ? "RULE: " + anError : std::string("no RULE record");
		return false;
	}
	// Stage 7: the rules' level shapes; events name cells of the largest
	std::vector<BoardLayout> aShapes;
	int aMaxCols = 0;
	int aMaxRows = 0;
	for (int i = 0; i < mRules.ShapeCount(); i++)
	{
		aShapes.push_back(mRules.LayoutForLevel(i));
		aMaxCols = std::max(aMaxCols, aShapes.back().mCols);
		aMaxRows = std::max(aMaxRows, aShapes.back().mRows);
	}
	if (!Replay::CheckPlayable(mFile, aMaxCols, aMaxRows, &anError, mRules.PlaysAlikeInEveryEngine(), &aShapes, GameConfig::GemColors(mRules)))
	{
		*theError = anError;
		return false;
	}
	uint8_t anId[Sha256::DIGEST_SIZE];
	Sha256::Hash(mFile.mRules.data(), mFile.mRules.size(), anId);
	if (!mFile.mHasRulesId || (memcmp(anId, mFile.mRulesId, sizeof(anId)) != 0))
	{
		*theError = "RLID is not the SHA-256 of RULE";
		return false;
	}
	// Someone else's rules: the play code divides by, indexes and loops with what they say (the 12 diff.txt
	// values give SetupLevel's mLevelPoints, Update's divisor), so only rules the game itself makes. Canonical
	// bytes again: one rule set has one RulesId
	if (mRules.Canonical() != mFile.mRules)
	{
		*theError = "RULE: not the canonical form of the rules it holds";
		return false;
	}
	if (!mRules.CheckRecorded(&anError))
	{
		*theError = "RULE: " + anError;
		return false;
	}
	MTRand aCheck(4357);
	for (size_t s = 0; s < mFile.mSegments.size(); s++)
	{
		const Replay::Segment& aSegment = mFile.mSegments[s];
		if (((aSegment.mRng.mKind == Replay::RNG_STATE) && !aCheck.SetState(aSegment.mRng.mMt, aSegment.mRng.mMti)) ||
			(aSegment.mHasLoadRng && !aCheck.SetState(aSegment.mLoadRng.mMt, aSegment.mLoadRng.mMti)))
		{
			*theError = "segment " + std::to_string(s) + ": an RNG state MTRand refuses";
			return false;
		}
	}
	return true;
}

// The app's state and mOther trade places: the replay's comes in for a step, the player's goes back after it
void WatchController::Exchange()
{
	std::swap(mApp->mShowInstructions, mOther.mShowInstructions);
	std::swap(mApp->mTimesPlayed, mOther.mTimesPlayed);
	std::swap(gGameConfig, mOther.mConfig);
	DiffValues anApp;
	anApp.mNormalPointsBase = mApp->mNormalPointsBase;
	anApp.mNormalPointsPerLevel = mApp->mNormalPointsPerLevel;
	anApp.mNormalMultBase = mApp->mNormalMultBase;
	anApp.mNormalMultPerLevel = mApp->mNormalMultPerLevel;
	anApp.mTimedPointsBase = mApp->mTimedPointsBase;
	anApp.mTimedPointsPerLevel = mApp->mTimedPointsPerLevel;
	anApp.mTimedMultBase = mApp->mTimedMultBase;
	anApp.mTimedMultPerLevel = mApp->mTimedMultPerLevel;
	anApp.mTimeDrainBase = mApp->mTimeDrainBase;
	anApp.mTimeDrainPerLevel = mApp->mTimeDrainPerLevel;
	anApp.mTimeDrainAccelBase = mApp->mTimeDrainAccelBase;
	anApp.mTimeDrainAccelPerLevel = mApp->mTimeDrainAccelPerLevel;
	const DiffValues& aDiff = mOther.mDiff;
	mApp->mNormalPointsBase = aDiff.mNormalPointsBase;
	mApp->mNormalPointsPerLevel = aDiff.mNormalPointsPerLevel;
	mApp->mNormalMultBase = aDiff.mNormalMultBase;
	mApp->mNormalMultPerLevel = aDiff.mNormalMultPerLevel;
	mApp->mTimedPointsBase = aDiff.mTimedPointsBase;
	mApp->mTimedPointsPerLevel = aDiff.mTimedPointsPerLevel;
	mApp->mTimedMultBase = aDiff.mTimedMultBase;
	mApp->mTimedMultPerLevel = aDiff.mTimedMultPerLevel;
	mApp->mTimeDrainBase = aDiff.mTimeDrainBase;
	mApp->mTimeDrainPerLevel = aDiff.mTimeDrainPerLevel;
	mApp->mTimeDrainAccelBase = aDiff.mTimeDrainAccelBase;
	mApp->mTimeDrainAccelPerLevel = aDiff.mTimeDrainAccelPerLevel;
	mOther.mDiff = anApp;
	mSwappedIn = !mSwappedIn;
}

// TitleScreenIsFinished, instead of the two new games: the replay's state, the first segment's board
bool WatchController::Start()
{
	std::string anError;
	if (!Load(&anError))
	{
		mResult = "REPLAY " + mFileName + ": cannot play: " + anError;
		Print(mResult);
		mStatus = STATUS_STOPPED;
		if (mApp->mReplayQuit)
		{
			mApp->Shutdown();
			return true;
		}
		return false; // the game starts as usual
	}

	size_t anEvents = 0;
	for (size_t s = 0; s < mFile.mSegments.size(); s++)
		anEvents += mFile.mSegments[s].mEvents.size();
	Print("REPLAY start " + mFileName + ": " + std::to_string(mFile.mSegments.size()) + " segments, " + std::to_string(anEvents) +
		" events, END " + EndReasonName(mFile.mEnd.mReason) + " tick " + std::to_string(mFile.mEnd.mEndTick));

	// The replay's state: its rules (tunables and diff.txt values included), no features, no seed; the
	// player's switches that are not rules (-forcevariant, the mods sign) stay
	mOther = AppState();
	mOther.mConfig.mRules = mRules;
	mOther.mConfig.LoadFromRules(mRules);
	mOther.mConfig.mAnyModEnabled = gGameConfig.mAnyModEnabled;
	mOther.mConfig.mForceVariant = gGameConfig.mForceVariant;
	mOther.mConfig.mReplayDir = gGameConfig.mReplayDir;
	mOther.mConfig.mFeatures.mGameStats = gGameConfig.mFeatures.mGameStats; // the Stats page: it counts, it plays nothing
	mOther.mDiff = mRules.mDiff;
	mSavedTimedMode = mApp->mTimedMode;
	mStatus = STATUS_PLAYING;
	mSegment = 0;
	Exchange();
	CreateBoard();
	Exchange();
	return true;
}

// The current segment's playback board, in place of the board and the high-score screen there are (a game
// in progress is saved by ~Board, as on a mode switch), as NewGame makes one. Called swapped in.
void WatchController::CreateBoard()
{
	const Replay::Segment& aSegment = mFile.mSegments[mSegment];
	WinDMApp* anApp = mApp;
	if (anApp->mHighScoreWidget != NULL)
	{
		anApp->mWidgetManager->RemoveWidget(anApp->mHighScoreWidget);
		anApp->SafeDeleteWidget(anApp->mHighScoreWidget);
		anApp->mHighScoreWidget = NULL;
	}
	// The Stats page (GameStats.h): the counts of the segment before go on after this segment's LOAD
	mHasCarriedStats = (anApp->mBoard != NULL) && (anApp->mBoard->mStats != NULL);
	if (mHasCarriedStats)
		mCarriedStats = *anApp->mBoard->mStats;
	if (anApp->mBoard != NULL)
	{
		anApp->mWidgetManager->RemoveWidget(anApp->mBoard);
		anApp->SafeDeleteWidget(anApp->mBoard);
		anApp->mBoard = NULL;
	}

	bool isTimed = (aSegment.mMode != 0);
	anApp->SetTimedMode(isTimed, false);
	anApp->mShowInstructions = aSegment.mShowInstructions;
	anApp->mTimesPlayed = aSegment.mTimesPlayed;
	gGameConfig.mNextBoardSeed = 0;
	gGameConfig.mNextBoardUserSeeded = false;
	gGameConfig.mNextBoardPlayback = &aSegment;
	anApp->EnsureGemColors(gGameConfig.mGemColors);   // the replay's rules may want more colours than loaded
	mBoard = new Board(anApp, isTimed);
	anApp->mBoard = mBoard;
	// Only the player updates it (Step), once per frame like the game
	mBoard->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE;
	mBoard->Resize(0, 0, 640, 480);
	anApp->mWidgetManager->AddWidget(mBoard);
	anApp->mWidgetManager->BringToBack(mBoard);
	anApp->mWidgetManager->SetFocus(mBoard);
	anApp->mMusicInterface->FadeOut(isTimed ? MUSIC_NORMAL : MUSIC_TIMED, true, 0.004);
	anApp->mMusicInterface->FadeIn(isTimed ? MUSIC_TIMED : MUSIC_NORMAL, -1, 0.002, false);
	anApp->OnWatchBoard(); // the overlay over it, with the focus

	mEvent = 0;
	mLastState = mBoard->mState;
	mCheckDue = -1;
	mBoardEnded = false;
	mNewGameApplied = false;
	std::string aLine = "REPLAY segment " + std::to_string(mSegment) + ": " + (isTimed ? "timed" : "normal") + ", " +
		RngText(aSegment.mRng, aSegment.mUserSeeded);
	if (aSegment.mHasLoadRng)
		aLine += ", LOAD " + RngText(aSegment.mLoadRng) + (aSegment.mHasLoadSave ? " (its own save)" : "");
	Print(aLine);
}

void WatchController::BeforeFrame()
{
	if (mStatus != STATUS_PLAYING)
		return;
	// A dialog the player opened holds the watch, as it pauses a game
	if ((mApp->mWidgetManager->mBaseModalWidget != NULL) || mPaused)
		return;

	// 1 << speed updates a frame; a step to the next move at 8x, which pauses before it
	int anUpdates = mStepping ? (1 << (NUM_SPEEDS - 1)) : (1 << mSpeed);
	bool isPlaying = true;
	Exchange();
	for (int i = 0; (i < anUpdates) && isPlaying; i++)
	{
		if (mStepping && (mStepUpdates > 0) && IsMoveDue())
		{
			mStepping = false;
			mPaused = true;
			break;
		}
		isPlaying = Step();
		mStepUpdates++;
	}
	Exchange();
	if (isPlaying)
		return;

	mStatus = STATUS_ENDED;
	mStepping = false;
	Print(mResult);
	if (mApp->mReplayQuit)
	{
		Stop();
		mApp->Shutdown();
	}
	else
		mApp->OnWatchEnded(); // REPLAY END
}

void WatchController::SetSpeed(int theSpeed)
{
	mSpeed = std::max(0, std::min(theSpeed, (int) NUM_SPEEDS - 1));
	mPaused = false;
	mStepping = false;
}

void WatchController::TogglePause()
{
	mPaused = mStepping || !mPaused;
	mStepping = false;
}

void WatchController::StepToMove()
{
	if (mStatus != STATUS_PLAYING)
		return;
	mPaused = false;
	mStepping = true;
	mStepUpdates = 0;
}

// The events the next Step applies first, those at the board's tick: a swap's target (not one that only drops the
// selection), a twist, a slide or an undo
bool WatchController::IsMoveDue() const
{
	const std::vector<Replay::Event>& anEvents = mFile.mSegments[mSegment].mEvents;
	uint32_t aTick = (uint32_t) mBoard->mUpdateCnt;
	for (size_t i = mEvent; (i < anEvents.size()) && (anEvents[i].mTick == aTick); i++)
	{
		const Replay::Event& anEvent = anEvents[i];
		if ((anEvent.mOp == Replay::OP_TWIST) || (anEvent.mOp == Replay::OP_SLIDE) || (anEvent.mOp == Replay::OP_UNDO) ||
			((anEvent.mOp == Replay::OP_TARGET) && (anEvent.mArgs[0] != -1)))
			return true;
	}
	return false;
}

// One iteration of the playback loop (6.8). Swapped in.
bool WatchController::Step()
{
	Board* b = mBoard;
	const Replay::Segment& aSegment = mFile.mSegments[mSegment];
	const std::vector<Replay::Event>& anEvents = aSegment.mEvents;
	bool isLast = (mSegment + 1 == mFile.mSegments.size());
	uint32_t aTick = (uint32_t) b->mUpdateCnt;

	// The recording's CHECK of a settle is the first event at the tick the board settled at
	if ((mCheckDue >= 0) && ((mEvent >= anEvents.size()) || (anEvents[mEvent].mTick != aTick) || (anEvents[mEvent].mOp != Replay::OP_CHECK)))
		return Corrupt("the board settled into play at tick " + std::to_string(mCheckDue) + ", and the replay has no CHECK there");
	while ((mEvent < anEvents.size()) && (anEvents[mEvent].mTick == aTick))
	{
		if (!Apply(anEvents[mEvent]))
			return false;
		mEvent++;
		mEventsApplied++;
	}
	if ((mEvent < anEvents.size()) && (anEvents[mEvent].mTick < aTick))
		return Corrupt("an event for a tick the board has passed", &anEvents[mEvent]);

	bool isDone = (mEvent == anEvents.size());
	if (isDone && !isLast)
		return NextSegment();
	if (isLast)
	{
		if (mBoardEnded)
		{
			if (!isDone)
				return Corrupt("events after the board's game ended (New Game)", &anEvents[mEvent]);
			return Finish();
		}
		if (isDone && ((b->mState == STATE_GAME_OVER_DONE) || (aTick >= mFile.mEnd.mEndTick)))
			return Finish();
		if (aTick >= mFile.mEnd.mEndTick)
			return Corrupt("the replay goes on past its END tick " + std::to_string(mFile.mEnd.mEndTick));
	}
	if ((b->mPauseCount > 0) || (b->mUserPauseCount > 0))
		return Corrupt("the board is paused at tick " + std::to_string(aTick) + ", and no event unpauses it");

	// ENV: what the tick-150 block of this update reads (the recorder took it at the start of the same update)
	if (aTick == 149)
	{
		mApp->mShowInstructions = aSegment.mShowInstructions;
		mApp->mTimesPlayed = aSegment.mTimesPlayed;
	}
	b->Update();
	if ((b->mState == STATE_PLAYING) && (mLastState != STATE_PLAYING))
		mCheckDue = b->mUpdateCnt;
	mLastState = b->mState;
	return true;
}

Piece* WatchController::CellPiece(int64_t theCol, int64_t theRow)
{
	if ((theCol < 0) || (theCol >= mBoard->mCols) || (theRow < 0) || (theRow >= mBoard->mRows))
		return NULL;
	return mBoard->mBoard[theRow][theCol];
}

// One event, after its gate (6.3): the conditions under which the live input handler would have recorded it.
// Swapped in.
bool WatchController::Apply(const Replay::Event& theEvent)
{
	Board* b = mBoard;
	const int64_t* a = theEvent.mArgs;
	// MouseDown's gates, before it selects, deselects or targets: no user pause, no input delay, not in game
	// over, the vortex or the wipe
	bool isMouseDownOpen = (b->mUserPauseCount == 0) && (b->mInputDelay == 0) && (b->mState != STATE_GAME_OVER) &&
		(b->mState != STATE_GAME_OVER_DONE) && (b->mState != STATE_BLACK_HOLE) && (b->mState != STATE_LEVEL_WIPE);
	b->mPlaybackApplying = true;
	bool isOk = true;
	std::string aWhy;
	switch (theEvent.mOp)
	{
	case Replay::OP_SELECT:
		{
			Piece* aPiece = CellPiece(a[0], a[1]);
			if (!isMouseDownOpen || (b->mSelectedPiece != NULL) || (aPiece == NULL))
			{
				aWhy = "a selection MouseDown would not make";
				isOk = false;
				break;
			}
			b->mCursorFrame = 0;
			b->mSelectedPiece = aPiece;
			b->MarkDirty();
			mApp->PlaySample(SOUND_SELECT, b->GetPanForX((int)(aPiece->mX + 24.0)));
		}
		break;

	case Replay::OP_DESELECT:
		if (!isMouseDownOpen)
		{
			aWhy = "a right click MouseDown would ignore";
			isOk = false;
			break;
		}
		b->mSelectedPiece = NULL;
		b->MarkDirty();
		break;

	case Replay::OP_TARGET:
		{
			// A click (MouseDown) or a drag (MouseDrag): both need a selection and the board in play
			Piece* aTarget = CellPiece(a[0], a[1]);
			if ((b->mState != STATE_PLAYING) || (b->mSelectedPiece == NULL) || ((aTarget == NULL) && (a[0] != -1)))
			{
				aWhy = "a target without a selection, out of play or on an empty cell";
				isOk = false;
				break;
			}
			if ((aTarget != NULL) && (aTarget != b->mSelectedPiece))
				b->StartSwap(b->mSelectedPiece, aTarget);
			b->mSelectedPiece = NULL;
			b->MarkDirty();
		}
		break;

	case Replay::OP_TWIST:
		// Stage 10: the gate the input layer records it behind (Board::CanTwist: in play, open to input, the rules'
		// twists that way, four gems)
		if (!b->mVariant.mMoves || !b->CanTwist((int) a[0], (int) a[1], (int) a[2]))
		{
			aWhy = "a twist the board would not make";
			isOk = false;
			break;
		}
		b->StartTwist((int) a[0], (int) a[1], (int) a[2]);
		break;

	case Replay::OP_SLIDE:
		// ... (Board::CanSlide: a whole segment of gems from that cell, 1 <= k < its length)
		if (!b->mVariant.mMoves || !b->CanSlide((int) a[0], (int) a[1], (int) a[2], (int) a[3], (int) a[4]))
		{
			aWhy = "a slide the board would not make";
			isOk = false;
			break;
		}
		b->StartSlide((int) a[0], (int) a[1], (int) a[2], (int) a[3], (int) a[4]);
		break;

	case Replay::OP_HINT:
		// ShowHint's gate; the board's own ShowHint ignores a playback board. Stage 5: a board playing without a
		// move gives no hint either (Board::NoHintVariant)
		if ((b->mState != STATE_PLAYING) || (b->mPauseCount != 0) || b->NoHintVariant())
		{
			aWhy = "a hint ShowHint would not give";
			isOk = false;
			break;
		}
		b->DoHint();
		break;

	case Replay::OP_UNDO:
		// Stage 6: RequestUndo's gate (Board::CanUndo); the board's own RequestUndo ignores a playback board
		if (!b->CanUndo())
		{
			aWhy = "an undo RequestUndo would refuse";
			isOk = false;
			break;
		}
		b->Undo();
		break;

	case Replay::OP_PAUSE:
		b->Pause(a[0] != 0);
		break;

	case Replay::OP_UNPAUSE:
		b->Unpause(a[0] != 0);
		break;

	case Replay::OP_NEW_GAME:
		b->RequestNewGame();
		mNewGameApplied = true;
		break;

	case Replay::OP_XRAND:
		// Draws from the global stream that were not the board's: only a board on that stream sees them. Its
		// MTRand draws them, past Board::Rand, so mBoardDraws stays the recorded board's
		if ((b->mGameSeed != 0) || (b->mOwnRng == NULL))
		{
			aWhy = "outside draws for a board with its own seed";
			isOk = false;
			break;
		}
		for (int64_t i = 0; i < a[0]; i++)
			b->mOwnRng->Next();
		break;

	case Replay::OP_HOVER:
		b->mMouseX = (int) a[0];
		b->mMouseY = (int) a[1];
		break;

	case Replay::OP_SAVE:
		{
			// The save holds the raw pieces, whose hover glow follows the pointer: only a replay with the hover
			// channel makes the same bytes (without it, the LOGIC hash, which leaves the glow out, still matches).
			// In the vortex their positions and angles come from libm's cos, sin and atan2, whose last bits differ
			// between platforms (the LOGIC hash leaves them out too): a replay recorded natively, played in wasm
			// (the verifier, stage 4), or the other way round, makes other bytes there
			mLastSaveExact = ((mFile.mFlags & Replay::FLAG_HOVER) != 0) && (b->mState != STATE_BLACK_HOLE);
			if (a[0] == Replay::SAVE_GAME)
				b->SaveGame(); // into mPlaybackSave (Board::WriteSaveFile)
			else
			{
				Buffer aSnapshot;
				b->SaveGameSnapshot(&aSnapshot);
				b->mPlaybackSave = aSnapshot;
			}
			// Hashed only where it is compared
			if (mLastSaveExact)
			{
				std::string aSave = BufferBytes(b->mPlaybackSave);
				uint32_t aHash = Replay::SaveHash(aSave.data(), aSave.size());
				if (aHash != (uint32_t) a[1])
				{
					char aText[64];
					snprintf(aText, sizeof(aText), "the save's hash is %08x, the replay's %08x", (unsigned) aHash, (unsigned) a[1]);
					aWhy = aText;
					isOk = false;
					break;
				}
			}
			// What the resume of the next segment may check its RNGS against
			Replay::RngState aRng;
			b->GetRngState(&aRng);
			mPrevSeeded = (b->mGameSeed != 0);
			mPrevRng = aRng;
		}
		break;

	case Replay::OP_LOAD:
		{
			const Replay::Segment& aSegment = mFile.mSegments[mSegment];
			if (b->mUpdateCnt != 0)
			{
				aWhy = "a LOAD after the board's first update";
				isOk = false;
				break;
			}
			if (aSegment.mHasLoadSave)
			{
				mLoadSave.Clear();
				mLoadSave.WriteBytes((const uchar*) aSegment.mLoadSave.data(), (int) aSegment.mLoadSave.size());
			}
			b->mPlaybackLoad = &mLoadSave;
			bool isLoaded = b->LoadGame();
			b->mPlaybackLoad = NULL;
			if (!isLoaded)
			{
				aWhy = "the save does not load";
				isOk = false;
				break;
			}
			// The Stats page counts the whole game: the segment before saved it, unless the replay holds only the save
			if (b->mStats != NULL)
			{
				bool isCarried = mHasCarriedStats && !aSegment.mHasLoadSave;
				if (isCarried)
					*b->mStats = mCarriedStats;
				else
				{
					*b->mStats = GameStats();
					b->mStats->mFromResume = true;
				}
				b->mStats->OnLoad(b);
			}
			// A game the segment before played on its own seed goes on with that RNG as it was at the save (6.5)
			const Replay::RngState& aRng = aSegment.mLoadRng;
			if (!aSegment.mHasLoadSave && mPrevSeeded && !SameRng(aRng, mPrevRng))
			{
				aWhy = "RNGS is not the RNG the saved game had";
				isOk = false;
				break;
			}
			// A variant board's v2 save holds the RNG the game goes on with (its RNGS chunk, stage 5): the segment's
			// RNGS must be that one (LDSV bytes too)
			Replay::RngState aSaved;
			b->GetRngState(&aSaved);
			if (b->mVariant.mAny && ((aSaved.mSeed != 0) || (aRng.mSeed != 0)) && !SameRng(aRng, aSaved))
			{
				aWhy = "RNGS is not the RNG the v2 save holds";
				isOk = false;
				break;
			}
			if (!b->SetPlaybackRng(aRng))
			{
				aWhy = "an RNGS state MTRand refuses";
				isOk = false;
			}
		}
		break;

	case Replay::OP_CHECK:
		{
			if (mCheckDue != (int) theEvent.mTick)
			{
				aWhy = "a CHECK where the board did not settle";
				isOk = false;
				break;
			}
			mCheckDue = -1;
			uint64_t aHash = StateCodec::LogicHash(b);
			if (aHash != (uint64_t) a[0])
			{
				char aText[96];
				snprintf(aText, sizeof(aText), "LOGIC hash %016llx, the replay's %016llx", (unsigned long long) aHash, (unsigned long long) a[0]);
				aWhy = aText;
				isOk = false;
				break;
			}
			mChecks++;
		}
		break;

	default:
		aWhy = "a command this build does not play";
		isOk = false;
		break;
	}
	b->mPlaybackApplying = false;
	return isOk || Corrupt(aWhy, &theEvent);
}

// The segment's last SAVE made the bytes the next one LOADs (unless it brings its own); then its board
bool WatchController::NextSegment()
{
	mLoadSave = mBoard->mPlaybackSave;
	mSegment++;
	CreateBoard();
	return true;
}

// The replay's END against the board: the game over, New Game, or the tick it was dropped or saved at
bool WatchController::Finish()
{
	Board* b = mBoard;
	const Replay::EndInfo& anEnd = mFile.mEnd;
	Replay::EndInfo aGot = b->GetEndInfo(anEnd.mReason);
	const std::vector<Replay::Event>& anEvents = mFile.mSegments[mSegment].mEvents;
	bool isGameOver = (b->mState == STATE_GAME_OVER_DONE);
	std::string aWhy;
	if ((anEnd.mReason == Replay::END_NO_MOVES) || (anEnd.mReason == Replay::END_TIME_UP) || (anEnd.mReason == Replay::END_GOLD_TIME_UP) ||
		(anEnd.mReason == Replay::END_ARROW_ESCAPED))
	{
		if (!isGameOver)
			aWhy = "END says the game was over, the board is not";
	}
	else if (anEnd.mReason == Replay::END_NEW_GAME)
	{
		if (!mNewGameApplied)
			aWhy = "END says New Game, the replay has none";
	}
	else if (anEnd.mReason == Replay::END_SAVED)
	{
		if (anEvents.empty() || (anEvents.back().mOp != Replay::OP_SAVE))
			aWhy = "END says saved, the replay does not end in a SAVE";
	}
	else if (isGameOver)
		aWhy = "the game is over, END does not say so";
	if (aWhy.empty() && ((aGot.mEndTick != anEnd.mEndTick) || (aGot.mScore != anEnd.mScore) || (aGot.mLevel != anEnd.mLevel) ||
		(aGot.mGems != anEnd.mGems) || (aGot.mBoardDraws != anEnd.mBoardDraws) || (aGot.mLogicHash != anEnd.mLogicHash)))
	{
		char aText[256];
		snprintf(aText, sizeof(aText), "END differs: tick %u score %d level %d gems %d draws %u hash %016llx, the replay's tick %u score %d "
			"level %d gems %d draws %u hash %016llx", (unsigned) aGot.mEndTick, (int) aGot.mScore, (int) aGot.mLevel, (int) aGot.mGems,
			(unsigned) aGot.mBoardDraws, (unsigned long long) aGot.mLogicHash, (unsigned) anEnd.mEndTick, (int) anEnd.mScore,
			(int) anEnd.mLevel, (int) anEnd.mGems, (unsigned) anEnd.mBoardDraws, (unsigned long long) anEnd.mLogicHash);
		aWhy = aText;
	}
	// A sidecar: the save it goes with (where SAVE compares the bytes: with the hover channel, out of the vortex),
	// and the RNG the save was made with
	if (aWhy.empty() && mFile.mHasSaveHash && mLastSaveExact)
	{
		std::string aSave = BufferBytes(b->mPlaybackSave);
		if (Replay::SaveHash(aSave.data(), aSave.size()) != mFile.mSaveHash)
			aWhy = "SVHA is not the hash of the last save";
	}
	if (aWhy.empty() && mFile.mHasTrailingRng)
	{
		Replay::RngState aRng;
		b->GetRngState(&aRng);
		if (!SameRng(aRng, mFile.mTrailingRng))
			aWhy = "the sidecar's RNGS is not the board's RNG at the save";
	}
	if (!aWhy.empty())
		return Corrupt(aWhy);

	char aText[256];
	snprintf(aText, sizeof(aText), "REPLAY end: ok, %d CHECKs, END %s tick %u score %d level %d gems %d draws %u", mChecks,
		EndReasonName(anEnd.mReason), (unsigned) aGot.mEndTick, (int) aGot.mScore, (int) aGot.mLevel, (int) aGot.mGems,
		(unsigned) aGot.mBoardDraws);
	mResult = aText;
	return false;
}

bool WatchController::Corrupt(const std::string& theWhy, const Replay::Event* theEvent)
{
	std::string aWhere = "segment " + std::to_string(mSegment) + " tick " + std::to_string(mBoard->mUpdateCnt);
	if (theEvent != NULL)
	{
		const std::vector<Replay::Event>& anEvents = mFile.mSegments[mSegment].mEvents;
		aWhere += " event " + std::to_string(theEvent - anEvents.data()) + " " + Replay::OpName(theEvent->mOp);
		for (int i = 0; i < Replay::OpArgCount(theEvent->mOp); i++)
			aWhere += " " + std::to_string((long long) theEvent->mArgs[i]);
	}
	mResult = "REPLAY end: CORRUPT at " + aWhere + ": " + theWhy;
	return false;
}

// Back to the game: the player's mode (the rest of the player's state never left the app between steps)
void WatchController::Stop()
{
	if (mSwappedIn)
		Exchange();
	mStatus = STATUS_STOPPED;
	mApp->SetTimedMode(mSavedTimedMode, false);
}

// A playback board is the watch's (only it makes them); once the replay has ended it is its last one, whatever
// board NewGame has made since (WatchController.h)
bool WatchController::IsEndedBoard(const Board* theBoard) const
{
	return (theBoard != NULL) && theBoard->mPlaybackOnly && (mStatus != STATUS_PLAYING);
}

bool WatchController::OnBoardEnd(bool isGameOverScreen)
{
	if ((mStatus != STATUS_PLAYING) || (mBoard == NULL) || (mApp->mBoard != mBoard))
		return false;
	// The game-over screen: the board is in STATE_GAME_OVER_DONE, which ends the replay. New Game's fall is
	// over: the recorded board was deleted here.
	if (!isGameOverScreen)
		mBoardEnded = true;
	return true;
}

// New Game and the mode buttons end the watch: the game the player left is offered again (4.11). Options,
// Quit and Hint (which a playback board ignores) work as usual; the overlay covers the undo orb.
bool WatchController::ButtonDepress(int theId)
{
	if (!IsWatching())
		return false;
	if ((theId != BUTTON_NEW_GAME) && (theId != BUTTON_NORMAL_MODE) && (theId != BUTTON_TIMED_MODE))
		return false;
	mApp->EndWatch();
	return true;
}
