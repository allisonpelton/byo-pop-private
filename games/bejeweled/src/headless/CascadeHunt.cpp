// port(variant): bejeweled_verify cascadehunt (Verifier.h): a bot that plays the original rules from seeds, tries every
// move at every settle and keeps the biggest cascades, and replays of them. Not part of the original game.

#include "Verifier.h"
#include "HeadlessApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "GameRules.h"
#include "StateCodec.h"
#include "Replay.h"
#include "Buffer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

// The points font's ascent in every recording's ENV (the AlphaDigits font): it places the score popups
const int POINTS_ASCENT = 32;
// Updates a move's cascade, or a level-up's vortex, wipe and fall, may take
const int SETTLE_CAP = 20000;

enum Policy
{
	POLICY_GREEDY,      // of the moves that leave SAFE_MOVES or more, the one that scores most (the first of equals)
	POLICY_RANDOM,      // any of those, from an MT19937 on the seed
	POLICY_SURVIVE,     // the move that scores most of those with a line of gSurviveDepth moves on which the board keeps
	                    // SAFE_MOVES (or 2), with deeper lines on a narrow board (Game::Choose); else as greedy
	POLICY_FAIR         // knowing only the board in play: the move least likely to end the game, the refills guessed
	                    // on gFairSamples boards (Game::ChooseFair)
};
// The moves a move must leave to be safe; with none that does, the bot takes the one that leaves most
const int SAFE_MOVES = 3;
// POLICY_SURVIVE's lookahead: the move and this many more after it (--depth)
int gSurviveDepth = 4;
// POLICY_SURVIVE: the trials one search of a move may make
const int SURVIVE_BUDGET = 20000;
// POLICY_FAIR: the guessed refills each move is tried with (--samples); the risk above the least that a move may have
// to be chosen for its points (--tolerance); the least risk above which it looks a move further, and the guesses each
// of those next moves is tried with
int gFairSamples = 16;
double gFairTolerance = 0.0;
const double FAIR_DEEPER = 0.125;
const int FAIR_NEXT_SAMPLES = 4;
// play: a line with the score every this many moves (--progress; 0: none), and each game recorded and verified as
// <gRecord>-seed<s>.bjr (--record <prefix>; empty: none)
int gProgress = 0;
std::string gRecord;

struct Move
{
	int		mCol1 = 0;
	int		mRow1 = 0;
	int		mCol2 = 0;
	int		mRow2 = 0;
};

std::string MoveText(const Move& theMove)
{
	return std::to_string(theMove.mCol1) + "," + std::to_string(theMove.mRow1) + "," + std::to_string(theMove.mCol2) + "," +
		std::to_string(theMove.mRow2);
}

bool ParseMove(const std::string& theText, Move* theMove)
{
	return sscanf(theText.c_str(), "%d,%d,%d,%d", &theMove->mCol1, &theMove->mRow1, &theMove->mCol2, &theMove->mRow2) == 4;
}

// What a move does, from the swap to the first settle after it: into play (or the vortex) or the game over. The
// level is beaten when the score reaches its target: the vortex starts once the score shown has counted up to it, in
// play (Board::Update)
struct Outcome
{
	int		mPoints = 0;        // mMoveScore
	int		mRounds = 0;        // rounds of matches: the swap's and each refill's that matched (mCascadeCount - 1)
	int		mRuns = 0;          // runs scored (mMoveMatchCount)
	int		mState = -1;        // the board's state at that settle; -1: none within SETTLE_CAP
	int		mLevel = 0;         // the level the move was made on (mDisplayLevel)
	bool	mBeaten = false;    // the score reached the level's target (mLevelTargetScore)
	int		mMovesLeft = 0;     // the moves of the board in play after it (FindMove); 0 at the game over

	bool Same(const Outcome& o) const
	{
		return (mPoints == o.mPoints) && (mRounds == o.mRounds) && (mRuns == o.mRuns) && (mState == o.mState);
	}
};

std::string OutcomeText(const Outcome& theOutcome)
{
	const char* anEnd = theOutcome.mBeaten ? ", level beaten" :
		((theOutcome.mState == STATE_GAME_OVER) || (theOutcome.mState == STATE_GAME_OVER_DONE)) ? ", game over" :
		(theOutcome.mState < 0) ? ", no settle" : "";
	return std::to_string(theOutcome.mRounds) + " rounds, " + std::to_string(theOutcome.mRuns) + " runs, " +
		std::to_string(theOutcome.mPoints) + " points" + anEnd;
}

bool IsSettled(int theState)
{
	return (theState == STATE_PLAYING) || (theState == STATE_BLACK_HOLE) || (theState == STATE_GAME_OVER) ||
		(theState == STATE_GAME_OVER_DONE);
}

// A -gameseed game's segment: normal mode, the board's own seed, the instructions long gone
Replay::Segment SeedSegment(uint32_t theSeed)
{
	Replay::Segment aSegment;
	aSegment.mMode = 0;
	aSegment.mRng.mKind = Replay::RNG_SEED;
	aSegment.mRng.mSeed = theSeed;
	aSegment.mUserSeeded = true;
	aSegment.mShowInstructions = false;
	aSegment.mTimesPlayed = 5;
	aSegment.mPointsAscent = POINTS_ASCENT;
	return aSegment;
}

// A board made as WatchController::CreateBoard makes one for theSegment, on an app of its own, and played as the
// player plays a replay (WatchController::Step and Apply). Recording, it keeps the events of that play as the
// recorder writes them: SELECT, TARGET, LOAD, and a CHECK at every settle into play.
class Driver
{
public:
	HeadlessApp*	mApp;
	Board*			mBoard;
	Replay::Segment	mSegment;
	bool			mRecording;
	int				mLastState;
	int				mLostTargets = 0;   // moves whose board left play between SELECT and TARGET (none: ReadyToMove)

	Driver(const Replay::Segment& theSegment, bool isRecording) : mSegment(theSegment), mRecording(isRecording)
	{
		mSegment.mEvents.clear();
		bool isTimed = (mSegment.mMode != 0);
		mApp = new HeadlessApp();
		mApp->SetPointsAscent(mSegment.mPointsAscent);
		mApp->SetTimedMode(isTimed, false);
		mApp->mShowInstructions = mSegment.mShowInstructions;
		mApp->mTimesPlayed = mSegment.mTimesPlayed;
		gGameConfig.mNextBoardSeed = 0;
		gGameConfig.mNextBoardUserSeeded = false;
		gGameConfig.mNextBoardPlayback = &mSegment;
		mBoard = new Board(mApp, isTimed);
		mApp->mBoard = mBoard;
		mBoard->Resize(0, 0, 640, 480);
		mLastState = mBoard->mState;
	}

	~Driver()
	{
		delete mApp; // and the board
	}

	void Add(int theOp, int64_t theArg0 = 0, int64_t theArg1 = 0)
	{
		if (!mRecording)
			return;
		Replay::Event anEvent;
		anEvent.mTick = (uint32_t) mBoard->mUpdateCnt;
		anEvent.mOp = theOp;
		anEvent.mArgs[0] = theArg0;
		anEvent.mArgs[1] = theArg1;
		mSegment.mEvents.push_back(anEvent);
	}

	// One update as Step makes it: the ENV for the tick-150 block, then the CHECK of a settle into play
	void Update()
	{
		if (mBoard->mUpdateCnt == 149)
		{
			mApp->mShowInstructions = mSegment.mShowInstructions;
			mApp->mTimesPlayed = mSegment.mTimesPlayed;
		}
		mBoard->Update();
		int aState = mBoard->mState;
		if ((aState == STATE_PLAYING) && (mLastState != STATE_PLAYING))
			Add(Replay::OP_CHECK, (int64_t) StateCodec::LogicHash(mBoard));
		mLastState = aState;
	}

	void Wait(int theTicks)
	{
		for (int i = 0; i < theTicks; i++)
			Update();
	}

	// In play, and MouseDown's gate open (Apply's SELECT)
	bool CanSelect()
	{
		Board* b = mBoard;
		return (b->mState == STATE_PLAYING) && (b->mUserPauseCount == 0) && (b->mPauseCount == 0) && (b->mInputDelay == 0) &&
			(b->mSelectedPiece == NULL);
	}

	// Updates until the board takes a selection (after a cascade, a level-up's vortex, wipe and fall); false at the
	// game over, or no play within SETTLE_CAP
	bool RunToPlay()
	{
		for (int aTick = 0; aTick <= SETTLE_CAP; aTick++)
		{
			int aState = mBoard->mState;
			if ((aState == STATE_GAME_OVER) || (aState == STATE_GAME_OVER_DONE))
				return false;
			if (CanSelect())
				return true;
			Update();
		}
		return false;
	}

	// In play after theWait updates, the board taking a selection. A beaten level is played no more: the vortex starts
	// once the score shown has counted up to the target, in play, and a move made then would have its TARGET land in
	// the vortex, where the game takes none. False at the game over.
	bool ReadyToMove(int theWait)
	{
		if (!RunToPlay())
			return false;
		Board* b = mBoard;
		for (int i = 0; (i < SETTLE_CAP) && (b->mState == STATE_PLAYING) && (b->mScore >= b->mLevelTargetScore); i++)
			Update();
		if (!RunToPlay())
			return false;
		Wait(theWait);
		return RunToPlay();
	}

	// SELECT, an update, TARGET (Apply's): the board starts the swap
	void StartMove(const Move& theMove)
	{
		Board* b = mBoard;
		Add(Replay::OP_SELECT, theMove.mCol1, theMove.mRow1);
		b->mPlaybackApplying = true;
		b->mCursorFrame = 0;
		b->mSelectedPiece = b->mBoard[theMove.mRow1][theMove.mCol1];
		b->MarkDirty();
		b->mPlaybackApplying = false;
		Update();
		if (b->mState != STATE_PLAYING)
		{
			mLostTargets++;
			return;
		}
		Add(Replay::OP_TARGET, theMove.mCol2, theMove.mRow2);
		b->mPlaybackApplying = true;
		Piece* aTarget = b->mBoard[theMove.mRow2][theMove.mCol2];
		if ((aTarget != NULL) && (aTarget != b->mSelectedPiece))
			b->StartSwap(b->mSelectedPiece, aTarget);
		b->mSelectedPiece = NULL;
		b->MarkDirty();
		b->mPlaybackApplying = false;
	}

	// Updates to the swap's first settle
	Outcome RunMove()
	{
		Outcome anOutcome;
		Board* b = mBoard;
		anOutcome.mLevel = b->mDisplayLevel;
		for (int aTick = 1; aTick <= SETTLE_CAP; aTick++)
		{
			Update();
			if (IsSettled(b->mState))
			{
				anOutcome.mState = b->mState;
				break;
			}
		}
		anOutcome.mPoints = b->mMoveScore;
		anOutcome.mRounds = b->mCascadeCount - 1;
		anOutcome.mRuns = b->mMoveMatchCount;
		anOutcome.mBeaten = (b->mScore >= b->mLevelTargetScore) || (b->mState == STATE_BLACK_HOLE);
		int aMove[4];
		while ((b->mState == STATE_PLAYING) && b->FindMove(aMove, anOutcome.mMovesLeft, true, true))
			anOutcome.mMovesLeft++;
		if (b->mState == STATE_BLACK_HOLE)
			anOutcome.mMovesLeft = SAFE_MOVES; // a new level's board
		return anOutcome;
	}

	Outcome PlayMove(const Move& theMove)
	{
		StartMove(theMove);
		return RunMove();
	}

	// Apply's LOAD at tick 0: the segment's save, then its RNGS
	bool Load(std::string* theError)
	{
		Board* b = mBoard;
		Add(Replay::OP_LOAD);
		Buffer aSave;
		aSave.WriteBytes((const uchar*) mSegment.mLoadSave.data(), (int) mSegment.mLoadSave.size());
		b->mPlaybackApplying = true;
		b->mPlaybackLoad = &aSave;
		bool isLoaded = b->LoadGame();
		b->mPlaybackLoad = NULL;
		bool isSet = isLoaded && b->SetPlaybackRng(mSegment.mLoadRng);
		b->mPlaybackApplying = false;
		if (!isSet)
			*theError = isLoaded ? "the RNGS does not set" : "the save does not load";
		return isSet;
	}

	// The replay of this play, ended now
	std::string File(int theReason)
	{
		Replay::File aFile;
		aFile.mHasRules = true;
		aFile.mRules = mBoard->mRules.Canonical();
		aFile.mHasRulesId = true;
		mBoard->mRules.RulesId(aFile.mRulesId);
		aFile.mSegments.push_back(mSegment);
		aFile.mEnd = mBoard->GetEndInfo(theReason);
		return Replay::Write(aFile);
	}
};

struct Trial
{
	Move	mMove;
	Outcome	mOutcome;
};

// The moves of the board in play, each pair once, in FindMove's order
std::vector<Move> UniqueMoves(Board* b)
{
	std::vector<Move> aMoves;
	int aFound[4];
	for (int i = 0; b->FindMove(aFound, i, true, true); i++)
	{
		Move aMove;
		aMove.mCol1 = aFound[0];
		aMove.mRow1 = aFound[1];
		aMove.mCol2 = aFound[2];
		aMove.mRow2 = aFound[3];
		bool isSeen = false;
		for (const Move& m : aMoves)
		{
			if (((m.mCol1 == aMove.mCol1) && (m.mRow1 == aMove.mRow1) && (m.mCol2 == aMove.mCol2) && (m.mRow2 == aMove.mRow2)) ||
				((m.mCol1 == aMove.mCol2) && (m.mRow1 == aMove.mRow2) && (m.mCol2 == aMove.mCol1) && (m.mRow2 == aMove.mRow1)))
				isSeen = true;
		}
		if (!isSeen)
			aMoves.push_back(aMove);
	}
	return aMoves;
}

// theScratch's board to theSnapshot (of theMain's board or its own), with theMain's app fields
bool Restore(Driver* theMain, Driver* theScratch, const std::string& theSnapshot, std::string* theError)
{
	if (!StateCodec::Decode(theScratch->mBoard, theSnapshot, theError))
		return false;
	theScratch->mApp->mShowInstructions = theMain->mApp->mShowInstructions;
	theScratch->mApp->mTimesPlayed = theMain->mApp->mTimesPlayed;
	theScratch->mLastState = theScratch->mBoard->mState;
	return true;
}

// The moves of the board in play (each pair once), each tried on theScratch from theMain's snapshot
bool TryMoves(Driver* theMain, Driver* theScratch, std::vector<Trial>* theTrials, std::string* theError)
{
	theTrials->clear();
	Board* b = theMain->mBoard;
	std::vector<Move> aMoves = UniqueMoves(b);
	std::string aSnapshot = StateCodec::Encode(b, StateCodec::PROFILE_SNAPSHOT);
	for (const Move& aMove : aMoves)
	{
		if (!Restore(theMain, theScratch, aSnapshot, theError))
			return false;
		Trial aTrial;
		aTrial.mMove = aMove;
		aTrial.mOutcome = theScratch->PlayMove(aMove);
		theTrials->push_back(aTrial);
	}
	return true;
}

// One seed's game: the main board (recording or not) and the scratch board the moves are tried on
class Game
{
public:
	Driver				mMain;
	Driver				mScratch;
	Policy				mPolicy;
	int					mWait;
	std::mt19937		mRand;
	int					mMoves = 0;
	int					mMismatches = 0;
	long long			mTrials = 0;
	std::vector<Trial>	mHere;          // the trials of the settle the game is at
	std::string			mError;         // POLICY_SURVIVE: a snapshot that did not restore
	int					mBudget = 0;    // POLICY_SURVIVE: the trials left to the search (FirstSurvivor)
	bool				mOutOfBudget = false;
	int					mNarrow = 0;    // POLICY_SURVIVE: moves chosen on a narrow board (no line keeping 2 moves)
	int					mShort = 0;     // ... of those, with no line of gSurviveDepth at all

	Game(uint32_t theSeed, Policy thePolicy, int theWait, bool isRecording) :
		mMain(SeedSegment(theSeed), isRecording), mScratch(SeedSegment(theSeed), false), mPolicy(thePolicy), mWait(theWait),
		mRand(theSeed ^ 0x9e3779b9u)
	{
	}

	// To the next settle, mWait updates after it, with its moves tried: false at the game over
	bool Settle(std::string* theError)
	{
		if (!mMain.ReadyToMove(mWait))
			return false;
		if (!TryMoves(&mMain, &mScratch, &mHere, theError))
			return false;
		mTrials += (long long) mHere.size();
		return !mHere.empty();
	}

	// POLICY_SURVIVE: whether mScratch's board, settled after a move, has theMinMoves moves or more and a line of theDepth
	// more moves on which every board has as many, each move made mWait updates after its settle as the main board makes
	// it. The refills come from the board's own RNG, so the line is certain, not likely. A beaten level's line goes on
	// through the vortex on the next level's board, which can be a trap too. Past mBudget trials the answer is false and
	// mOutOfBudget set.
	bool Survives(int theDepth, int theMinMoves)
	{
		Board* b = mScratch.mBoard;
		if ((b->mState == STATE_GAME_OVER) || (b->mState == STATE_GAME_OVER_DONE))
			return false;
		// The board to count is the next level's after a beaten one; a board in play has the same moves after the wait
		bool isBeaten = (b->mState == STATE_BLACK_HOLE) || (b->mScore >= b->mLevelTargetScore);
		if ((isBeaten || (theDepth > 0)) && !mScratch.ReadyToMove(mWait))
			return false;
		std::vector<Move> aMoves = UniqueMoves(b);
		if ((int) aMoves.size() < theMinMoves)
			return false;
		if (theDepth == 0)
			return true;
		std::string aSnapshot = StateCodec::Encode(b, StateCodec::PROFILE_SNAPSHOT);
		for (size_t i = 0; i < aMoves.size(); i++)
		{
			if (mBudget <= 0)
			{
				mOutOfBudget = true;
				return false;
			}
			if ((i > 0) && !Restore(&mMain, &mScratch, aSnapshot, &mError))
				return false;
			mTrials++;
			mBudget--;
			mScratch.PlayMove(aMoves[i]);
			if (Survives(theDepth - 1, theMinMoves))
				return true;
		}
		return false;
	}

	// POLICY_SURVIVE: of theOrder's moves (mHere's, by points), the first that passes Survives(theDepth, theMinMoves) from
	// theSnapshot of the main board; -1 for none, -2 when the budget ran out first
	int FirstSurvivor(const std::vector<int>& theOrder, const std::string& theSnapshot, int theDepth, int theMinMoves)
	{
		mBudget = SURVIVE_BUDGET;
		mOutOfBudget = false;
		for (int i : theOrder)
		{
			const Outcome& o = mHere[i].mOutcome;
			if ((o.mState == STATE_GAME_OVER) || (o.mState == STATE_GAME_OVER_DONE) || (o.mState < 0))
				continue;
			if (!Restore(&mMain, &mScratch, theSnapshot, &mError))
				return -1;
			mTrials++;
			mScratch.PlayMove(mHere[i].mMove);
			if (Survives(theDepth, theMinMoves))
				return i;
			if (!mError.empty())
				return -1;
		}
		return mOutOfBudget ? -2 : -1;
	}

	// POLICY_FAIR: mScratch's board to theSnapshot, its RNG reseeded with theSeed: refills the bot draws itself, where the
	// game's own would come from the stream the snapshot holds
	bool RestoreSample(const std::string& theSnapshot, uint32_t theSeed)
	{
		if (!Restore(&mMain, &mScratch, theSnapshot, &mError))
			return false;
		if (mScratch.mBoard->mOwnRng == NULL)
		{
			mError = "the scratch board has no RNG of its own to reseed";
			return false;
		}
		mScratch.mBoard->mOwnRng->SRand(theSeed);
		return true;
	}

	// POLICY_FAIR: how likely theOutcome's board is to end the game: 1 at the game over, 0 for a beaten level (a fresh
	// board), else halved by each move it has
	static double SampleRisk(const Outcome& theOutcome)
	{
		if (theOutcome.mBeaten)
			return 0.0;
		if ((theOutcome.mState == STATE_GAME_OVER) || (theOutcome.mState == STATE_GAME_OVER_DONE) || (theOutcome.mState < 0) ||
			(theOutcome.mMovesLeft <= 0))
			return 1.0;
		return std::pow(0.5, theOutcome.mMovesLeft);
	}

	// POLICY_FAIR: the risk after the bot's best next move from mScratch's board, settled after a move: each of its
	// moves tried on theSeeds' refills
	double NextRisk(const std::vector<uint32_t>& theSeeds)
	{
		Board* b = mScratch.mBoard;
		if (b->mState == STATE_BLACK_HOLE)
			return 0.0;
		if (!mScratch.ReadyToMove(mWait))
			return 1.0;
		std::vector<Move> aMoves = UniqueMoves(b);
		std::string aSnapshot = StateCodec::Encode(b, StateCodec::PROFILE_SNAPSHOT);
		double aBest = 1.0;
		for (size_t j = 0; (j < aMoves.size()) && (aBest > 0.0); j++)
		{
			double aRisk = 0.0;
			for (uint32_t aSeed : theSeeds)
			{
				if (!RestoreSample(aSnapshot, aSeed))
					return 1.0;
				mTrials++;
				aRisk += SampleRisk(mScratch.PlayMove(aMoves[j]));
			}
			aBest = std::min(aBest, aRisk / theSeeds.size());
		}
		return aBest;
	}

	// POLICY_FAIR: the move least likely to end the game, judged from the board in play alone. mHere gives only the
	// moves; its outcomes (the game's own refills) stay unread. Each move is tried on gFairSamples boards whose RNG the
	// bot reseeds from mRand, the same seeds for every move of the choice, and its risk is the mean of SampleRisk on
	// them. When even the least is above FAIR_DEEPER, each move's risk is that after the best next move instead (an
	// expectimax of two moves, the next move's refills guessed afresh). Of the moves within gFairTolerance of the least
	// risk, the one with the most points on the samples
	int ChooseFair()
	{
		std::string aSnapshot = StateCodec::Encode(mMain.mBoard, StateCodec::PROFILE_SNAPSHOT);
		std::vector<uint32_t> aSeeds, aNextSeeds;
		for (int k = 0; k < gFairSamples; k++)
			aSeeds.push_back((uint32_t) mRand());
		for (int k = 0; k < FAIR_NEXT_SAMPLES; k++)
			aNextSeeds.push_back((uint32_t) mRand());
		int aCount = (int) mHere.size();
		std::vector<double> aPoints(aCount, 0.0), aRisk(aCount, 0.0);
		for (int i = 0; i < aCount; i++)
		{
			for (uint32_t aSeed : aSeeds)
			{
				if (!RestoreSample(aSnapshot, aSeed))
					return 0;
				mTrials++;
				Outcome o = mScratch.PlayMove(mHere[i].mMove);
				aPoints[i] += o.mPoints;
				aRisk[i] += SampleRisk(o);
			}
			aRisk[i] /= aSeeds.size();
		}
		double aLeast = *std::min_element(aRisk.begin(), aRisk.end());
		if (aLeast > FAIR_DEEPER)
		{
			mNarrow++;
			for (int i = 0; i < aCount; i++)
			{
				if (aRisk[i] >= 1.0)
					continue;
				double aDeeper = 0.0;
				for (uint32_t aSeed : aSeeds)
				{
					if (!RestoreSample(aSnapshot, aSeed))
						return 0;
					mTrials++;
					double aNow = SampleRisk(mScratch.PlayMove(mHere[i].mMove));
					aDeeper += ((aNow >= 1.0) || (aNow <= 0.0)) ? aNow : NextRisk(aNextSeeds);
				}
				aRisk[i] = aDeeper / aSeeds.size();
			}
			aLeast = *std::min_element(aRisk.begin(), aRisk.end());
		}
		int aBest = -1;
		for (int i = 0; i < aCount; i++)
		{
			if ((aRisk[i] <= aLeast + gFairTolerance) && ((aBest < 0) || (aPoints[i] > aPoints[aBest])))
				aBest = i;
		}
		return aBest;
	}

	int Choose()
	{
		if (mPolicy == POLICY_FAIR)
			return ChooseFair();
		std::vector<int> aSafe;
		int aMost = 0;
		for (size_t i = 0; i < mHere.size(); i++)
		{
			if (mHere[i].mOutcome.mMovesLeft >= SAFE_MOVES)
				aSafe.push_back((int) i);
			if (mHere[i].mOutcome.mMovesLeft > mHere[aMost].mOutcome.mMovesLeft)
				aMost = (int) i;
		}
		if (mPolicy == POLICY_SURVIVE)
		{
			// The moves by points (the first of equals first). First the best that keeps SAFE_MOVES, then 2, on every
			// board of a line of gSurviveDepth; on a narrow board the one with the longest line that never runs out (to
			// three times the depth); then shorter lines; else as greedy
			std::vector<int> anOrder;
			for (size_t i = 0; i < mHere.size(); i++)
				anOrder.push_back((int) i);
			std::stable_sort(anOrder.begin(), anOrder.end(), [this](int x, int y) { return mHere[x].mOutcome.mPoints > mHere[y].mOutcome.mPoints; });
			std::string aSnapshot = StateCodec::Encode(mMain.mBoard, StateCodec::PROFILE_SNAPSHOT);
			for (int aMinMoves = SAFE_MOVES; aMinMoves >= 2; aMinMoves--)
			{
				int i = FirstSurvivor(anOrder, aSnapshot, gSurviveDepth, aMinMoves);
				if (i >= 0)
					return i;
			}
			mNarrow++;
			int aLongest = -1;
			for (int aDepth = gSurviveDepth; aDepth <= 3 * gSurviveDepth; aDepth += gSurviveDepth)
			{
				int i = FirstSurvivor(anOrder, aSnapshot, aDepth, 1);
				if (i < 0)
					break;
				aLongest = i;
			}
			if (aLongest >= 0)
				return aLongest;
			mShort++;
			for (int aDepth = gSurviveDepth - 1; aDepth >= 1; aDepth--)
			{
				int i = FirstSurvivor(anOrder, aSnapshot, aDepth, 1);
				if (i >= 0)
					return i;
			}
			return aMost;
		}
		if (aSafe.empty())
			return aMost;
		if (mPolicy == POLICY_RANDOM)
			return aSafe[mRand() % aSafe.size()];
		int aBest = aSafe[0];
		for (int i : aSafe)
		{
			if (mHere[i].mOutcome.mPoints > mHere[aBest].mOutcome.mPoints)
				aBest = i;
		}
		return aBest;
	}

	// Plays a move on the main board: its outcome against the trial's when it was tried
	Outcome Play(const Move& theMove)
	{
		Outcome anOutcome = mMain.PlayMove(theMove);
		for (const Trial& t : mHere)
		{
			if ((t.mMove.mCol1 == theMove.mCol1) && (t.mMove.mRow1 == theMove.mRow1) && (t.mMove.mCol2 == theMove.mCol2) &&
				(t.mMove.mRow2 == theMove.mRow2) && !t.mOutcome.Same(anOutcome))
				mMismatches++;
		}
		mMismatches += mMain.mLostTargets + mScratch.mLostTargets;
		mMain.mLostTargets = 0;
		mScratch.mLostTargets = 0;
		mMoves++;
		return anOutcome;
	}
};

struct Record
{
	uint32_t	mSeed = 0;
	int			mAt = 0;            // the moves played before it
	Move		mMove;
	Outcome		mOutcome;
	int			mScoreBefore = 0;
};

std::string RecordText(const Record& theRecord)
{
	return "seed " + std::to_string(theRecord.mSeed) + " --at " + std::to_string(theRecord.mAt) + " --move " + MoveText(theRecord.mMove) +
		", level " + std::to_string(theRecord.mOutcome.mLevel) + ", score " + std::to_string(theRecord.mScoreBefore) + ": " +
		OutcomeText(theRecord.mOutcome);
}

bool ParseRange(const std::string& theText, uint32_t* theFirst, uint32_t* theLast)
{
	size_t aDots = theText.find("..");
	if (aDots == std::string::npos)
	{
		*theFirst = *theLast = (uint32_t) strtoul(theText.c_str(), NULL, 10);
		return *theFirst != 0;
	}
	*theFirst = (uint32_t) strtoul(theText.substr(0, aDots).c_str(), NULL, 10);
	*theLast = (uint32_t) strtoul(theText.substr(aDots + 2).c_str(), NULL, 10);
	return (*theFirst != 0) && (*theLast >= *theFirst);
}

double Seconds(std::chrono::steady_clock::time_point theStart)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - theStart).count();
}

// The share of n out of theTotal with its 95% Wilson interval
std::string Share(long long n, long long theTotal)
{
	if (theTotal <= 0)
		return "-";
	double p = (double) n / theTotal;
	double z = 1.96;
	double d = 1.0 + z * z / theTotal;
	double c = (p + z * z / (2.0 * theTotal)) / d;
	double h = z * std::sqrt(p * (1.0 - p) / theTotal + z * z / (4.0 * theTotal * theTotal)) / d;
	char aText[96];
	snprintf(aText, sizeof(aText), "%.4f%% (95%%: %.4f%%..%.4f%%)", p * 100.0, std::max(0.0, c - h) * 100.0, (c + h) * 100.0);
	return aText;
}

// first: every first move of each seed's opening board
int First(uint32_t theFirst, uint32_t theLast, int theWait, int theTop, bool isVerbose)
{
	auto aStart = std::chrono::steady_clock::now();
	long long aSeeds = 0, aBeaten = 0, aMoves = 0, aBeatingMoves = 0, aTrials = 0;
	std::vector<long long> aBuckets(11, 0);    // the best first move per 100 points, 1000 and more in the last
	std::vector<long long> aRounds(32, 0);     // the most rounds of a board's first moves
	std::vector<Record> aTop;
	int aTarget = 0;
	for (uint32_t aSeed = theFirst; ; aSeed++)
	{
		Game aGame(aSeed, POLICY_GREEDY, theWait, false);
		std::string anError;
		if (!aGame.Settle(&anError))
		{
			printf("seed %u: no first settle%s%s\n", (unsigned) aSeed, anError.empty() ? "" : ": ", anError.c_str());
			return 1;
		}
		aSeeds++;
		aTrials += (long long) aGame.mHere.size();
		aTarget = aGame.mMain.mBoard->mLevelTargetScore;
		int aBest = 0;
		int aMostRounds = 0;
		bool isBeaten = false;
		for (size_t i = 0; i < aGame.mHere.size(); i++)
		{
			const Outcome& o = aGame.mHere[i].mOutcome;
			aMoves++;
			if (o.mBeaten)
			{
				aBeatingMoves++;
				isBeaten = true;
			}
			if (o.mPoints > aGame.mHere[aBest].mOutcome.mPoints)
				aBest = (int) i;
			aMostRounds = std::max(aMostRounds, o.mRounds);
		}
		if (isBeaten)
			aBeaten++;
		const Outcome& aBestOutcome = aGame.mHere[aBest].mOutcome;
		aBuckets[std::min(10, aBestOutcome.mPoints / 100)]++;
		aRounds[std::min(31, aMostRounds)]++;
		Record aRecord;
		aRecord.mSeed = aSeed;
		aRecord.mMove = aGame.mHere[aBest].mMove;
		aRecord.mOutcome = aBestOutcome;
		if (isVerbose || isBeaten)
			printf("seed %u: %d moves, best %s%s\n", (unsigned) aSeed, (int) aGame.mHere.size(), RecordText(aRecord).c_str(),
				isBeaten ? " (BEATS LEVEL 1)" : "");
		aTop.push_back(aRecord);
		std::sort(aTop.begin(), aTop.end(), [](const Record& a, const Record& b) { return a.mOutcome.mPoints > b.mOutcome.mPoints; });
		if ((int) aTop.size() > theTop)
			aTop.resize(theTop);
		if ((aSeeds % 10000) == 0)
		{
			printf("... %lld seeds, %lld beat level 1, %.0f s\n", aSeeds, aBeaten, Seconds(aStart));
			fflush(stdout);
		}
		if (aSeed == theLast)
			break;
	}

	printf("first: seeds %u..%u, %lld boards, %lld first moves (%.2f a board), %lld tried in %.1f s\n", (unsigned) theFirst,
		(unsigned) theLast, aSeeds, aMoves, (double) aMoves / aSeeds, aTrials, Seconds(aStart));
	printf("boards with a first move that beats level 1 (%d points): %lld, %s\n", aTarget, aBeaten, Share(aBeaten, aSeeds).c_str());
	printf("first moves that beat level 1: %lld, %s\n", aBeatingMoves, Share(aBeatingMoves, aMoves).c_str());
	printf("best first move of a board, points:");
	for (int i = 0; i < 11; i++)
		printf(" %s%d: %lld", (i == 10) ? ">=" : "", i * 100, aBuckets[i]);
	printf("\nmost rounds of a board's first moves:");
	for (int i = 1; i < 32; i++)
	{
		if (aRounds[i] > 0)
			printf(" %d: %lld", i, aRounds[i]);
	}
	printf("\ntop first moves:\n");
	for (const Record& r : aTop)
		printf("  %s\n", RecordText(r).c_str());
	return 0;
}

// play: games to the game over or the move cap, the records as they come
bool WriteBytes(const std::string& theName, const std::string& theBytes);
std::string Finish(Driver* theDriver, int theTail);

int Play(uint32_t theFirst, uint32_t theLast, Policy thePolicy, int theWait, int theMaxMoves, int theLevelCap, bool isVerbose)
{
	auto aStart = std::chrono::steady_clock::now();
	long long aTrials = 0, aMoves = 0;
	int aGames = 0, aGameOvers = 0, aMismatches = 0;
	std::vector<long long> aRounds(64, 0);          // every trial's rounds
	Record aMostRounds, aMostPoints, aMostPoints15;
	for (uint32_t aSeed = theFirst; ; aSeed++)
	{
		Game aGame(aSeed, thePolicy, theWait, !gRecord.empty());
		std::string anError;
		bool isOver = false;
		while (aGame.mMoves < theMaxMoves)
		{
			if (!aGame.Settle(&anError))
			{
				isOver = true;
				break;
			}
			Board* b = aGame.mMain.mBoard;
			if (b->mDisplayLevel > theLevelCap)
				break;
			for (const Trial& t : aGame.mHere)
			{
				const Outcome& o = t.mOutcome;
				aRounds[std::min(63, std::max(0, o.mRounds))]++;
				Record aRecord;
				aRecord.mSeed = aSeed;
				aRecord.mAt = aGame.mMoves;
				aRecord.mMove = t.mMove;
				aRecord.mOutcome = o;
				aRecord.mScoreBefore = b->mScore;
				if ((o.mRounds > aMostRounds.mOutcome.mRounds) || ((o.mRounds == aMostRounds.mOutcome.mRounds) &&
					(o.mPoints > aMostRounds.mOutcome.mPoints)))
				{
					aMostRounds = aRecord;
					printf("most rounds: %s\n", RecordText(aRecord).c_str());
					fflush(stdout);
				}
				if (o.mPoints > aMostPoints.mOutcome.mPoints)
				{
					aMostPoints = aRecord;
					printf("most points: %s\n", RecordText(aRecord).c_str());
					fflush(stdout);
				}
				if ((o.mLevel <= 15) && (o.mPoints > aMostPoints15.mOutcome.mPoints))
				{
					aMostPoints15 = aRecord;
					if (o.mPoints >= 10000)
					{
						printf("most points to level 15: %s\n", RecordText(aRecord).c_str());
						fflush(stdout);
					}
				}
			}
			int aChoice = aGame.Choose();
			if (!aGame.mError.empty())
			{
				anError = aGame.mError;
				break;
			}
			aGame.Play(aGame.mHere[aChoice].mMove);
			if ((gProgress > 0) && (aGame.mMoves % gProgress == 0))
			{
				printf("seed %u: move %d, level %d, score %d\n", (unsigned) aSeed, aGame.mMoves, b->mDisplayLevel, b->mScore);
				fflush(stdout);
			}
		}
		if (!anError.empty())
		{
			printf("seed %u: %s\n", (unsigned) aSeed, anError.c_str());
			return 1;
		}
		Board* b = aGame.mMain.mBoard;
		aGames++;
		aGameOvers += isOver ? 1 : 0;
		aTrials += aGame.mTrials;
		aMoves += aGame.mMoves;
		aMismatches += aGame.mMismatches;
		if (isVerbose)
			printf("seed %u: %d moves, level %d, score %d, %s (state %d, %d moves at the end); longest cascade %d, best move %d\n",
				(unsigned) aSeed, aGame.mMoves, b->mDisplayLevel, b->mScore, isOver ? "game over" : "capped", b->mState,
				(int) aGame.mHere.size(), b->mLongestCascade, b->mBestMoveScore);
		if (isVerbose && (thePolicy == POLICY_FAIR))
			printf("  fair: %d moves looked a move further\n", aGame.mNarrow);
		if (isVerbose && (thePolicy == POLICY_SURVIVE))
			printf("  survive: %d moves on a narrow board, %d of them without a line of %d\n", aGame.mNarrow, aGame.mShort, gSurviveDepth);
		if (!gRecord.empty())
		{
			std::string aBytes = Finish(&aGame.mMain, 400);
			std::string aName = gRecord + "-seed" + std::to_string(aSeed) + ".bjr";
			bool isOk = false;
			// VerifyReplay leaves the config at its defaults, without the diff.txt values the next game needs
			GameConfig aConfig = gGameConfig;
			auto aVerifyStart = std::chrono::steady_clock::now();
			std::string aJson = Verifier::VerifyReplay(aBytes, aName, false, false, &isOk);
			double aVerifySeconds = Seconds(aVerifyStart);
			gGameConfig = aConfig;
			bool isWritten = WriteBytes(aName, aBytes);
			printf("record: %s, %d bytes%s, verified in %.1f s: %s\n", aName.c_str(), (int) aBytes.size(), isWritten ? "" : " (NOT WRITTEN)",
				aVerifySeconds, aJson.c_str());
			if (!isOk || !isWritten)
				aMismatches++;
		}
		fflush(stdout);
		if (aSeed == theLast)
			break;
	}
	double aSeconds = Seconds(aStart);
	printf("play: seeds %u..%u, %d games (%d game overs), %lld moves, %lld tried in %.1f s (%.0f a second), %d trials the move "
		"played differed from\n", (unsigned) theFirst, (unsigned) theLast, aGames, aGameOvers, aMoves, aTrials, aSeconds,
		aTrials / std::max(aSeconds, 0.001), aMismatches);
	printf("rounds of the moves tried:");
	for (int i = 0; i < 64; i++)
	{
		if (aRounds[i] > 0)
			printf(" %d: %lld", i, aRounds[i]);
	}
	printf("\nmost rounds: %s\nmost points: %s\nmost points to level 15: %s\n", RecordText(aMostRounds).c_str(),
		RecordText(aMostPoints).c_str(), RecordText(aMostPoints15).c_str());
	return (aMismatches == 0) ? 0 : 1;
}

bool WriteBytes(const std::string& theName, const std::string& theBytes)
{
	FILE* aFile = fopen(theName.c_str(), "wb");
	if (aFile == NULL)
		return false;
	bool isOk = fwrite(theBytes.data(), 1, theBytes.size(), aFile) == theBytes.size();
	return (fclose(aFile) == 0) && isOk;
}

// After the last move: on to the next settle into play (or the game over), theTail updates, END
std::string Finish(Driver* theDriver, int theTail)
{
	bool isPlaying = theDriver->RunToPlay();
	Board* b = theDriver->mBoard;
	// A beaten level: on while the score shown counts up, through the vortex and the wipe to the next level's settle
	if (isPlaying && (b->mScore >= b->mLevelTargetScore))
	{
		for (int i = 0; (i < SETTLE_CAP) && (b->mState == STATE_PLAYING); i++)
			theDriver->Update();
		isPlaying = theDriver->RunToPlay();
	}
	for (int i = 0; isPlaying && (i < theTail); i++)
		theDriver->Update();
	while (!isPlaying && (b->mState != STATE_GAME_OVER_DONE))
		theDriver->Update();
	return theDriver->File(isPlaying ? Replay::END_ABANDONED : Replay::END_NO_MOVES);
}

// replay: the game of theSeed again to its theAt-th move, then theMove (thePause updates after its settle), then theAfter
// moves of the bot (all of them to the game over when it is below 0); the whole game and the moves from theLead before
// the big one on from a save (LDSV), each verified
int MakeReplays(uint32_t theSeed, Policy thePolicy, int theWait, int theAt, const Move& theMove, int theLead, int thePause, int theTail,
	int theAfter, const std::string& theOut)
{
	Game aGame(theSeed, thePolicy, theWait, true);
	std::string anError;
	std::vector<Move> aPlayed;
	std::vector<Outcome> anOutcomes;
	std::string aSave;
	Replay::RngState aRng;
	int aFrom = std::max(0, theAt - theLead);
	int aStartLevel = 0, aStartScore = 0;
	for (;;)
	{
		aGame.mWait = (aGame.mMoves == theAt) ? thePause : theWait;
		if (!aGame.Settle(&anError))
		{
			printf("replay: the game is over after %d moves%s%s\n", aGame.mMoves, anError.empty() ? "" : ": ", anError.c_str());
			return 1;
		}
		Board* b = aGame.mMain.mBoard;
		if (aGame.mMoves == aFrom)
		{
			Buffer aBuffer;
			b->SaveGameSnapshot(&aBuffer);
			aSave.assign((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
			b->GetRngState(&aRng);
			aStartLevel = b->mDisplayLevel;
			aStartScore = b->mScore;
		}
		if (aGame.mMoves >= aFrom)
			aPlayed.push_back((aGame.mMoves == theAt) ? theMove : aGame.mHere[aGame.Choose()].mMove);
		Move aMove = (aGame.mMoves >= aFrom) ? aPlayed.back() : aGame.mHere[aGame.Choose()].mMove;
		bool isLast = (aGame.mMoves == theAt);
		Outcome anOutcome = aGame.Play(aMove);
		if (aGame.mMoves > aFrom)
			anOutcomes.push_back(anOutcome);
		if (isLast)
			break;
	}
	const Outcome aBig = anOutcomes.back();
	printf("replay: seed %u, move %d (%s) on level %d: %s\n", (unsigned) theSeed, theAt, MoveText(theMove).c_str(), aBig.mLevel,
		OutcomeText(aBig).c_str());
	size_t aBigIndex = aPlayed.size() - 1;
	aGame.mWait = theWait;
	for (int i = 0; (theAfter < 0) || (i < theAfter); i++)
	{
		if (!aGame.Settle(&anError))
		{
			if (!anError.empty())
			{
				printf("replay: after the big move: %s\n", anError.c_str());
				return 1;
			}
			break;
		}
		aPlayed.push_back(aGame.mHere[aGame.Choose()].mMove);
		anOutcomes.push_back(aGame.Play(aPlayed.back()));
	}
	int anAfterPlayed = (int) (aPlayed.size() - aBigIndex - 1);
	if (anAfterPlayed > 0)
		printf("replay: the bot played %d moves after it, to level %d, score %d%s; %d trials the move played differed from\n",
			anAfterPlayed, aGame.mMain.mBoard->mDisplayLevel, aGame.mMain.mBoard->mScore,
			((theAfter < 0) || (anAfterPlayed < theAfter)) ? " (the game over)" : "", aGame.mMismatches);
	std::string aFull = Finish(&aGame.mMain, theTail);

	// The short one: a fresh replay that holds the save it starts from (ReplayRecorder::OnLoad), the moves after it to the
	// big one (not the bot's after it: from a load the draws differ, and with them a level-up's board)
	Replay::Segment aSegment = SeedSegment(theSeed);
	aSegment.mHasLoadSave = true;
	aSegment.mLoadSave = aSave;
	aSegment.mHasLoadRng = true;
	aSegment.mLoadRng = aRng;
	Driver aShort(aSegment, true);
	if (!aShort.Load(&anError))
	{
		printf("replay: the short one: %s\n", anError.c_str());
		return 1;
	}
	int aDiffering = 0;
	for (size_t i = 0; i <= aBigIndex; i++)
	{
		if (!aShort.ReadyToMove((i == aBigIndex) ? thePause : theWait))
		{
			printf("replay: the short one is over before its move %d\n", (int) i);
			return 1;
		}
		Outcome anOutcome = aShort.PlayMove(aPlayed[i]);
		if (!anOutcome.Same(anOutcomes[i]))
		{
			aDiffering++;
			printf("replay: the short one's move %d differs: %s, the game's %s\n", (int) i, OutcomeText(anOutcome).c_str(),
				OutcomeText(anOutcomes[i]).c_str());
		}
	}
	std::string aShortBytes = Finish(&aShort, theTail);

	int aBad = aDiffering;
	const std::pair<const char*, const std::string*> aFiles[] = {{"full", &aFull}, {"short", &aShortBytes}};
	for (const auto& f : aFiles)
	{
		std::string aName = theOut + "-" + f.first + ".bjr";
		bool isOk = false;
		std::string aJson = Verifier::VerifyReplay(*f.second, aName, false, false, &isOk);
		bool isWritten = WriteBytes(aName, *f.second);
		printf("%s: %d bytes%s, %s\n", aName.c_str(), (int) f.second->size(), isWritten ? "" : " (NOT WRITTEN)", aJson.c_str());
		if (!isOk || !isWritten)
			aBad++;
	}
	printf("the short one starts on level %d with score %d, %d moves before the big one\n", aStartLevel, aStartScore,
		(int) aBigIndex);
	if (aGame.mMismatches > 0)
		aBad++;
	return (aBad == 0) ? 0 : 1;
}

}

int Verifier::CascadeHunt(int argc, char** argv)
{
	if (argc < 3)
		return 2;
	std::string aCommand = argv[2];
	uint32_t aFirst = 1, aLast = 1, aSeed = 0;
	Policy aPolicy = POLICY_GREEDY;
	int aWait = 50, aMaxMoves = 1000000, aLevelCap = 1000000, aTop = 10, anAt = -1, aLead = 0, aPause = 200, aTail = 400, anAfter = 0;
	bool isVerbose = false;
	Move aMove;
	bool hasMove = false;
	std::string anOut = "cascade";
	for (int i = 3; i < argc; i++)
	{
		std::string anArg = argv[i];
		bool hasValue = (i + 1 < argc);
		if ((anArg == "--seeds") && hasValue)
		{
			if (!ParseRange(argv[++i], &aFirst, &aLast))
				return 2;
		}
		else if ((anArg == "--seed") && hasValue)
			aSeed = (uint32_t) strtoul(argv[++i], NULL, 10);
		else if ((anArg == "--policy") && hasValue)
		{
			std::string aName = argv[++i];
			if ((aName != "greedy") && (aName != "random") && (aName != "survive") && (aName != "fair"))
				return 2;
			aPolicy = (aName == "random") ? POLICY_RANDOM : (aName == "survive") ? POLICY_SURVIVE :
				(aName == "fair") ? POLICY_FAIR : POLICY_GREEDY;
		}
		else if ((anArg == "--depth") && hasValue)
			gSurviveDepth = std::max(1, atoi(argv[++i]));
		else if ((anArg == "--samples") && hasValue)
			gFairSamples = std::max(1, atoi(argv[++i]));
		else if ((anArg == "--tolerance") && hasValue)
			gFairTolerance = std::max(0.0, atof(argv[++i]));
		else if ((anArg == "--progress") && hasValue)
			gProgress = std::max(0, atoi(argv[++i]));
		else if ((anArg == "--record") && hasValue)
			gRecord = argv[++i];
		else if ((anArg == "--wait") && hasValue)
			aWait = atoi(argv[++i]);
		else if ((anArg == "--moves") && hasValue)
			aMaxMoves = atoi(argv[++i]);
		else if ((anArg == "--levels") && hasValue)
			aLevelCap = atoi(argv[++i]);
		else if ((anArg == "--top") && hasValue)
			aTop = atoi(argv[++i]);
		else if ((anArg == "--at") && hasValue)
			anAt = atoi(argv[++i]);
		else if ((anArg == "--move") && hasValue)
			hasMove = ParseMove(argv[++i], &aMove);
		else if ((anArg == "--lead") && hasValue)
			aLead = atoi(argv[++i]);
		else if ((anArg == "--pause") && hasValue)
			aPause = atoi(argv[++i]);
		else if ((anArg == "--tail") && hasValue)
			aTail = atoi(argv[++i]);
		else if ((anArg == "--after") && hasValue)
		{
			std::string aValue = argv[++i];
			anAfter = (aValue == "all") ? -1 : atoi(aValue.c_str());
		}
		else if ((anArg == "--out") && hasValue)
			anOut = argv[++i];
		else if (anArg == "-v")
			isVerbose = true;
		else
			return 2;
	}
	// A game with no mods, as WatchController plays one: the default rules with the diff.txt values
	gGameConfig = GameConfig();
	gGameConfig.mRules.mDiff = Verifier::OriginalDiffValues();
	gGameConfig.LoadFromRules(gGameConfig.mRules);
	if (aCommand == "first")
		return First(aFirst, aLast, aWait, aTop, isVerbose);
	if (aCommand == "play")
		return Play(aFirst, aLast, aPolicy, aWait, aMaxMoves, aLevelCap, isVerbose);
	if ((aCommand == "replay") && (aSeed != 0) && (anAt >= 0) && hasMove)
		return MakeReplays(aSeed, aPolicy, aWait, anAt, aMove, aLead, aPause, aTail, anAfter, anOut);
	return 2;
}
