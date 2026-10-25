// port(variant): the headless verifier's work; see Verifier.h. Not part of the original game.

#include "Verifier.h"
#include "HeadlessApp.h"
#include "DiffBot.h"
#include "Board.h"
#include "BoardTrace.h"
#include "WatchController.h"
#include "GameConfig.h"
#include "GameRules.h"
#include "StateCodec.h"
#include "Replay.h"
#include "Buffer.h"
#include "platform/Json.h"
#include "platform/ModSettings.h"

#include <cstdio>
#include <cstring>
#include <random>
#include <utility>
#include <vector>

using namespace Sexy;

namespace
{

std::string JsonString(const std::string& theText)
{
	std::string anOut = "\"";
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char c = (unsigned char) theText[i];
		if (c == '"')
			anOut += "\\\"";
		else if (c == '\\')
			anOut += "\\\\";
		else if (c == '\n')
			anOut += "\\n";
		else if (c < 0x20)
		{
			char anEscape[8];
			snprintf(anEscape, sizeof(anEscape), "\\u%04x", c);
			anOut += anEscape;
		}
		else
			anOut += (char) c;
	}
	return anOut + "\"";
}

std::string Hex(const uint8_t* theBytes, size_t theSize)
{
	static const char aDigits[] = "0123456789abcdef";
	std::string anOut;
	for (size_t i = 0; i < theSize; i++)
	{
		anOut += aDigits[theBytes[i] >> 4];
		anOut += aDigits[theBytes[i] & 15];
	}
	return anOut;
}

std::string Hex64(uint64_t theValue)
{
	char aText[24];
	snprintf(aText, sizeof(aText), "%016llx", (unsigned long long) theValue);
	return aText;
}

bool StartsWith(const std::string& theText, const char* thePrefix)
{
	return theText.compare(0, strlen(thePrefix), thePrefix) == 0;
}

const char* EndReasonName(int theReason)
{
	static const char* const aNames[] = {"abandoned", "no moves", "time up", "new game", "gold time up", "saved"};
	return ((theReason >= 0) && (theReason <= Replay::END_SAVED)) ? aNames[theReason] : "?";
}

// The player's state the verifier starts from: no mods, no seed, no switches (a replay brings its own rules)
void ResetConfig(bool forceVariant)
{
	gGameConfig = GameConfig();
	gGameConfig.mForceVariant = forceVariant;
}

}

std::string Verifier::VerifyReplay(std::string theBytes, const std::string& theName, bool theTrace, bool forceVariant, bool* isOk)
{
	ResetConfig(forceVariant);
	HeadlessApp* anApp = new HeadlessApp();

	// The game's -replay=<file> -replayquit: the player quits at the end. It reads the file (once: the JSON below
	// takes the file's fields from the player's copy)
	WatchController* aWatch = new WatchController(anApp, theName);
	aWatch->SetBytes(std::move(theBytes));
	anApp->mWatch = aWatch;
	anApp->mReplayQuit = true;
	BoardTracer aTracer;
	aWatch->Start();
	// The points font's ascent, from the ENV, before the first update (the constructor does not read it)
	const Replay::File* aParsed = aWatch->GetFile();
	if ((aParsed != NULL) && !aParsed->mSegments.empty())
		anApp->SetPointsAscent(aParsed->mSegments[0].mPointsAscent);
	if (theTrace)
		aTracer.AfterUpdate(anApp->mBoard, anApp->mUpdateCount);
	anApp->ProcessSafeDeleteList();
	while (!anApp->mShutdown)
	{
		// One frame as in the game: the player's step (WinDMApp::UpdateFrames), the trace, the deferred deletes
		anApp->UpdateFrames();
		if (theTrace)
			aTracer.AfterUpdate(aWatch->IsEndedBoard(anApp->mBoard) ? NULL : anApp->mBoard, anApp->mUpdateCount);
		anApp->ProcessSafeDeleteList();
	}

	const std::string& aMessage = aWatch->GetResult();
	const char* aResult = "unplayable";
	if (StartsWith(aMessage, "REPLAY end: ok, "))
		aResult = "ok";
	else if (StartsWith(aMessage, "REPLAY end: CORRUPT "))
		aResult = "corrupt";
	if (isOk != NULL)
		*isOk = (strcmp(aResult, "ok") == 0);

	std::string aJson = "{\"result\":\"" + std::string(aResult) + "\",\"message\":" + JsonString(aMessage);
	aJson += ",\"checks\":" + std::to_string(aWatch->GetChecks()) + ",\"events\":" + std::to_string(aWatch->GetEventsApplied());
	aJson += ",\"updates\":" + std::to_string(anApp->mUpdateCount);
	if (aParsed != NULL)
	{
		const Replay::File& aFile = *aParsed;
		aJson += ",\"segments\":" + std::to_string(aFile.mSegments.size());
		aJson += ",\"engineAbi\":" + std::to_string(aFile.mEngineAbi);
		aJson += std::string(",\"flags\":{\"hover\":") + (((aFile.mFlags & Replay::FLAG_HOVER) != 0) ? "true" : "false") +
			",\"ranked\":" + (((aFile.mFlags & Replay::FLAG_RANKED) != 0) ? "true" : "false") +
			",\"multiSegment\":" + (((aFile.mFlags & Replay::FLAG_MULTI_SEGMENT) != 0) ? "true" : "false") + "}";
		aJson += ",\"rulesId\":" + (aFile.mHasRulesId ? JsonString(Hex(aFile.mRulesId, sizeof(aFile.mRulesId))) : std::string("null"));
		const Replay::EndInfo& anEnd = aFile.mEnd;
		aJson += ",\"end\":{\"reason\":" + JsonString(EndReasonName(anEnd.mReason)) + ",\"reasonCode\":" + std::to_string(anEnd.mReason) +
			",\"tick\":" + std::to_string(anEnd.mEndTick) + ",\"score\":" + std::to_string(anEnd.mScore) + ",\"level\":" +
			std::to_string(anEnd.mLevel) + ",\"gems\":" + std::to_string(anEnd.mGems) + ",\"draws\":" + std::to_string(anEnd.mBoardDraws) +
			",\"logicHash\":" + JsonString(Hex64(anEnd.mLogicHash)) + "}";
		aJson += ",\"seeds\":[";
		for (size_t s = 0; s < aFile.mSegments.size(); s++)
		{
			const Replay::Segment& aSegment = aFile.mSegments[s];
			if (s > 0)
				aJson += ",";
			aJson += std::string("{\"mode\":") + ((aSegment.mMode != 0) ? "\"timed\"" : "\"normal\"");
			if (aSegment.mRng.mKind == Replay::RNG_SEED)
				aJson += ",\"seed\":" + std::to_string(aSegment.mRng.mSeed) + ",\"userSeeded\":" + (aSegment.mUserSeeded ? "true" : "false");
			else
				aJson += ",\"seed\":null";
			aJson += "}";
		}
		aJson += "]";
	}
	aJson += "}";

	delete anApp; // and the player and its last board
	ResetConfig(false);
	return aJson;
}

namespace
{

// One side of the difftest: an app of its own (a board's tick-150 block and its swaps read and write the app's
// mShowInstructions and mTimesPlayed), its board and its bot
struct Side
{
	HeadlessApp*	mApp;
	Board*			mBoard;
	DiffBot*		mBot;

	Side(uint32_t theSeed, bool isTimed, bool forceVariant)
	{
		mApp = new HeadlessApp();
		mApp->mTimedMode = isTimed;
		// The board owns MTRand(seed), as a game on a per-game seed (Board::InitVariant), so the two never share a
		// stream; -forcevariant leaves the RNG source alone
		ResetConfig(forceVariant);
		gGameConfig.mNextBoardSeed = theSeed;
		gGameConfig.mNextBoardUserSeeded = false;
		mBoard = new Board(mApp, isTimed);
		mBoard->Resize(0, 0, 640, 480);
		mApp->mBoard = mBoard;
		ResetConfig(false);
		mBot = new DiffBot(theSeed);
	}

	~Side()
	{
		delete mBot;
		delete mApp; // and the board (~Board saves a game in progress into the app's memory files)
	}

	bool Ended() { return mApp->mNewGameCalled || mApp->mHighScoresCalled; }
};

std::string MoveList(Board* theBoard)
{
	std::string aList;
	int aMove[4];
	for (int i = 0; theBoard->FindMove(aMove, i, true, true); i++)
		aList += std::to_string(aMove[0]) + "," + std::to_string(aMove[1]) + "," + std::to_string(aMove[2]) + "," + std::to_string(aMove[3]) + " ";
	return aList;
}

std::string SaveBytes(Board* theBoard)
{
	Buffer aBuffer;
	theBoard->SaveGameSnapshot(&aBuffer);
	return std::string((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
}

// The field at which two LOGIC encodings first differ
std::string FirstDifference(Board* thePlain, Board* theVariant)
{
	std::vector<std::pair<size_t, const char*> > aRows;
	std::string a = StateCodec::Encode(thePlain, StateCodec::PROFILE_LOGIC, NULL, false, &aRows);
	std::string b = StateCodec::Encode(theVariant, StateCodec::PROFILE_LOGIC);
	size_t aPos = 0;
	while ((aPos < a.size()) && (aPos < b.size()) && (a[aPos] == b[aPos]))
		aPos++;
	const char* aName = "?";
	for (size_t i = 0; i < aRows.size(); i++)
	{
		if (aRows[i].first <= aPos)
			aName = aRows[i].second;
	}
	return std::string(aName) + " (byte " + std::to_string(aPos) + " of " + std::to_string(a.size()) + "/" + std::to_string(b.size()) + ")";
}

struct ModeTotals
{
	int				mGames = 0;
	int				mDiffering = 0;
	long long		mTicks = 0;
	int				mGameOvers = 0;
	int				mNewGames = 0;
	int				mCapped = 0;
	int				mSettles = 0;
	int				mMoves = 0;
	int				mHints = 0;
	int				mPauses = 0;
	int				mMaxLevel = 0;
	long long		mScores = 0;
};

// One seed in one mode: "" when the two boards played alike, else what differed first
std::string DiffGame(uint32_t theSeed, bool isTimed, int theTicks, ModeTotals* theTotals, std::string* theLine)
{
	Side aPlain(theSeed, isTimed, false);
	Side aVariant(theSeed, isTimed, true);
	Board* a = aPlain.mBoard;
	Board* b = aVariant.mBoard;
	std::string aWhy;
	int aTick = 0;
	int aLastState = -1;
	if (StateCodec::Encode(a, StateCodec::PROFILE_LOGIC) != StateCodec::Encode(b, StateCodec::PROFILE_LOGIC))
		aWhy = "after the constructor: LOGIC differs at " + FirstDifference(a, b);
	for (; aWhy.empty() && (aTick < theTicks); aTick++)
	{
		if (aPlain.Ended() || aVariant.Ended())
		{
			if (aPlain.Ended() != aVariant.Ended())
				aWhy = "one board's game ended, the other's did not";
			break;
		}
		aPlain.mBot->BeforeUpdate(a);
		aVariant.mBot->BeforeUpdate(b);
		a->Update();
		b->Update();
		std::string anA = StateCodec::Encode(a, StateCodec::PROFILE_LOGIC);
		std::string aB = StateCodec::Encode(b, StateCodec::PROFILE_LOGIC);
		if (anA != aB)
		{
			aWhy = "LOGIC differs at " + FirstDifference(a, b) + ": hashes " + Hex64(StateCodec::Fnv1a(anA)) + " and " + Hex64(StateCodec::Fnv1a(aB));
			break;
		}
		if ((a->mState == STATE_PLAYING) && (aLastState != STATE_PLAYING))
		{
			theTotals->mSettles++;
			if (MoveList(a) != MoveList(b))
				aWhy = "the move lists differ: " + MoveList(a) + "/ " + MoveList(b);
			else if (SaveBytes(a) != SaveBytes(b))
				aWhy = "the saves differ";
		}
		aLastState = a->mState;
	}

	theTotals->mGames++;
	theTotals->mTicks += aTick;
	if (aPlain.mApp->mHighScoresCalled)
		theTotals->mGameOvers++;
	else if (aPlain.mApp->mNewGameCalled)
		theTotals->mNewGames++;
	else
		theTotals->mCapped++;
	theTotals->mMoves += aPlain.mBot->mMoves;
	theTotals->mHints += aPlain.mBot->mHints;
	theTotals->mPauses += aPlain.mBot->mPauses;
	if (a->mDisplayLevel > theTotals->mMaxLevel)
		theTotals->mMaxLevel = a->mDisplayLevel;
	theTotals->mScores += a->mScore;
	*theLine = std::string(isTimed ? "timed" : "normal") + " seed " + std::to_string(theSeed) + ": " + std::to_string(aTick) + " updates, board tick " +
		std::to_string(a->mUpdateCnt) + ", state " + std::to_string(a->mState) + ", level " + std::to_string(a->mDisplayLevel) + ", score " +
		std::to_string(a->mScore) + ", " + std::to_string(aPlain.mBot->mMoves) + " moves, LOGIC " +
		Hex64(StateCodec::LogicHash(a)) + (aWhy.empty() ? ": equal" : ": DIFFERS at update " + std::to_string(aTick) + ": " + aWhy);
	return aWhy;
}

}

int Verifier::Difftest(uint32_t theFirst, uint32_t theLast, int theTicks, int theModes, bool isVerbose, std::string* theReport)
{
	// What the exports pass as it comes: the loop below ends only at theLast, from theFirst up
	const char* aWrong = (theFirst == 0) ? "seed 0 (seeds are 1..4294967295)" :
		(theLast < theFirst) ? "the last seed is below the first" :
		(theLast - theFirst >= (uint32_t) MAX_DIFFTEST_SEEDS) ? "more seeds than one call plays (100000)" :
		((theTicks < 1) || (theTicks > (int) Replay::MAX_TICKS)) ? "ticks outside 1..8640000" :
		((theModes < 1) || (theModes > 3)) ? "modes outside 1..3 (normal, timed, both)" : NULL;
	if (aWrong != NULL)
	{
		*theReport += std::string("difftest: ") + aWrong + "\n";
		return -1;
	}
	int aDiffering = 0;
	for (int aMode = 0; aMode < 2; aMode++)
	{
		if ((theModes & ((aMode == 0) ? MODE_NORMAL : MODE_TIMED)) == 0)
			continue;
		bool isTimed = (aMode == 1);
		ModeTotals aTotals;
		for (uint32_t aSeed = theFirst; ; aSeed++)
		{
			std::string aLine;
			std::string aWhy = DiffGame(aSeed, isTimed, theTicks, &aTotals, &aLine);
			if (!aWhy.empty())
			{
				aTotals.mDiffering++;
				aDiffering++;
			}
			if (isVerbose || !aWhy.empty())
				*theReport += "difftest " + aLine + "\n";
			if (aSeed == theLast)
				break;
		}
		char aText[512];
		snprintf(aText, sizeof(aText), "difftest %s: seeds %u..%u: %d games, %d differ; %lld updates, %d settles; ends: %d game over, %d New Game, "
			"%d at the cap of %d; %d moves, %d hints, %d pauses; levels up to %d, mean score %lld\n", isTimed ? "timed" : "normal",
			(unsigned) theFirst, (unsigned) theLast, aTotals.mGames, aTotals.mDiffering, aTotals.mTicks, aTotals.mSettles, aTotals.mGameOvers,
			aTotals.mNewGames, aTotals.mCapped, theTicks, aTotals.mMoves, aTotals.mHints, aTotals.mPauses, aTotals.mMaxLevel,
			(aTotals.mGames > 0) ? aTotals.mScores / aTotals.mGames : 0LL);
		*theReport += aText;
	}
	return aDiffering;
}

namespace
{

// The mirrortest's boards (stage 7c): direction down or up, Twilight's fallAccel (both boards then have the gravity
// rows, so one's snapshot decodes into the other), on the full 8x8, on gMirrorMask or (stage 7b) on gMirrorDirtMask,
// up on its mirror
enum MirrorShape { MIRROR_FULL, MIRROR_MASKED, MIRROR_DIRT, MIRROR_SHAPES };
const char* const gMirrorShapeNames[MIRROR_SHAPES] = {"8x8", "masked", "dirt"};
const char* const gMirrorMask[8] = {"########", "###..###", "########", "#.######", "########", "######.#", "########", "##....##"};
const char* const gMirrorDirtMask[8] = {"#d######", "########", "#####3##", "#.######", "##2#####", "########", "######d#", "###..###"};
const int MIRROR_FALL_ACCEL = 75;

struct MirrorSide
{
	HeadlessApp*	mApp;
	Board*			mBoard;

	MirrorSide(uint32_t theSeed, bool isUp, MirrorShape theShape)
	{
		mApp = new HeadlessApp();
		mApp->mTimedMode = false;
		ResetConfig(false);
		GameRules& aRules = gGameConfig.mRules;
		aRules.mBoard.mGravity = isUp ? GameRules::GRAVITY_UP : GameRules::GRAVITY_DOWN;
		aRules.mBoard.mFallAccel = MIRROR_FALL_ACCEL;
		if (theShape != MIRROR_FULL)
		{
			const char* const* aMask = (theShape == MIRROR_DIRT) ? gMirrorDirtMask : gMirrorMask;
			for (int r = 0; r < 8; r++)
				aRules.mBoard.mMask.push_back(aMask[isUp ? 7 - r : r]);
			aRules.mBoard.mDirtPoints = (theShape == MIRROR_DIRT) ? 25 : 0;
		}
		gGameConfig.mNextBoardSeed = theSeed;
		gGameConfig.mNextBoardUserSeeded = false;
		mBoard = new Board(mApp, false);
		mBoard->Resize(0, 0, 640, 480);
		mApp->mBoard = mBoard;
		ResetConfig(false);
		// No tutorial: its move and arrows are in rows 4..6, which the mirror moves
		mApp->mShowInstructions = false;
		mBoard->mTutorialCol = -1;
	}

	~MirrorSide()
	{
		delete mApp;
	}
};

int MirrorLine(Board* theBoard)
{
	return theBoard->GetRowY(0) + theBoard->GetRowY(theBoard->mRows - 1);
}

// theUp takes theDown's state (StateCodec's SNAPSHOT, the RNG included) with the grid and every gem's y mirrored. Both
// are new boards, so dirt (stage 7b) is each one's mask's, mirrored already: the snapshot leaves its rows out
bool RestoreMirrored(Board* theDown, Board* theUp, std::string* theError)
{
	bool hasDirt = theDown->mDirtRules;
	theDown->mDirtRules = false;
	theUp->mDirtRules = false;
	bool isDecoded = StateCodec::Decode(theUp, StateCodec::Encode(theDown, StateCodec::PROFILE_SNAPSHOT), theError);
	theDown->mDirtRules = hasDirt;
	theUp->mDirtRules = hasDirt;
	if (!isDecoded)
		return false;
	int aRows = theUp->mRows;
	for (int r = 0; r < aRows / 2; r++)
	{
		for (int c = 0; c < theUp->mCols; c++)
			std::swap(theUp->mBoard[r][c], theUp->mBoard[aRows - 1 - r][c]);
	}
	int aMirror = MirrorLine(theUp);
	for (int r = 0; r < aRows; r++)
	{
		for (int c = 0; c < theUp->mCols; c++)
		{
			Piece* aPiece = theUp->mBoard[r][c];
			if (aPiece == NULL)
				continue;
			aPiece->mRow = r;
			aPiece->mY = aMirror - aPiece->mY;
		}
	}
	return true;
}

bool SameRngState(Board* a, Board* b)
{
	Replay::RngState x;
	Replay::RngState y;
	a->GetRngState(&x);
	b->GetRngState(&y);
	return (x.mMti == y.mMti) && (memcmp(x.mMt, y.mMt, sizeof(x.mMt)) == 0) && (a->mBoardDraws == b->mBoardDraws);
}

// "" when theUp is theDown mirrored: every cell's colour, the gems' rows, and (withPositions) every gem's y; the dirt
// layers and the side the next diagonal slide tries first (stage 7b)
std::string MirrorDifference(Board* theDown, Board* theUp, bool withPositions)
{
	int aRows = theDown->mRows;
	int aMirror = MirrorLine(theDown);
	if (theDown->mDiagonalLeftFirst != theUp->mDiagonalLeftFirst)
		return "the next diagonal slide's side";
	for (int r = 0; r < aRows; r++)
	{
		for (int c = 0; c < theDown->mCols; c++)
		{
			Piece* a = theDown->mBoard[r][c];
			Piece* b = theUp->mBoard[aRows - 1 - r][c];
			std::string aCell = "cell (" + std::to_string(c) + "," + std::to_string(r) + ")";
			int aDirt = theDown->mLayout.IsDirt(c, r) ? theDown->mLayout.mDirt[r][c] : 0;
			int aMirrorDirt = theUp->mLayout.IsDirt(c, aRows - 1 - r) ? theUp->mLayout.mDirt[aRows - 1 - r][c] : 0;
			if (aDirt != aMirrorDirt)
				return aCell + ": dirt layers " + std::to_string(aDirt) + " and " + std::to_string(aMirrorDirt);
			if ((a == NULL) != (b == NULL))
				return aCell + ": a gem on one side only";
			if (a == NULL)
				continue;
			if (a->mColor != b->mColor)
				return aCell + ": colours " + std::to_string(a->mColor) + " and " + std::to_string(b->mColor);
			if (b->mRow != aRows - 1 - r)
				return aCell + ": the up gem's row is " + std::to_string(b->mRow);
			if (withPositions && (aMirror - a->mY != b->mY))
				return aCell + ": y " + std::to_string(a->mY) + " and " + std::to_string(b->mY);
		}
	}
	return "";
}

// Unit checks on a masked pair from one RNG state: the upward fill and refill are the mirrors of the downward ones,
// in colours, rows, start positions and draws (on the dirt mask the refill with its diagonal slides, stage 7b)
std::string MirrorFillRefill(uint32_t theSeed, MirrorShape theShape)
{
	MirrorSide aDown(theSeed, false, theShape);
	MirrorSide anUp(theSeed, true, theShape);
	Board* a = aDown.mBoard;
	Board* b = anUp.mBoard;
	std::string anError;
	if (!RestoreMirrored(a, b, &anError))
		return "restore: " + anError;
	a->FillBoardVariant();
	b->FillBoardVariant();
	std::string aWhy = MirrorDifference(a, b, true);
	if (!aWhy.empty())
		return "the fill: " + aWhy;
	if (!SameRngState(a, b))
		return "the fill: the draws differ";
	// The same cells emptied on both sides (mirrored), then one refill
	std::mt19937 aRand(theSeed);
	for (int i = 0; i < 12; i++)
	{
		int c = (int) (aRand() % 8);
		int r = (int) (aRand() % 8);
		if ((a->mBoard[r][c] == NULL) || (a->mBoard[r][c] == a->mSelectedPiece))
			continue;
		delete a->mBoard[r][c];
		a->mBoard[r][c] = NULL;
		delete b->mBoard[7 - r][c];
		b->mBoard[7 - r][c] = NULL;
	}
	if (theShape == MIRROR_DIRT)
	{
		a->RefillDiagonal();
		b->RefillDiagonal();
	}
	else
	{
		a->RefillMasked();
		b->RefillMasked();
	}
	aWhy = MirrorDifference(a, b, true);
	if (!aWhy.empty())
		return "the refill: " + aWhy;
	if (!SameRngState(a, b))
		return "the refill: the draws differ";
	return "";
}

struct MirrorTotals
{
	int				mGames = 0;
	int				mDiffering = 0;
	int				mSettles = 0;
	int				mMoves = 0;
	int				mTickWarnings = 0;
	int				mLevelUps = 0;
	int				mGameOvers = 0;
	int				mCapped = 0;
};

// Updates theBoard to its next settle (STATE_PLAYING reached), or to a state the comparison ends at: the level-up
// (its draws depend on tick alignment) or a game over. The ticks taken, -1 at the cap.
int RunToSettle(Board* theBoard, int theCap)
{
	for (int aTick = 1; aTick <= theCap; aTick++)
	{
		theBoard->Update();
		int aState = theBoard->mState;
		if ((aState == STATE_PLAYING) || (aState >= STATE_GAME_OVER))
			return aTick;
	}
	return -1;
}

// One seed: the down board and its mirror played with mirrored moves; "" when every settle matched
std::string MirrorGame(uint32_t theSeed, MirrorShape theShape, int theMoves, MirrorTotals* theTotals, std::string* theLine)
{
	MirrorSide aDown(theSeed, false, theShape);
	MirrorSide anUp(theSeed, true, theShape);
	Board* a = aDown.mBoard;
	Board* b = anUp.mBoard;
	std::mt19937 aRand(theSeed);
	std::string aWhy;
	std::string anError;
	int aMoves = 0;
	int aWarnings = 0;
	const char* anEnd = "the move cap";
	if (!RestoreMirrored(a, b, &anError))
		aWhy = "restore: " + anError;
	while (aWhy.empty())
	{
		int aTicksA = RunToSettle(a, 20000);
		int aTicksB = RunToSettle(b, 20000);
		if ((aTicksA < 0) || (aTicksB < 0))
		{
			aWhy = "no settle in 20000 ticks";
			break;
		}
		if (a->mState != b->mState)
		{
			aWhy = "states " + std::to_string(a->mState) + " and " + std::to_string(b->mState);
			break;
		}
		if (aTicksA != aTicksB)
			aWarnings++;
		if (a->mState != STATE_PLAYING)
		{
			anEnd = (a->mState == STATE_BLACK_HOLE) ? "a level-up" : "a game over";
			if (a->mState == STATE_BLACK_HOLE)
				theTotals->mLevelUps++;
			else
				theTotals->mGameOvers++;
			break;
		}
		theTotals->mSettles++;
		aWhy = MirrorDifference(a, b, true);
		if (aWhy.empty() && ((a->mScore != b->mScore) || (a->mGemsCleared != b->mGemsCleared) || (a->mLongestCascade != b->mLongestCascade) ||
			(a->mBestMoveScore != b->mBestMoveScore) || (a->mMoveCount != b->mMoveCount)))
			aWhy = "score or stats: score " + std::to_string(a->mScore) + "/" + std::to_string(b->mScore) + ", gems " + std::to_string(a->mGemsCleared) +
				"/" + std::to_string(b->mGemsCleared);
		if (aWhy.empty() && !SameRngState(a, b))
			aWhy = "the draws differ";
		if (!aWhy.empty() || (aMoves >= theMoves))
		{
			if (aWhy.empty())
				theTotals->mCapped++;
			break;
		}
		// A move from the down board's list, mirrored for the up board
		std::vector<std::vector<int> > aList;
		int aMove[4];
		for (int i = 0; a->FindMove(aMove, i, true, true); i++)
			aList.push_back(std::vector<int>(aMove, aMove + 4));
		if (aList.empty())
		{
			aWhy = "a settle in PLAYING with no move";
			break;
		}
		const std::vector<int>& m = aList[aRand() % aList.size()];
		int aRows = a->mRows;
		a->StartSwap(a->mBoard[m[1]][m[0]], a->mBoard[m[3]][m[2]]);
		b->StartSwap(b->mBoard[aRows - 1 - m[1]][m[0]], b->mBoard[aRows - 1 - m[3]][m[2]]);
		aMoves++;
	}
	theTotals->mGames++;
	theTotals->mMoves += aMoves;
	theTotals->mTickWarnings += aWarnings;
	*theLine = std::string(gMirrorShapeNames[theShape]) + " seed " + std::to_string(theSeed) + ": " + std::to_string(aMoves) + " moves, score " +
		std::to_string(a->mScore) + ", ended at " + anEnd + (aWarnings ? ", " + std::to_string(aWarnings) + " settles on other ticks" : "") +
		(aWhy.empty() ? ": mirrored" : ": DIFFERS after move " + std::to_string(aMoves) + ": " + aWhy);
	return aWhy;
}

}

int Verifier::Mirrortest(uint32_t theFirst, uint32_t theLast, int theMoves, bool isVerbose, std::string* theReport)
{
	const char* aWrong = (theFirst == 0) ? "seed 0 (seeds are 1..4294967295)" :
		(theLast < theFirst) ? "the last seed is below the first" :
		(theLast - theFirst >= (uint32_t) MAX_DIFFTEST_SEEDS) ? "more seeds than one call plays (100000)" :
		((theMoves < 1) || (theMoves > 100000)) ? "moves outside 1..100000" : NULL;
	if (aWrong != NULL)
	{
		*theReport += std::string("mirrortest: ") + aWrong + "\n";
		return -1;
	}
#ifdef BEJ_NO_VARIANTS
	// The board never leaves the original paths here, so gravity has no up to mirror (run_tests.sh reads this line)
	*theReport += "mirrortest skipped: a BEJ_NO_VARIANTS build has no gravity\n";
	(void) isVerbose;
	return 0;
#endif
	int aDiffering = 0;
	int aFillDiffering = 0;
	int aFillChecks = 0;
	for (uint32_t aSeed = theFirst; ; aSeed++)
	{
		for (int aShape = MIRROR_MASKED; aShape <= MIRROR_DIRT; aShape++)
		{
			std::string aWhy = MirrorFillRefill(aSeed, (MirrorShape) aShape);
			aFillChecks++;
			if (!aWhy.empty())
			{
				aFillDiffering++;
				*theReport += std::string("mirrortest fill ") + gMirrorShapeNames[aShape] + " seed " + std::to_string(aSeed) + ": DIFFERS: " + aWhy + "\n";
			}
		}
		if (aSeed == theLast)
			break;
	}
	aDiffering += aFillDiffering;
	for (int aShape = 0; aShape < MIRROR_SHAPES; aShape++)
	{
		MirrorTotals aTotals;
		for (uint32_t aSeed = theFirst; ; aSeed++)
		{
			std::string aLine;
			std::string aWhy = MirrorGame(aSeed, (MirrorShape) aShape, theMoves, &aTotals, &aLine);
			if (!aWhy.empty())
			{
				aTotals.mDiffering++;
				aDiffering++;
			}
			if (isVerbose || !aWhy.empty())
				*theReport += "mirrortest " + aLine + "\n";
			if (aSeed == theLast)
				break;
		}
		char aText[512];
		snprintf(aText, sizeof(aText), "mirrortest %s: seeds %u..%u: %d games, %d differ; %d settles, %d moves; ends: %d level-up, %d game over, "
			"%d at the cap of %d moves; %d settles on other ticks (warnings)\n", gMirrorShapeNames[aShape], (unsigned) theFirst, (unsigned) theLast,
			aTotals.mGames, aTotals.mDiffering, aTotals.mSettles, aTotals.mMoves, aTotals.mLevelUps, aTotals.mGameOvers, aTotals.mCapped, theMoves,
			aTotals.mTickWarnings);
		*theReport += aText;
	}
	char aText[160];
	snprintf(aText, sizeof(aText), "mirrortest fill and refill (masked, dirt): %d checks, %d differ\n", aFillChecks, aFillDiffering);
	*theReport += aText;
	return aDiffering;
}

namespace
{

DiffValues OriginalDiff()
{
	DiffValues aDiff;
	aDiff.mNormalPointsBase = 500;
	aDiff.mNormalPointsPerLevel = 150;
	aDiff.mNormalMultBase = 1.0;
	aDiff.mNormalMultPerLevel = 0.5;
	aDiff.mTimedPointsBase = 250;
	aDiff.mTimedPointsPerLevel = 10;
	aDiff.mTimedMultBase = 2.0;
	aDiff.mTimedMultPerLevel = 0.5;
	aDiff.mTimeDrainBase = 1.0;
	aDiff.mTimeDrainPerLevel = 2.0;
	aDiff.mTimeDrainAccelBase = 0.01;
	aDiff.mTimeDrainAccelPerLevel = 0.005;
	return aDiff;
}

std::string MessagesJson(const std::vector<RulesMessage>& theMessages)
{
	std::string anOut = "[";
	for (size_t i = 0; i < theMessages.size(); i++)
		anOut += ((i > 0) ? "," : "") + JsonString(theMessages[i].mModId + ": " + theMessages[i].mText);
	return anOut + "]";
}

}

DiffValues Verifier::OriginalDiffValues()
{
	return OriginalDiff();
}

std::string Verifier::RulesIdJson(const std::string& theModJson)
{
	if (theModJson.size() > (size_t) MAX_MOD_JSON)
		return "{\"error\":\"more than " + std::to_string((int) MAX_MOD_JSON) + " bytes, larger than a mod.json is\"}";
	Platform::JsonValue aManifest;
	std::string anError;
	if (!Platform::ParseJson(theModJson, aManifest, &anError))
		return "{\"error\":" + JsonString("not JSON: " + anError) + "}";
	if (!aManifest.IsObject())
		return "{\"error\":\"not a JSON object\"}";
	// port(mods): a mod with settings plays its rules with every value at its default unless the player changes one,
	// so its "$setting" placeholders are resolved at the defaults first, as ModManager::Apply does
	// (docs/MOD_MENU_DESIGN.md 8.2). Setting errors are errors; a key with a wrong placeholder is left out.
	RulesStatus aStatus;
	Platform::JsonValue anEffective = aManifest;
	Platform::ModSettings aSettings;
	aSettings.Parse(aManifest);
	if (aSettings.mPresent)
	{
		aSettings.Analyze(aManifest, Platform::NextGameFilter(), std::vector<std::string>(1, "data/diff.txt"));
		Platform::ModSettingValues aValues;
		aSettings.ReadValues(Platform::JsonValue(), true, &aValues, NULL);
		std::vector<Platform::ModSettingsError> aLeftOut;
		aSettings.Resolve(aManifest, aValues, &anEffective, &aLeftOut);
		for (const std::string& anError : aSettings.mErrors)
			aStatus.mErrors.push_back({"mod", "settings: " + anError});
		for (const Platform::ModSettingsError& anError : aLeftOut)
		{
			// Analyze reported the placeholder already (as "<key>: <text>"); a key left out for another reason is new
			bool isReported = false;
			for (const std::string& aSettingError : aSettings.mErrors)
				isReported = isReported || (aSettingError == anError.mKey + ": " + anError.mText);
			if (!isReported)
				aStatus.mErrors.push_back({"mod", "\"" + anError.mKey + "\" left out: " + anError.mText});
		}
	}
	RulesSource aSource;
	aSource.mModId = "mod";
	aSource.mTunables = anEffective["tunables"];
	aSource.mRules = anEffective["rules"];
	GameRules aRules;
	aRules.mDiff = OriginalDiff();
	MergeRules(std::vector<RulesSource>(1, aSource), &aRules, &aStatus);
	std::string aJson = "{\"rulesId\":" + JsonString(aRules.RulesIdHex()) + ",\"variant\":" + (aRules.HasVariantRules() ? "true" : "false");
	aJson += ",\"errors\":" + MessagesJson(aStatus.mErrors) + ",\"dropped\":" + MessagesJson(aStatus.mDropped) + ",\"warnings\":" +
		MessagesJson(aStatus.mWarnings) + "}";
	return aJson;
}
