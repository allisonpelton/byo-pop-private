// port(variant): the counts behind the game-over screen's Stats page (GameStats.h). Not part of the original game.

#include "GameStats.h"
#include "Board.h"

#include <algorithm>
#include <cstring>

using namespace Sexy;

GameStats::GameStats()
{
	mLastLevel = -1;
	mLevelStartScore = 0;
	mEnded = false;
	mMovePending = false;
}

GameStats::Level& GameStats::CurrentLevel(Board* theBoard)
{
	if (mLevels.empty() || (mLevels.back().mLevel != theBoard->mLevel))
	{
		Level aLevel;
		aLevel.mLevel = theBoard->mLevel;
		mLevels.push_back(aLevel);
	}
	return mLevels.back();
}

void GameStats::AddPoint(int theTick, int theScore)
{
	if (!mTimeline.empty() && (mTimeline.back().mTick == theTick))
	{
		mTimeline.back().mScore = theScore;
		return;
	}
	if ((int) mTimeline.size() >= MAX_TIMELINE)
	{
		// every other point goes, the first and the newest stay
		std::vector<Point> aKept;
		for (size_t i = 0; i < mTimeline.size(); i += 2)
			aKept.push_back(mTimeline[i]);
		if ((mTimeline.size() % 2) == 0)
			aKept.push_back(mTimeline.back());
		mTimeline.swap(aKept);
	}
	Point aPoint;
	aPoint.mTick = theTick;
	aPoint.mScore = theScore;
	mTimeline.push_back(aPoint);
}

void GameStats::OnRun(int theLength, int theColor, int thePoints)
{
	int aSlot = std::min(std::max(theLength, 3), 5) - 3;
	mRuns[aSlot]++;
	mRunPoints[aSlot] += thePoints;
	if ((theColor >= 0) && (theColor < MAX_COLORS))
		mGems[theColor] += theLength;
}

void GameStats::OnMove(Board* theBoard)
{
	mMoves++;
	CurrentLevel(theBoard).mMoves++;
	mMovePending = true;
}

void GameStats::OnUndone(const GameStats& theSnapshot, int thePenalty)
{
	// The play time and the undo count stay, as the board's do (mUndosUsed: two undos in a row are two); everything else
	// is the snapshot's, so the points follow the score: an undo after an undo takes back the first one's penalty too
	int aTicks = mTicks;
	int anUndos = mUndos;
	int aLevelTicks = mLevels.empty() ? 0 : mLevels.back().mTicks;
	int aLevel = mLevels.empty() ? -1 : mLevels.back().mLevel;
	*this = theSnapshot;
	mTicks = aTicks;
	if (!mLevels.empty() && (mLevels.back().mLevel == aLevel))
		mLevels.back().mTicks = aLevelTicks;
	mUndos = anUndos + 1;
	mUndoPoints += thePenalty;
}

void GameStats::OnSettle(Board* theBoard)
{
	if (!mMovePending)
		return;
	mMovePending = false;
	// mCascadeCount is 1 when the swap lands and grows with each refill, so a move that cleared one set of runs
	// settles at 2; a kept invalid move cleared none
	int aSets = theBoard->mCascadeCount - 1;
	if (aSets >= 1)
	{
		if ((int) mCascades.size() < aSets)
			mCascades.resize(aSets, 0);
		mCascades[aSets - 1]++;
	}
	AddPoint(mTicks, theBoard->mScore);
}

void GameStats::AfterUpdate(Board* theBoard)
{
	if (mLastLevel < 0)
	{
		mLastLevel = theBoard->mLevel;
		mLevelStartScore = theBoard->mScore;
		CurrentLevel(theBoard);
		AddPoint(mTicks, theBoard->mScore);
	}
	if (theBoard->mLevel != mLastLevel)
	{
		// the level that ended keeps the points it gained; the next starts from here
		if (!mLevels.empty())
			mLevels.back().mPoints = theBoard->mScore - mLevelStartScore;
		mLevelUpTicks.push_back(mTicks);
		AddPoint(mTicks, theBoard->mScore);
		mLastLevel = theBoard->mLevel;
		mLevelStartScore = theBoard->mScore;
		CurrentLevel(theBoard);
	}
	if ((theBoard->mState == STATE_GAME_OVER) || (theBoard->mState == STATE_GAME_OVER_DONE))
	{
		if (!mEnded)
		{
			mEnded = true;
			AddPoint(mTicks, theBoard->mScore);
		}
	}
	else if (theBoard->mPauseCount == 0)
	{
		mTicks++;
		CurrentLevel(theBoard).mTicks++;
	}
	CurrentLevel(theBoard).mPoints = theBoard->mScore - mLevelStartScore;
}

void GameStats::OnLoad(Board* theBoard)
{
	mLastLevel = theBoard->mLevel;
	Level& aLevel = CurrentLevel(theBoard);
	mLevelStartScore = theBoard->mScore - aLevel.mPoints;
	mEnded = false;
	mMovePending = false;
	if (mTimeline.empty())
		AddPoint(mTicks, theBoard->mScore);
}

// ---- The STAT record: "GST1", then i32s, each list as a count and its items ----

namespace
{

void PutInt(std::string& theOut, int theValue)
{
	uint32_t aValue = (uint32_t) theValue;
	for (int i = 0; i < 4; i++)
		theOut += (char) ((aValue >> (8 * i)) & 0xff);
}

struct Reader
{
	const std::string&	mBytes;
	size_t				mPos = 0;
	bool				mOk = true;

	explicit Reader(const std::string& theBytes) : mBytes(theBytes) {}

	int Int()
	{
		if (mPos + 4 > mBytes.size())
		{
			mOk = false;
			return 0;
		}
		uint32_t aValue = 0;
		for (int i = 0; i < 4; i++)
			aValue |= (uint32_t) (uint8_t) mBytes[mPos + i] << (8 * i);
		mPos += 4;
		return (int) aValue;
	}

	// A list's length: not negative, and its items fit in what is left
	int Count(int theItemInts)
	{
		int aCount = Int();
		if (!mOk || (aCount < 0) || ((size_t) aCount * theItemInts * 4 > mBytes.size() - mPos))
		{
			mOk = false;
			return 0;
		}
		return aCount;
	}
};

const char kMagic[4] = {'G', 'S', 'T', '1'};

}

std::string GameStats::Encode() const
{
	std::string anOut(kMagic, 4);
	const int aScalars[] = {mMoves, mSwapsBack, mInvalidKept, mInvalidPoints, mHints, mHintPoints, mUndos, mUndoPoints,
		mTicks, mTwists, mSlides, mOtherPoints, mSpecialsMade, mSpecialsSetOff, mGoldCovered};
	for (int aValue : aScalars)
		PutInt(anOut, aValue);
	for (int i = 0; i < 3; i++)
	{
		PutInt(anOut, mRuns[i]);
		PutInt(anOut, mRunPoints[i]);
	}
	PutInt(anOut, MAX_COLORS);
	for (int i = 0; i < MAX_COLORS; i++)
		PutInt(anOut, mGems[i]);
	PutInt(anOut, (int) mCascades.size());
	for (int aCount : mCascades)
		PutInt(anOut, aCount);
	PutInt(anOut, (int) mLevels.size());
	for (const Level& aLevel : mLevels)
	{
		PutInt(anOut, aLevel.mLevel);
		PutInt(anOut, aLevel.mPoints);
		PutInt(anOut, aLevel.mTicks);
		PutInt(anOut, aLevel.mMoves);
	}
	PutInt(anOut, (int) mTimeline.size());
	for (const Point& aPoint : mTimeline)
	{
		PutInt(anOut, aPoint.mTick);
		PutInt(anOut, aPoint.mScore);
	}
	PutInt(anOut, (int) mLevelUpTicks.size());
	for (int aTick : mLevelUpTicks)
		PutInt(anOut, aTick);
	PutInt(anOut, mFromResume ? 1 : 0);
	return anOut;
}

bool GameStats::Decode(const std::string& theBytes)
{
	if ((theBytes.size() < 4) || (memcmp(theBytes.data(), kMagic, 4) != 0))
		return false;
	Reader aReader(theBytes);
	aReader.mPos = 4;
	GameStats aStats;
	int* aScalars[] = {&aStats.mMoves, &aStats.mSwapsBack, &aStats.mInvalidKept, &aStats.mInvalidPoints, &aStats.mHints, &aStats.mHintPoints, &aStats.mUndos,
		&aStats.mUndoPoints, &aStats.mTicks, &aStats.mTwists, &aStats.mSlides, &aStats.mOtherPoints, &aStats.mSpecialsMade, &aStats.mSpecialsSetOff,
		&aStats.mGoldCovered};
	for (int* aValue : aScalars)
		*aValue = aReader.Int();
	for (int i = 0; i < 3; i++)
	{
		aStats.mRuns[i] = aReader.Int();
		aStats.mRunPoints[i] = aReader.Int();
	}
	int aColors = aReader.Count(1);
	for (int i = 0; i < aColors; i++)
	{
		int aGems = aReader.Int();
		if (i < MAX_COLORS)
			aStats.mGems[i] = aGems;
	}
	aStats.mCascades.resize(aReader.Count(1));
	for (int& aCount : aStats.mCascades)
		aCount = aReader.Int();
	aStats.mLevels.resize(aReader.Count(4));
	for (Level& aLevel : aStats.mLevels)
	{
		aLevel.mLevel = aReader.Int();
		aLevel.mPoints = aReader.Int();
		aLevel.mTicks = aReader.Int();
		aLevel.mMoves = aReader.Int();
	}
	aStats.mTimeline.resize(aReader.Count(2));
	for (Point& aPoint : aStats.mTimeline)
	{
		aPoint.mTick = aReader.Int();
		aPoint.mScore = aReader.Int();
	}
	aStats.mLevelUpTicks.resize(aReader.Count(1));
	for (int& aTick : aStats.mLevelUpTicks)
		aTick = aReader.Int();
	aStats.mFromResume = aReader.Int() != 0;
	if (!aReader.mOk || (aReader.mPos != theBytes.size()))
		return false;
	*this = aStats;
	return true;
}
