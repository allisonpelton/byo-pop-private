// port(variant): bejeweled_verify funlab (Verifier.h): bot games on mods' rules, measured for what makes a game mode
// fun or not (games/bejeweled/docs/GAME_MODES.md). Not part of the original game. Test code: the bots' choices and
// guessed refills come from their own std::mt19937, never from the board's RNG.

#include "Verifier.h"
#include "HeadlessApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "GameRules.h"
#include "MoveSet.h"
#include "StateCodec.h"
#include "Replay.h"
#include "platform/Json.h"
#include "platform/ModSettings.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

using namespace Sexy;

namespace
{

// The points font's ascent in every recording's ENV (CascadeHunt.cpp's)
const int POINTS_ASCENT = 32;
// Updates a move's cascade, or a level-up's vortex, wipe and fall, may take
const int SETTLE_CAP = 20000;
const int TICKS_PER_SECOND = 100;
// A Normal board with this many moves or fewer is in danger (the game ends when none is left)
const int FEW_MOVES = 2;
// The timed bar below this share of its height, or a gold level's time below this share of its limit: in danger
const double LOW_TIME = 0.25;
// An arrow this many gems or fewer from its edge: in danger (0: the next settle steps it off)
const int ARROW_NEAR = 1;
// The skilled bot: the risk it sees in a board with an arrow 0 or 1 gem from its edge
const double ARROW_RISK[ARROW_NEAR + 1] = {0.5, 0.25};
// A move scoring this many times the median of the level's earlier moves is a big moment, after this many moves
const double BIG_FACTOR = 4.0;
const int BIG_AFTER = 5;
// Danger at one of the last this many decision points before the game over: an end the player saw coming
const int WARNING_POINTS = 3;
// The skilled bot: the risk above the least a move may have to be chosen for its points
const double RISK_TOLERANCE = 0.05;

enum Policy
{
	POLICY_SKILLED,     // each move tried on gSamples guessed refills: the least risk, then the most points (and gold)
	POLICY_NOVICE       // any move it has, at random
};
int gSamples = 2;

bool ReadText(const std::string& theName, std::string* theText)
{
	FILE* aFile = fopen(theName.c_str(), "rb");
	if (aFile == NULL)
		return false;
	char aBuf[65536];
	size_t n;
	while ((n = fread(aBuf, 1, sizeof(aBuf), aFile)) > 0)
		theText->append(aBuf, n);
	fclose(aFile);
	return true;
}

// The folder a mod.json is in: the mod's id, as ModManager names it
std::string ModId(const std::string& thePath)
{
	std::string aDir = thePath;
	size_t aSlash = aDir.find_last_of("/\\");
	aDir = (aSlash == std::string::npos) ? std::string(".") : aDir.substr(0, aSlash);
	aSlash = aDir.find_last_of("/\\");
	return (aSlash == std::string::npos) ? aDir : aDir.substr(aSlash + 1);
}

// thePath's "tunables" and "rules" with its settings at theStored's values (the defaults for the rest), as
// ModManager::Apply resolves them (Verifier::RulesIdJson's way). Errors and setting warnings go to theErrors.
bool LoadMod(const std::string& thePath, const Platform::JsonValue& theStored, RulesSource* theSource, std::vector<std::string>* theErrors)
{
	std::string aText, anError;
	Platform::JsonValue aManifest;
	if (!ReadText(thePath, &aText))
	{
		theErrors->push_back(thePath + ": cannot be read");
		return false;
	}
	if (!Platform::ParseJson(aText, aManifest, &anError) || !aManifest.IsObject())
	{
		theErrors->push_back(thePath + ": not a JSON object " + anError);
		return false;
	}
	Platform::JsonValue anEffective = aManifest;
	Platform::ModSettings aSettings;
	aSettings.Parse(aManifest);
	if (aSettings.mPresent)
	{
		aSettings.Analyze(aManifest, Platform::NextGameFilter(), std::vector<std::string>(1, "data/diff.txt"));
		Platform::ModSettingValues aValues;
		std::vector<std::string> aWarnings;
		aSettings.ReadValues(theStored, true, &aValues, &aWarnings);
		std::vector<Platform::ModSettingsError> aLeftOut;
		aSettings.Resolve(aManifest, aValues, &anEffective, &aLeftOut);
		for (const std::string& s : aSettings.mErrors)
			theErrors->push_back(thePath + ": settings: " + s);
		for (const std::string& s : aWarnings)
			theErrors->push_back(thePath + ": setting: " + s);
		for (const Platform::ModSettingsError& e : aLeftOut)
			theErrors->push_back(thePath + ": \"" + e.mKey + "\" left out: " + e.mText);
	}
	theSource->mModId = ModId(thePath);
	theSource->mTunables = anEffective["tunables"];
	theSource->mRules = anEffective["rules"];
	return true;
}

// A -gameseed game's segment (CascadeHunt.cpp's), in either mode
Replay::Segment SeedSegment(uint32_t theSeed, bool isTimed)
{
	Replay::Segment aSegment;
	aSegment.mMode = isTimed ? 1 : 0;
	aSegment.mRng.mKind = Replay::RNG_SEED;
	aSegment.mRng.mSeed = theSeed;
	aSegment.mUserSeeded = true;
	aSegment.mShowInstructions = false;
	aSegment.mTimesPlayed = 5;
	aSegment.mPointsAscent = POINTS_ASCENT;
	return aSegment;
}

bool IsOver(int theState)
{
	return (theState == STATE_GAME_OVER) || (theState == STATE_GAME_OVER_DONE);
}

bool IsSettled(int theState)
{
	return (theState == STATE_PLAYING) || (theState == STATE_BLACK_HOLE) || IsOver(theState);
}

// The level's goal reached: the score in Normal, the score less the drain in Timed (Board::Update's test of the bar, on
// the score shown), every cell gold on a gold board. The vortex starts once the score shown has counted up to it.
bool LevelBeaten(Board* b)
{
	if (b->mVariant.mGold)
		return b->GoldLeft() == 0;
	if (b->mTimedMode)
		return b->mScore - (int) b->mTimeDrain - b->mLevelStartScore >= b->mLevelTargetScore - b->mLevelStartScore;
	return b->mScore >= b->mLevelTargetScore;
}

// What a move did, from the swap (twist, slide) to its first settle
struct Outcome
{
	int		mPoints = 0;        // mMoveScore
	int		mRounds = 0;        // rounds of matches (mCascadeCount - 1)
	int		mState = -1;        // the board's state at that settle; -1: none within SETTLE_CAP
	bool	mBeaten = false;    // the level's target reached (or its gold), or the vortex begun
	int		mMovesLeft = 0;     // the moves of the board after it
	int		mSpecials = 0;      // flame and lightning gems made
	int		mArrows = 0;        // arrows cleared
	int		mGilded = 0;        // cells turned gold
};

// A board made as WatchController::CreateBoard makes one for a -gameseed segment, on an app of its own, its input
// given as the replay player gives it (CascadeHunt.cpp's Driver, with twists and slides, and nothing recorded)
class LabBoard
{
public:
	HeadlessApp*	mApp;
	Board*			mBoard;
	Replay::Segment	mSegment;
	int				mLastState;
	int				mEndFrom = -1;  // the state the game over came from (ReplayRecorder's end reason)

	LabBoard(uint32_t theSeed, bool isTimed) : mSegment(SeedSegment(theSeed, isTimed))
	{
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

	~LabBoard()
	{
		delete mApp; // and the board
	}

	void Update()
	{
		if (mBoard->mUpdateCnt == 149)
		{
			mApp->mShowInstructions = mSegment.mShowInstructions;
			mApp->mTimesPlayed = mSegment.mTimesPlayed;
		}
		mBoard->Update();
		if (IsOver(mBoard->mState) && !IsOver(mLastState))
			mEndFrom = mLastState;
		mLastState = mBoard->mState;
	}

	bool CanSelect()
	{
		Board* b = mBoard;
		return (b->mState == STATE_PLAYING) && (b->mUserPauseCount == 0) && (b->mPauseCount == 0) && (b->mInputDelay == 0) &&
			(b->mSelectedPiece == NULL);
	}

	bool RunToPlay()
	{
		for (int aTick = 0; aTick <= SETTLE_CAP; aTick++)
		{
			if (IsOver(mBoard->mState))
				return false;
			if (CanSelect())
				return true;
			Update();
		}
		return false;
	}

	// In play theWait updates after the settle (the player's look at the board), a beaten level played no more
	// (CascadeHunt.cpp's ReadyToMove). False at the game over.
	bool ReadyToMove(int theWait)
	{
		if (!RunToPlay())
			return false;
		Board* b = mBoard;
		for (int i = 0; (i < SETTLE_CAP) && (b->mState == STATE_PLAYING) && LevelBeaten(b); i++)
			Update();
		if (!RunToPlay())
			return false;
		for (int i = 0; i < theWait; i++)
			Update();
		return RunToPlay();
	}

	// The move as the replay player applies it: SELECT, an update, TARGET for a swap; TWIST and SLIDE at once
	void StartMove(const MoveSet::Move& theMove)
	{
		Board* b = mBoard;
		if (theMove.mType == GameRules::MOVE_TWIST)
		{
			b->mPlaybackApplying = true;
			b->StartTwist(theMove.mA, theMove.mB, theMove.mC);
			b->mPlaybackApplying = false;
			return;
		}
		if (theMove.mType == GameRules::MOVE_SLIDE)
		{
			b->mPlaybackApplying = true;
			b->StartSlide(theMove.mA, theMove.mB, theMove.mC, theMove.mE, 0);
			b->mPlaybackApplying = false;
			return;
		}
		b->mPlaybackApplying = true;
		b->mCursorFrame = 0;
		b->mSelectedPiece = b->mBoard[theMove.mB][theMove.mA];
		b->MarkDirty();
		b->mPlaybackApplying = false;
		Update();
		if (b->mState != STATE_PLAYING)
			return;
		b->mPlaybackApplying = true;
		Piece* aTarget = b->mBoard[theMove.mD][theMove.mC];
		if ((aTarget != NULL) && (aTarget != b->mSelectedPiece))
			b->StartSwap(b->mSelectedPiece, aTarget);
		b->mSelectedPiece = NULL;
		b->MarkDirty();
		b->mPlaybackApplying = false;
	}

	Outcome PlayMove(const MoveSet::Move& theMove);
};

// The moves of the board in play, a swap once (not from both of its cells), in the search's order
std::vector<MoveSet::Move> BoardMoves(Board* b)
{
	std::vector<MoveSet::Move> aMoves;
	auto anAdd = [&aMoves](const MoveSet::Move& m)
	{
		for (const MoveSet::Move& o : aMoves)
		{
			if ((o.mType == m.mType) && (o.mA == m.mA) && (o.mB == m.mB) && (o.mC == m.mC) && (o.mD == m.mD) && (o.mE == m.mE))
				return;
			if ((m.mType == GameRules::MOVE_SWAP) && (o.mType == m.mType) && (o.mA == m.mC) && (o.mB == m.mD) && (o.mC == m.mA) &&
				(o.mD == m.mB))
				return;
		}
		aMoves.push_back(m);
	};
	if (b->mVariant.mSearch)
	{
		std::vector<MoveSet::Found> aFound;
		b->FindVariantMoves(&aFound);
		for (const MoveSet::Found& f : aFound)
			anAdd(f.mMove);
		return aMoves;
	}
	int aFound[4];
	for (int i = 0; b->FindMove(aFound, i, true, true); i++)
	{
		MoveSet::Move m;
		m.mA = aFound[0];
		m.mB = aFound[1];
		m.mC = aFound[2];
		m.mD = aFound[3];
		anAdd(m);
	}
	return aMoves;
}

int SpecialsMade(Board* b)
{
	return b->mSpecialsMade[1] + b->mSpecialsMade[2];
}

Outcome LabBoard::PlayMove(const MoveSet::Move& theMove)
{
	Board* b = mBoard;
	int aSpecials = SpecialsMade(b);
	int anArrows = b->mArrowsCleared;
	int aGoldLeft = b->mVariant.mGold ? b->GoldLeft() : 0;
	int aLevel = b->mLevel;
	StartMove(theMove);
	Outcome anOutcome;
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
	anOutcome.mSpecials = SpecialsMade(b) - aSpecials;
	anOutcome.mArrows = b->mArrowsCleared - anArrows;
	anOutcome.mGilded = (b->mVariant.mGold && (b->mLevel == aLevel)) ? aGoldLeft - b->GoldLeft() : 0;
	anOutcome.mBeaten = (b->mState == STATE_BLACK_HOLE) || (b->mLevel != aLevel) || LevelBeaten(b);
	anOutcome.mMovesLeft = (b->mState == STATE_PLAYING) ? (int) BoardMoves(b).size() : 0;
	return anOutcome;
}

// The timed bar's height, 0 (time up) to 1 (the level beaten): Board::Update's tests on the score shown, here the score
double TimedBar(Board* b)
{
	int aSpan = b->mLevelTargetScore - b->mLevelStartScore;
	if (aSpan <= 0)
		return 1.0;
	double aProgress = (double) (b->mScore - (int) b->mTimeDrain - b->mLevelStartScore);
	return std::max(0.0, std::min(1.0, (aProgress + aSpan) / (2.0 * aSpan)));
}

// The arrows' least distance to their edges, -1 for no arrow
int NearestArrow(Board* b)
{
	int aLeast = -1;
	for (int r = 0; r < b->mRows; r++)
	{
		for (int c = 0; c < b->mCols; c++)
		{
			Piece* p = b->mBoard[r][c];
			if ((p == NULL) || (p->mArrow == 0))
				continue;
			int d = b->ArrowDistance(c, r, p->mArrow - 1);
			if ((aLeast < 0) || (d < aLeast))
				aLeast = d;
		}
	}
	return aLeast;
}

// How likely an outcome's board is to end the game (CascadeHunt.cpp's SampleRisk): 1 at the game over, 0 for a
// beaten level, else halved by each move it has; a board in play with no moves that goes on (noMoves "continue") is 0.5.
// An arrow left at its edge (or a gem from it) counts as the player sees it: one to deal with now (ARROW_RISK)
double Risk(Board* b, const Outcome& o)
{
	if (o.mBeaten)
		return 0.0;
	if (IsOver(o.mState) || (o.mState < 0))
		return 1.0;
	double aRisk = (o.mMovesLeft <= 0) ? ((o.mState == STATE_PLAYING) ? 0.5 : 1.0) : std::pow(0.5, o.mMovesLeft);
	int anArrow = b->mVariant.mArrows ? NearestArrow(b) : -1;
	if ((anArrow >= 0) && (anArrow <= ARROW_NEAR))
		aRisk = std::max(aRisk, ARROW_RISK[anArrow]);
	return aRisk;
}

// One seed's game in one mode by one bot, and what it measured
struct GameLog
{
	int					mMoves = 0;
	int					mTicks = 0;
	int					mLevel = 1;
	int					mScore = 0;
	std::string			mEnd = "capped";
	std::vector<int>	mChoices;           // the moves the board had at each decision point
	int					mForced = 0;        // decision points with FEW_MOVES or fewer (in Normal)
	int					mDanger = 0;        // decision points in danger (FEW_MOVES, LOW_TIME, ARROW_NEAR)
	int					mCloseCalls = 0;    // dangers the game came out of
	int					mCascades = 0;      // moves with 2 or more rounds
	int					mBigCascades = 0;   // 4 or more
	long long			mRounds = 0;
	int					mBig = 0;           // big moments (BIG_FACTOR)
	int					mEvents = 0;        // flame and lightning gems made, arrows cleared
	long long			mPoints = 0;
	std::vector<int>	mLevelMoves;        // the moves of each level beaten
	bool				mWarned = false;    // the game over came after danger at one of the last WARNING_POINTS
	std::vector<bool>	mDangerTrail;
};

class LabGame
{
public:
	LabBoard		mMain;
	LabBoard		mScratch;
	Policy			mPolicy;
	int				mWait;
	bool			mTimed;
	std::mt19937	mRand;
	long long		mTrials = 0;
	std::string		mError;

	LabGame(uint32_t theSeed, bool isTimed, Policy thePolicy, int theWait) :
		mMain(theSeed, isTimed), mScratch(theSeed, isTimed), mPolicy(thePolicy), mWait(theWait), mTimed(isTimed),
		mRand(theSeed ^ 0x51ed270bu ^ (isTimed ? 0x1000u : 0u))
	{
	}

	bool InDanger(int theChoices)
	{
		Board* b = mMain.mBoard;
		if (b->mVariant.mGold && b->GoldTimed() && (b->mGoldTickLimit > 0) && (b->mGoldTicksLeft < LOW_TIME * b->mGoldTickLimit))
			return true;
		if (mTimed && !b->mVariant.mGold && (TimedBar(b) < LOW_TIME))
			return true;
		if (!mTimed && (theChoices <= FEW_MOVES) && !b->mRules.mMoves.mAllowInvalid)
			return true;
		int anArrow = b->mVariant.mArrows ? NearestArrow(b) : -1;
		return (anArrow >= 0) && (anArrow <= ARROW_NEAR);
	}

	// The skilled bot's move: each tried on gSamples boards from the main board's snapshot, their RNG reseeded from
	// mRand (the same seeds for every move), so it knows the move's own matches and guesses the refills
	int ChooseSkilled(const std::vector<MoveSet::Move>& theMoves)
	{
		std::string aSnapshot = StateCodec::Encode(mMain.mBoard, StateCodec::PROFILE_SNAPSHOT);
		std::vector<uint32_t> aSeeds;
		for (int i = 0; i < gSamples; i++)
			aSeeds.push_back((uint32_t) mRand());
		std::vector<double> aValue(theMoves.size(), 0.0), aRisk(theMoves.size(), 0.0);
		for (size_t i = 0; i < theMoves.size(); i++)
		{
			for (uint32_t aSeed : aSeeds)
			{
				if (!StateCodec::Decode(mScratch.mBoard, aSnapshot, &mError))
					return 0;
				mScratch.mApp->mShowInstructions = mMain.mApp->mShowInstructions;
				mScratch.mApp->mTimesPlayed = mMain.mApp->mTimesPlayed;
				mScratch.mLastState = mScratch.mBoard->mState;
				if (mScratch.mBoard->mOwnRng == NULL)
				{
					mError = "the scratch board has no RNG of its own to reseed";
					return 0;
				}
				mScratch.mBoard->mOwnRng->SRand(aSeed);
				mTrials++;
				Outcome o = mScratch.PlayMove(theMoves[i]);
				aValue[i] += o.mPoints + 50.0 * o.mGilded * mMain.mBoard->mPointMultiplier;
				aRisk[i] += Risk(mScratch.mBoard, o);
			}
		}
		double aLeast = *std::min_element(aRisk.begin(), aRisk.end()) / aSeeds.size();
		int aBest = -1;
		for (size_t i = 0; i < theMoves.size(); i++)
		{
			if ((aRisk[i] / aSeeds.size() <= aLeast + RISK_TOLERANCE) && ((aBest < 0) || (aValue[i] > aValue[aBest])))
				aBest = (int) i;
		}
		return aBest;
	}

	// A swap of two neighbouring gems, for a board with no moves that goes on (free swaps)
	bool AnySwap(MoveSet::Move* theMove)
	{
		Board* b = mMain.mBoard;
		for (int aTry = 0; aTry < 200; aTry++)
		{
			int c = (int) (mRand() % b->mCols), r = (int) (mRand() % b->mRows);
			int aDown = (int) (mRand() % 2);
			int c2 = c + (aDown ? 0 : 1), r2 = r + (aDown ? 1 : 0);
			if ((c2 < b->mCols) && (r2 < b->mRows) && (b->mBoard[r][c] != NULL) && (b->mBoard[r2][c2] != NULL))
			{
				theMove->mType = GameRules::MOVE_SWAP;
				theMove->mA = c;
				theMove->mB = r;
				theMove->mC = c2;
				theMove->mD = r2;
				return true;
			}
		}
		return false;
	}

	// The game to its game over or a cap
	GameLog Play(int theMaxMoves, int theMaxTicks)
	{
		GameLog aLog;
		Board* b = mMain.mBoard;
		int aLevel = b->mLevel, aLevelStart = 0;
		std::vector<int> aLevelPoints;
		bool wasDanger = false;
		while ((aLog.mMoves < theMaxMoves) && (b->mUpdateCnt < theMaxTicks))
		{
			if (!mMain.ReadyToMove(mWait))
				break;
			if (b->mLevel != aLevel)
			{
				aLog.mLevelMoves.push_back(aLog.mMoves - aLevelStart);
				aLevelStart = aLog.mMoves;
				aLevel = b->mLevel;
				aLevelPoints.clear();
			}
			std::vector<MoveSet::Move> aMoves = BoardMoves(b);
			int aChoices = (int) aMoves.size();
			aLog.mChoices.push_back(aChoices);
			if (!mTimed && (aChoices <= FEW_MOVES))
				aLog.mForced++;
			bool isDanger = InDanger(aChoices);
			aLog.mDanger += isDanger ? 1 : 0;
			if (wasDanger && !isDanger)
				aLog.mCloseCalls++;
			wasDanger = isDanger;
			aLog.mDangerTrail.push_back(isDanger);
			MoveSet::Move aMove;
			if (aMoves.empty())
			{
				if (!AnySwap(&aMove))
					break;
			}
			else if (mPolicy == POLICY_NOVICE)
				aMove = aMoves[mRand() % aMoves.size()];
			else
			{
				int aChoice = ChooseSkilled(aMoves);
				if (!mError.empty())
					break;
				aMove = aMoves[aChoice];
			}
			Outcome o = mMain.PlayMove(aMove);
			aLog.mMoves++;
			aLog.mPoints += o.mPoints;
			aLog.mRounds += std::max(0, o.mRounds);
			aLog.mCascades += (o.mRounds >= 2) ? 1 : 0;
			aLog.mBigCascades += (o.mRounds >= 4) ? 1 : 0;
			aLog.mEvents += o.mSpecials + o.mArrows;
			if ((int) aLevelPoints.size() >= BIG_AFTER)
			{
				std::vector<int> aSorted = aLevelPoints;
				std::nth_element(aSorted.begin(), aSorted.begin() + aSorted.size() / 2, aSorted.end());
				int aMedian = aSorted[aSorted.size() / 2];
				aLog.mBig += ((aMedian > 0) && (o.mPoints >= BIG_FACTOR * aMedian)) ? 1 : 0;
			}
			aLevelPoints.push_back(o.mPoints);
		}
		aLog.mTicks = b->mUpdateCnt;
		aLog.mLevel = b->mDisplayLevel;
		aLog.mScore = b->mScore;
		if (IsOver(b->mState))
		{
			int aFrom = mMain.mEndFrom;
			aLog.mEnd = (aFrom == STATE_FALLING) ? "no moves" : (aFrom == STATE_ARROW_STEPPING) ? "arrow escaped" :
				b->mVariant.mGold ? "gold time up" : "time up";
			for (int i = std::max(0, (int) aLog.mDangerTrail.size() - WARNING_POINTS); i < (int) aLog.mDangerTrail.size(); i++)
				aLog.mWarned = aLog.mWarned || aLog.mDangerTrail[i];
		}
		return aLog;
	}
};

std::string IntList(const std::vector<int>& theValues)
{
	std::string s = "[";
	for (size_t i = 0; i < theValues.size(); i++)
		s += (i ? "," : "") + std::to_string(theValues[i]);
	return s + "]";
}

std::string GameJson(const char* theMode, const char* thePolicy, uint32_t theSeed, const GameLog& g)
{
	std::vector<int> aChoices = g.mChoices;
	int aMedian = 0;
	if (!aChoices.empty())
	{
		std::nth_element(aChoices.begin(), aChoices.begin() + aChoices.size() / 2, aChoices.end());
		aMedian = aChoices[aChoices.size() / 2];
	}
	char aBuf[1024];
	snprintf(aBuf, sizeof(aBuf), "{\"mode\":\"%s\",\"policy\":\"%s\",\"seed\":%u,\"moves\":%d,\"minutes\":%.3f,\"level\":%d,"
		"\"score\":%d,\"end\":\"%s\",\"decisions\":%d,\"choiceMedian\":%d,\"forced\":%d,\"danger\":%d,\"closeCalls\":%d,"
		"\"cascades\":%d,\"bigCascades\":%d,\"rounds\":%lld,\"big\":%d,\"events\":%d,\"points\":%lld,\"warned\":%s,\"levelMoves\":",
		theMode, thePolicy, (unsigned) theSeed, g.mMoves, g.mTicks / (60.0 * TICKS_PER_SECOND), g.mLevel, g.mScore, g.mEnd.c_str(),
		(int) g.mChoices.size(), aMedian, g.mForced, g.mDanger, g.mCloseCalls, g.mCascades, g.mBigCascades, g.mRounds, g.mBig,
		g.mEvents, g.mPoints, g.mWarned ? "true" : "false");
	return std::string(aBuf) + IntList(g.mLevelMoves) + "}";
}

bool ParseRange(const std::string& theText, uint32_t* theFirst, uint32_t* theLast)
{
	size_t aDots = theText.find("..");
	if (aDots == std::string::npos)
		return false;
	*theFirst = (uint32_t) strtoul(theText.substr(0, aDots).c_str(), NULL, 10);
	*theLast = (uint32_t) strtoul(theText.substr(aDots + 2).c_str(), NULL, 10);
	return (*theFirst != 0) && (*theLast >= *theFirst);
}

}

int Verifier::FunLab(int argc, char** argv)
{
	std::vector<std::string> aMods;
	std::string aSettingsPath;
	uint32_t aFirst = 1, aLast = 20;
	int aModes = MODE_NORMAL | MODE_TIMED, aPolicies = 3, aWait = 100, aMaxMoves = 3000;
	double aMaxMinutes = 30.0;
	for (int i = 2; i < argc; i++)
	{
		std::string anArg = argv[i];
		bool hasValue = (i + 1 < argc);
		if ((anArg == "--settings") && hasValue)
			aSettingsPath = argv[++i];
		else if ((anArg == "--seeds") && hasValue)
		{
			if (!ParseRange(argv[++i], &aFirst, &aLast))
				return 2;
		}
		else if ((anArg == "--mode") && hasValue)
		{
			std::string aName = argv[++i];
			aModes = (aName == "normal") ? MODE_NORMAL : (aName == "timed") ? MODE_TIMED : (aName == "both") ? 3 : 0;
			if (aModes == 0)
				return 2;
		}
		else if ((anArg == "--policy") && hasValue)
		{
			std::string aName = argv[++i];
			aPolicies = (aName == "skilled") ? 1 : (aName == "novice") ? 2 : (aName == "both") ? 3 : 0;
			if (aPolicies == 0)
				return 2;
		}
		else if ((anArg == "--wait") && hasValue)
			aWait = std::max(0, atoi(argv[++i]));
		else if ((anArg == "--samples") && hasValue)
			gSamples = std::max(1, atoi(argv[++i]));
		else if ((anArg == "--moves") && hasValue)
			aMaxMoves = std::max(1, atoi(argv[++i]));
		else if ((anArg == "--minutes") && hasValue)
			aMaxMinutes = std::max(0.1, atof(argv[++i]));
		else if ((anArg.size() > 2) && (anArg[0] != '-'))
			aMods.push_back(anArg);
		else
			return 2;
	}
	// The mods' rules over the defaults with the original diff.txt values, each mod's settings from the settings file
	// (the game's mod-settings.json: {"v":1,"mods":{"<id>":{...}}}), lowest priority first
	Platform::JsonValue aStoredAll;
	std::vector<std::string> anErrors;
	if (!aSettingsPath.empty())
	{
		std::string aText, anError;
		if (!ReadText(aSettingsPath, &aText) || !Platform::ParseJson(aText, aStoredAll, &anError) || !aStoredAll.IsObject())
		{
			fprintf(stderr, "funlab: %s: not a settings file %s\n", aSettingsPath.c_str(), anError.c_str());
			return 1;
		}
	}
	std::vector<RulesSource> aSources;
	for (const std::string& aPath : aMods)
	{
		RulesSource aSource;
		if (LoadMod(aPath, aStoredAll["mods"][ModId(aPath)], &aSource, &anErrors))
			aSources.push_back(aSource);
	}
	GameRules aRules;
	aRules.mDiff = Verifier::OriginalDiffValues();
	RulesStatus aStatus;
	MergeRules(aSources, &aRules, &aStatus);
	for (const RulesMessage& m : aStatus.mErrors)
		anErrors.push_back(m.mModId + ": " + m.mText);
	for (const RulesMessage& m : aStatus.mDropped)
		anErrors.push_back(m.mModId + ": dropped: " + m.mText);
	for (const RulesMessage& m : aStatus.mWarnings)
		printf("warning: %s: %s\n", m.mModId.c_str(), m.mText.c_str());
	if (!anErrors.empty())
	{
		for (const std::string& e : anErrors)
			fprintf(stderr, "funlab: %s\n", e.c_str());
		return 1;
	}
	gGameConfig = GameConfig();
	gGameConfig.mRules = aRules;
	gGameConfig.LoadFromRules(gGameConfig.mRules);
	printf("funlab: rules %s, seeds %u..%u, wait %d, samples %d\n", aRules.RulesIdHex().substr(0, 16).c_str(), (unsigned) aFirst,
		(unsigned) aLast, aWait, gSamples);
	fflush(stdout);
	auto aStart = std::chrono::steady_clock::now();
	long long aTrials = 0;
	int aGames = 0;
	for (int aMode = MODE_NORMAL; aMode <= MODE_TIMED; aMode <<= 1)
	{
		if (!(aModes & aMode))
			continue;
		for (int aPolicy = 0; aPolicy < 2; aPolicy++)
		{
			if (!(aPolicies & (1 << aPolicy)))
				continue;
			for (uint32_t aSeed = aFirst; ; aSeed++)
			{
				LabGame aGame(aSeed, aMode == MODE_TIMED, (Policy) aPolicy, aWait);
				GameLog aLog = aGame.Play(aMaxMoves, (int) (aMaxMinutes * 60 * TICKS_PER_SECOND));
				if (!aGame.mError.empty())
				{
					fprintf(stderr, "funlab: seed %u: %s\n", (unsigned) aSeed, aGame.mError.c_str());
					return 1;
				}
				printf("GAME %s\n", GameJson((aMode == MODE_TIMED) ? "timed" : "normal", aPolicy ? "novice" : "skilled", aSeed, aLog).c_str());
				fflush(stdout);
				aTrials += aGame.mTrials;
				aGames++;
				if (aSeed == aLast)
					break;
			}
		}
	}
	double aSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - aStart).count();
	printf("funlab: %d games, %lld moves tried, in %.1f s\n", aGames, aTrials, aSeconds);
	return 0;
}
