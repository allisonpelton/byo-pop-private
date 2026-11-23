// Sexy::InfernoBoard, the port's IceStormBoard: see the header. Every function says its slot and address in the exe (bejeweled3.exe).
#include "IceStormBoard.h"

#include "Bej3App.h"
#include "Common.h"
#include "EndLevelDialog.h"
#include "Graphics.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// 0x6e0cc0: a cell of the grid is 128 (1200ths); the ice is drawn with its top at the grid's bottom less the position in rows
const int CELL_SIZE = 128;
// 0x6e0cc0: the least a column's push gives way by in each update is -0.2 (a column is never moved down faster)
const float MOVE_LOWEST_STEP = -0.2f;
// 0x6e0cc0 (its crush loop): the crush effect's progress has to reach this before the ice it took is counted, and 1.0 before the column goes
const float CRUSH_PROGRESS_COUNTS = 0.1f;
// 0x6e0cc0: the crush effect's curve ("b;0,1,0.01,.5,...") steps 0.01 over 0.5: it is done 50 updates after it starts
const int CRUSH_DONE_TICKS = 50;
// the last column's index
const int LAST_COLUMN = IceStormBoard::COLUMN_COUNT - 1;
// 0x6c6870: the threshold the bar has when the list of MultiplierIceReq has nothing
const float MULTIPLIER_THRESHOLD_DEFAULT = 10.0f;
// 0x6e5850: the progress and target are the ice and the threshold times this
const double PROGRESS_SCALE = 10000.0;
// the first row of the settings
const int FIRST_STAGE = 0;

// the exe's random_shuffle (0x4215a0, the CRT's): each element from the second on is swapped with one of the elements up to and including itself,
// by the app's Rand()
void ShuffleWithAppRand(std::vector<int>& theValues)
{
	for (size_t i = 1; i < theValues.size(); i++) {
		size_t aOther = (size_t)(Rand() % (int)(i + 1));
		std::swap(theValues[i], theValues[aOther]);
	}
}

// the part of a "Level" setting before its fifth comma, split at its commas, and the curve that follows
bool SplitLevelRow(const std::string& theText, std::vector<std::string>& theFields, std::string& theCurve)
{
	size_t aStart = 0;
	for (int aField = 0; aField < 5; aField++) {
		size_t anEnd = theText.find(',', aStart);
		if (anEnd == std::string::npos) {
			theFields.push_back(theText.substr(aStart));
			aStart = theText.size();
			continue;
		}
		theFields.push_back(theText.substr(aStart, anEnd - aStart));
		aStart = anEnd + 1;
	}
	if (aStart < theText.size())
		theCurve = theText.substr(aStart);
	size_t aFirst = theCurve.find_first_not_of(" \t");
	size_t aLast = theCurve.find_last_not_of(" \t");
	theCurve = aFirst == std::string::npos ? std::string() : theCurve.substr(aFirst, aLast - aFirst + 1);
	return theFields.size() >= 5;
}

}  // namespace

// 0x6e0450's curve ("b;0,1,0.01,1,~###         ~####": 100 updates) and 0x6e5850's ramp after a multiplier ("b+0.2,1,0.01,1,...": from 0.2 to 1)
const char* const IceStormBoard::DEATH_CURVE = "b;0,1,0.01,1,~###         ~####";
const char* const IceStormBoard::RAMP_CURVE = "b+0.2,1,0.01,1,####         UtaHEK~P##";

// 0x6e2d90 (bejeweled3): QuestBoard's constructor (0x6d9000), then the vftable 0x8685ec and the fields from +0x3248: the columns' own constructor
// (0x6e0340, run for each by 0x6e2640) gives each its own index as its owner. port: the PIEffects and the popups the exe makes there are not made
IceStormBoard::IceStormBoard()
{
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		mColumns[aColumn].mOwner = aColumn;
		mColumns[aColumn].mPlannedAnchor = aColumn;
		mColumns[aColumn].mRamp.SetConstant(1.0);   // port: the ramp of a column that has had no multiplier does not slow it (not read from the export)
	}
	mStageCurve.SetConstant(1.0);
}

// slot 85 (0x154), 0x6e2670 (bejeweled3): QuestBoard's reset (0x6cda70), then the mode's state is put to its start: the columns are made again, the
// grace is 0x96, the lists and the sums are empty and the multiplier's pause is off. port: the PIEffects and the popups are not made
void IceStormBoard::ResetGame()
{
	QuestBoard::ResetGame();
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		Column& aState = mColumns[aColumn];
		aState.mFreeze = 0.0;
		aState.mPreviousPosition = 0.0f;
		aState.mPosition = 0.0f;
		aState.mPushStrength = 0.0f;
		aState.mSpeedDeviation = 0.0f;
		aState.mPendingIceRemoved = 0.0f;
		aState.mOwner = aColumn;
		aState.mPlannedAnchor = aColumn;
		aState.mSpan = 1;
		aState.mSpawnDelay = 0;
		aState.mCrackTicks = -1;
		aState.mReprieveDelay = 0;
		aState.mRampDelay = 0;
		aState.mPlannedWidth = 1;
		aState.mReachedHalf = false;
		aState.mActive = false;
		aState.mCrushed = false;
		aState.mCrackSpeedMod = 1.0;
		aState.mCrushedTicks = 0;
		aState.mRamp.SetConstant(1.0);
	}
	mTicks = 0;
	mGraceTicks = GRACE_TICKS;
	mStage = 0;
	mStageStartTick = 0;
	mIceRemoved = 0.0f;
	mPreviousIceRemoved = 0.0f;
	mProgress = 0;
	mProgressTarget = 0;
	mPaused = false;
	mPauseEndTick = 0;
	mSpawnDeadlineTick = SPAWN_DEADLINE_FIRST_TICKS;
	mLoseColumn = -1;
	mVerticalRunPieceIds.clear();
	mColumnPoints = 0;
	mColumnBonusTotal = 0;
	mBestChain = 0;
	mChainCount = 0;
	mChainLastTick = 0;
	mChainWindowTicks = 0;
	mDeathStarted = false;
	mDeathCurve.SetConstant(0.0);
	mSecondsUntilLose = 10;   // 0x6e2670 (+0x3cbc), until NewGame reads the setting
}

// slot 135 (0x21c), 0x6e6a10 (bejeweled3): the settings of the quest are read (the counts with atoi, the rest with wtof; the lists and the curves
// are parsed, the multiplier requirement list is multiplied by 8), the first stage is read and the columns planned, and the board's own new game
// (0x74e410) is made. port: the goal's object, the description and the hypermixers are left out as in the other secret modes
void IceStormBoard::NewGame()
{
	mMatchPushStrength = (float)SettingDouble("MatchPushStr");
	mSpecialGemPushMod = (float)SettingDouble("SpecialGemPushMod");
	mSecondsUntilLose = SettingInt("SecondsUntilLose");
	mDoubleEdgeMult = SettingDouble("DoubleEdgeMult");
	mFireSpeedMult = (float)SettingDouble("FireSpeedMult");
	mColumnDestroyBonus = SettingInt("ColDestroyBonus");
	mMaxRandomFireSpeedColumnDelta = SettingDouble("MaxRandFireSpeedColDelta");
	mDoubleColumnSpeedMult = SettingDouble("DoubleColSpeedMult");
	mFreezeDurationPerNegativeStrength = SettingDouble("FreezeDurationPerNegStrength");
	mFreezeMax = SettingDouble("FreezeMax");
	mStartColumns = SettingInt("StartColCount");
	mColumnCountBonus.clear();
	for (double aBonus : SettingList("ColCountBonus"))
		mColumnCountBonus.push_back((int)aBonus);
	mMultiplierIceReq.clear();
	for (double aRequired : SettingList("MultiplierIceReq"))
		mMultiplierIceReq.push_back((float)(aRequired * 8.0));
	auto aCurveSetting = [this](const char* theName) {
		auto anIter = mSettings.find(theName);
		return anIter == mSettings.end() ? std::string() : anIter->second;
	};
	mRowFireSpeed.SetCurve(aCurveSetting("RowFireSpeed"));
	mColumnDistribution.SetCurve(aCurveSetting("ColDistribution"));
	mReprieveStrengthVsRow.SetCurve(aCurveSetting("ReprieveStrVsRow"));
	mColumnComboCoolDownVsCount.SetCurve(aCurveSetting("ColComboCoolDownVsCount"));
	mLevelRows.clear();
	for (int aLevel = 1; aLevel <= LEVEL_ROWS_MAX; aLevel++) {
		char aName[16];
		snprintf(aName, sizeof(aName), "Level%d", aLevel);
		auto anIter = mSettings.find(aName);
		if (anIter == mSettings.end())
			break;
		std::vector<std::string> aFields;
		LevelRow aRow;
		if (!SplitLevelRow(anIter->second, aFields, aRow.mCurve))
			break;
		aRow.mDoubleColumns = atoi(aFields[0].c_str());
		aRow.mColumnCount = atoi(aFields[1].c_str());
		aRow.mFireSpeed = (float)atof(aFields[2].c_str()) * mFireSpeedMult;
		aRow.mDurationTicks = Ftol(atof(aFields[3].c_str()) * 100.0);
		aRow.mCrackSpeedMod = (float)atof(aFields[4].c_str());
		mLevelRows.push_back(aRow);
	}
	mStage = FIRST_STAGE;
	mPreviousStageFireSpeed = 0.0f;
	PlanLayout();
	mMultiplierIceThreshold = MultiplierThreshold();
	QuestBoard::NewGame();   // 0x6ddc70 (0x6e6a10 calls it): the goal is made, then Board's new game (0x74e410)
}

// 0x6c6600 (bejeweled3): the values of a stage's row. Past the last row they go on by the step between the last two rows (the last row's own
// when there is only one)
IceStormBoard::LevelRow IceStormBoard::StageRow(int theStage) const
{
	const int aLastRow = (int)mLevelRows.size() - 1;
	LevelRow aResult = mLevelRows[std::min(theStage, aLastRow)];
	const int aBeyond = theStage - aLastRow;
	if (aBeyond > 0 && aLastRow > 0) {
		const LevelRow& aBefore = mLevelRows[aLastRow - 1];
		aResult.mColumnCount += aBeyond * (aResult.mColumnCount - aBefore.mColumnCount);
		aResult.mDoubleColumns += aBeyond * (aResult.mDoubleColumns - aBefore.mDoubleColumns);
		aResult.mDurationTicks += aBeyond * (aResult.mDurationTicks - aBefore.mDurationTicks);
		aResult.mFireSpeed += (float)aBeyond * (aResult.mFireSpeed - aBefore.mFireSpeed);
		aResult.mCrackSpeedMod += (float)aBeyond * (aResult.mCrackSpeedMod - aBefore.mCrackSpeedMod);
	}
	return aResult;
}

// 0x6c6600 (bejeweled3): the stage's row is read: the column counts, the stage's length, its fire speed (the row's times FireSpeedMult, kept with
// the one before it, 0 for the first stage) and crack speed, and the curve that takes the fire speed from the last stage's to this one's.
// port: a row with no curve (and every stage past the last row) gives the constant 1, so the fire speed is the row's at once: the exe's default
// curve of the row was not read
void IceStormBoard::LoadStage(int theStage)
{
	if (mLevelRows.empty())
		return;
	const LevelRow aRow = StageRow(theStage);
	mPreviousStageFireSpeed = theStage > 0 ? StageRow(theStage - 1).mFireSpeed : 0.0f;
	mMaxActiveColumns = aRow.mColumnCount;
	mDoubleColumns = aRow.mDoubleColumns;
	mStageDurationTicks = std::max(1, aRow.mDurationTicks);
	mStageFireSpeed = aRow.mFireSpeed;
	mStageCrackSpeedMod = aRow.mCrackSpeedMod;
	if (theStage < (int)mLevelRows.size() && !aRow.mCurve.empty())
		mStageCurve.SetCurve(aRow.mCurve);
	else
		mStageCurve.SetConstant(1.0);
}

// 0x6e5690 (bejeweled3): the stage's row is read and each column is planned as part of a single or a double. With some doubles and some
// singles wanted, the middle columns (2 to 5) are singles and the edges doubles; with only doubles (the single count is not above the double
// count) all are doubles; with no doubles all are singles. The start of the stage is now. A stage starts with at least mStartColumns columns
// (the setting's count; 0x6e24b0 spawns them: its bound is not read to the end, see NOTES.md)
void IceStormBoard::PlanLayout()
{
	LoadStage(mStage);
	mStageStartTick = mTicks;
	const bool aBoth = mMaxActiveColumns > 0 && mDoubleColumns > 0;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		bool aSingle = mDoubleColumns < 1 || (mMaxActiveColumns - mDoubleColumns > 0 && (!aBoth || (aColumn > 1 && aColumn < 6)));
		Column& aState = mColumns[aColumn];
		if (aSingle) {
			if (mDoubleColumns == 0 || aBoth) {
				aState.mPlannedWidth = 1;
				for (int i = 0; i < aState.mSpan && aColumn + i < COLUMN_COUNT; i++)
					mColumns[aColumn + i].mPlannedAnchor = aColumn;
			}
		}
		else {
			aState.mPlannedWidth = 2;
			aState.mPlannedAnchor = aColumn & ~1;
		}
	}
	int aSingles = 0;
	int aDoubles = 0;
	CountActiveColumns(aSingles, aDoubles);
	while (aSingles + aDoubles < mStartColumns && SpawnColumn())
		CountActiveColumns(aSingles, aDoubles);
}

// 0x6c4fb0 (bejeweled3): the columns that are made and not crushed, each owner once, counted as doubles (span 2) or singles
void IceStormBoard::CountActiveColumns(int& theSingles, int& theDoubles) const
{
	theSingles = 0;
	theDoubles = 0;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		const Column& aState = mColumns[aColumn];
		if (aState.mOwner == aColumn && aState.mActive && !aState.mCrushed) {
			if (aState.mSpan == 2)
				theDoubles++;
			else
				theSingles++;
		}
	}
}

// 0x6c6870 (bejeweled3): the ice (in rows) the bar needs for the next multiplier: the list's entry for the multiplier less one (the last when
// the multiplier is past the list), 10 when the list is empty
float IceStormBoard::MultiplierThreshold() const
{
	if (mMultiplierIceReq.empty())
		return MULTIPLIER_THRESHOLD_DEFAULT;
	const int anIndex = std::min(mLevelNumber - 1, (int)mMultiplierIceReq.size() - 1);
	return mMultiplierIceReq[std::max(anIndex, 0)];
}

// 0x6c6520 (bejeweled3): true while a match group of type 7 that clears every colour (a hypercube's) lives (its other test, a group of type 2 whose
// +0x40 is the column, is of a group this port never makes). port: that second test is left out
bool IceStormBoard::ColumnHasLiveGroup(int theColumn) const
{
	(void)theColumn;
	for (const std::unique_ptr<MatchGroup>& aGroup : mMatchGroups) {
		if (aGroup->mType == 7 && aGroup->mColor == -1)
			return true;
	}
	return false;
}

// 0x6e0650 (bejeweled3): a column is made (theActive) or ended: the ice made starts at the bottom with its crack speed from the stage, a made column
// waits 0x32 updates and takes a random share of MaxRandFireSpeedColDelta as its own speed deviation; the columns it spans take its index as their
// owner (those it leaves, their planned one), and a double's owner is the even one
void IceStormBoard::SetColumnActive(int theColumn, int theWidth, bool theActive)
{
	Column& aState = mColumns[theColumn];
	if (aState.mActive != theActive) {
		aState.mCrushed = false;
		aState.mPosition = 0.0f;
		aState.mPreviousPosition = 0.0f;
		aState.mReachedHalf = false;
		aState.mActive = theActive;
		aState.mCrackSpeedMod = mStageCrackSpeedMod;
		aState.mCrushedTicks = 0;
	}
	for (int i = 0; i < aState.mSpan && theColumn + i < COLUMN_COUNT; i++) {
		mColumns[theColumn + i].mOwner = mColumns[theColumn + i].mPlannedAnchor;
		mColumns[theColumn + i].mActive = theActive;
	}
	if (theWidth != aState.mSpan)
		aState.mSpan = theWidth;
	for (int i = 0; i < aState.mSpan && theColumn + i < COLUMN_COUNT; i++) {
		mColumns[theColumn + i].mActive = theActive;
		mColumns[theColumn + i].mOwner = theActive ? theColumn : mColumns[theColumn + i].mPlannedAnchor;
	}
	aState.mOwner = theColumn - (aState.mSpan == 2 ? (theColumn & 1) : 0);
	if (theActive) {
		aState.mSpawnDelay = SPAWN_DELAY_TICKS;
		aState.mSpeedDeviation = (float)((double)(Rand() % 1000000) / 1000000.0 * mMaxRandomFireSpeedColumnDelta);
	}
	else
		aState.mSpawnDelay = 0;
}

// 0x6c5d30 (bejeweled3): the next column is due at the latest 0x4b game ticks from now
void IceStormBoard::LimitSpawnDeadline()
{
	int aLatest = mGameTicks + SPAWN_DEADLINE_TICKS;
	if (aLatest <= mSpawnDeadlineTick)
		mSpawnDeadlineTick = aLatest;
}

// 0x6e08a0 (bejeweled3): one more column of ice is made, true when it was. The candidates are the planned anchors that are not made (a planned
// double whose second column is in use by a made one is not one); none, or the count of the made ones at the stage's column count, ends it. They are
// put in a random order (the last stays last); an edge column (the first, or the last of the planned width) is put off while too few
// columns are made, by the stage's double count: with 3 or more doubles wanted the edges wait for one or two other columns, with fewer for
// two or four, with none for three or five. A column is made unless a hypercube's group is at work on it
bool IceStormBoard::SpawnColumn()
{
	std::vector<int> aInactive;
	std::vector<int> anActive;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		const Column& aState = mColumns[aColumn];
		if (aState.mPlannedAnchor != aColumn)
			continue;
		if (!aState.mActive) {
			if (aState.mPlannedWidth == 2 && aColumn + 1 < COLUMN_COUNT && mColumns[aColumn + 1].mActive)
				continue;
			aInactive.push_back(aColumn);
		}
		else
			anActive.push_back(aColumn);
	}
	if (aInactive.empty())
		return false;
	int aSingles = 0;
	int aDoubles = 0;
	CountActiveColumns(aSingles, aDoubles);
	if (aSingles + aDoubles >= mMaxActiveColumns)
		return false;
	const int anEdgeColumns = (mColumns[LAST_COLUMN].mActive ? 1 : 0) + (mColumns[0].mActive ? 1 : 0);
	const int aLast = aInactive.back();
	aInactive.pop_back();
	ShuffleWithAppRand(aInactive);
	aInactive.push_back(aLast);
	for (size_t aCandidateIndex = 0; aCandidateIndex < aInactive.size(); aCandidateIndex++) {
		const int aCandidate = aInactive[aCandidateIndex];
		const int aWidth = mColumns[aCandidate].mPlannedWidth;
		bool aInUse = false;
		for (int i = 0; i < aWidth && aCandidate + i < COLUMN_COUNT; i++)
			aInUse = aInUse || mColumns[aCandidate + i].mActive;
		if (aInUse)
			continue;
		const bool anEdge = aCandidate == 0 || aCandidate == (aWidth != 1 ? LAST_COLUMN - 1 : LAST_COLUMN);
		if (anEdge && aInactive.size() > 1) {
			bool aPutOff = false;
			if (mDoubleColumns > 2) {
				if (anEdgeColumns == 0)
					aPutOff = anActive.empty();
				else if (anEdgeColumns == 1)
					aPutOff = anActive.size() < 2;
			}
			else if (mDoubleColumns > 0) {
				if (anEdgeColumns == 0)
					aPutOff = anActive.size() < 2;
				else if (anEdgeColumns == 1)
					aPutOff = anActive.size() < 4;
			}
			else {
				if (anEdgeColumns == 0)
					aPutOff = anActive.size() < 3;
				else if (anEdgeColumns == 1)
					aPutOff = anActive.size() < 5;
			}
			if (aPutOff)
				continue;
		}
		bool aGroupAtWork = false;
		for (int i = 0; i < mColumns[aCandidate].mSpan && aCandidate + i < COLUMN_COUNT; i++)
			aGroupAtWork = aGroupAtWork || ColumnHasLiveGroup(aCandidate + i);
		if (aGroupAtWork)
			continue;
		SetColumnActive(aCandidate, aWidth, true);
		LimitSpawnDeadline();
		return true;
	}
	return false;
}

// 0x6d3df0 (bejeweled3): the fire speed of the stage now: the stage's curve (over the stage's updates) takes it from the last stage's to this
// one's. (The exe multiplies by a second curve, +0x3bd0, which it holds at 1.)
double IceStormBoard::StageFireSpeed()
{
	const double anInput = (double)(mTicks - mStageStartTick) / (double)mStageDurationTicks;
	return (mStageCurve.GetOutVal(anInput) * (double)(mStageFireSpeed - mPreviousStageFireSpeed) + (double)mPreviousStageFireSpeed);
}

// 0x6ce540 (bejeweled3): how far a column's crack has gone, from 0 to 1 (1 at SecondsUntilLose seconds, faster by the column's crack speed). port:
// the exe passes the fraction through a curve (0x4378f0 on an object that is not found); it is read as linear
double IceStormBoard::CrackFraction(const Column& theColumn) const
{
	if (theColumn.mCrackTicks < 0)
		return 0.0;
	const double aFraction = ((double)theColumn.mCrackTicks / 100.0 / (double)mSecondsUntilLose) * theColumn.mCrackSpeedMod;
	return std::min(aFraction, 1.0);
}

// 0x6ceba0 (bejeweled3): how hard the reprieve (after a multiplier) pushes a column down: nothing while it is below the curve's range (the
// top rows), else the larger of its own push and the curve's value at its height in the range
float IceStormBoard::ReprieveStrength(int theColumn)
{
	const Column& aState = mColumns[theColumn];
	const double aRangeStart = 8.0 - mReprieveStrengthVsRow.mInMax;
	if ((double)aState.mPosition <= aRangeStart)
		return 0.0f;
	const float aPush = aState.mPushStrength;
	const double aCurve = mReprieveStrengthVsRow.GetOutVal((double)aState.mPosition - aRangeStart);
	if (aCurve < (double)aPush)
		return aPush;
	return (float)mReprieveStrengthVsRow.GetOutVal((double)(aState.mPosition - (float)aRangeStart));
}

// 0x6e0cc0 (bejeweled3): the ice's step. theMove is false while the grace holds the columns still; theMayLose and theMaySpawn say whether a
// cracked column ends the game and whether a column may be made. For each anchor column: the push of the cleared gems decays; a made column waits
// its delay (all waiting columns' delays are cut to 0x14 when none is moving); one that is moving gets its speed from the stage's fire speed, the
// column's place in the grid (ColDistribution, mirrored for the right half) and its height (RowFireSpeed), the ramp after a multiplier and its
// own deviation, less the pushes of its columns; a push that wins moves the ice down (the ice pushed back is counted and freezes it for a while,
// a freeze slows its next rise), else it rises. The position is kept between -0.2 and 8, and over 0.5 once it has reached that. At 8 it cracks.
// port: the sounds, the PIEffects' speeds and the shake offsets are left out (0x6c5ea0, 0x6ce7c0 is a popup)
void IceStormBoard::StepColumns(bool theMove, bool theMayLose, bool theMaySpawn)
{
	mLoseColumn = -1;
	const double aFireBase = StageFireSpeed();
	bool aNoneMoving = true;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		Column& aState = mColumns[aColumn];
		if (!mColumns[aState.mOwner].mCrushed) {
			if (aState.mPushStrength > 0.0f)
				aState.mPushStrength = std::max((float)((double)aState.mPushStrength - (double)PUSH_DECAY_PER_TICK), 0.0f);
		}
		else
			aState.mPushStrength = 0.0f;
		if (aState.mOwner != aColumn)
			continue;
		if (aState.mSpawnDelay < 1 && aState.mActive)
			aNoneMoving = false;
		aState.mPreviousPosition = aState.mPosition;
		if (!aState.mActive || aState.mCrushed) {
			if (aState.mFreeze > 0.0)
				aState.mFreeze = std::max(aState.mFreeze - 1.0, 0.0);
			continue;
		}
		if (aState.mSpawnDelay >= 1) {
			if (aNoneMoving)
				aState.mSpawnDelay = std::min(aState.mSpawnDelay, (int)SPAWN_DELAY_WHEN_ALL_WAIT);
			aState.mSpawnDelay--;
			continue;
		}
		float aSpeed = 0.0f;
		if (theMove && (!mPaused || aState.mReprieveDelay > 0)) {
			int aRank = aColumn;
			if (aRank > 3)
				aRank = LAST_COLUMN - aRank;
			if (aState.mSpan == 2 && aColumn > 3)
				aRank--;
			const double aRowFactor = mRowFireSpeed.GetOutVal((double)aState.mPosition * 0.125);
			const double aColumnFactor = mColumnDistribution.GetOutVal((double)aRank * 0.25);
			aSpeed = (float)(aColumnFactor * aRowFactor);
			if (aState.mSpan > 1 && aRank == 0)
				aSpeed = (float)(aSpeed * (float)mDoubleEdgeMult);
			aSpeed = (float)(aSpeed * (float)aFireBase);
			if (aState.mRampDelay < 1)
				aState.mRamp.IncInVal();
			else
				aState.mRampDelay--;
			aSpeed = (float)(aState.mRamp.GetOutVal() * (double)aSpeed);
		}
		float aRise = aSpeed + aSpeed * aState.mSpeedDeviation;
		if (aState.mSpan > 1)
			aRise = (float)mDoubleColumnSpeedMult * aRise;
		float aPush = 0.0f;
		for (int i = 0; i < aState.mSpan && aColumn + i < COLUMN_COUNT; i++)
			aPush = mColumns[aColumn + i].mPushStrength + aPush;
		float aMove = aRise - aPush;
		if (aMove < 0.0f && 0.0f < aState.mPosition && !mPaused) {
			const float aPushedBack = std::min(std::fabs(aMove), aState.mPosition);
			mIceRemoved = mIceRemoved + aPushedBack;
			aState.mFreeze = std::min(std::fabs((double)aMove * mFreezeDurationPerNegativeStrength) + aState.mFreeze, mFreezeMax);
		}
		if (0.0f < aMove && 0.0 < aState.mFreeze) {
			aState.mFreeze = std::max(aState.mFreeze - 1.0, 0.0);
			aMove = (float)((1.0 - aState.mFreeze / mFreezeMax) * (double)aMove);
		}
		float aNewPosition = std::min(std::max(aState.mPosition + std::max(aMove, MOVE_LOWEST_STEP), POSITION_LOWEST), POSITION_TOP);
		if (!aState.mReachedHalf) {
			if (POSITION_HELD_ONCE_REACHED <= aNewPosition)
				aState.mReachedHalf = true;
		}
		else if (aNewPosition < POSITION_HELD_ONCE_REACHED)
			aNewPosition = POSITION_HELD_ONCE_REACHED;
		aState.mPosition = aNewPosition;   // slot 267 (0x6c4f20)
		if (aState.mPosition < POSITION_TOP) {
			aState.mCrackTicks = -1;
		}
		else {
			if (theMove) {
				if (aState.mCrackTicks == -1)
					aState.mCrackTicks = 0;
				else
					aState.mCrackTicks++;
			}
			if (CrackFraction(aState) >= 1.0)
				mLoseColumn = aColumn + (aState.mSpan != 1 ? 1 : 0);
		}
	}
	// the columns that were crushed: once the crush has shown for a while the ice they took off counts, and when it is over the column goes
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		Column& aState = mColumns[aColumn];
		if (aState.mOwner != aColumn || !aState.mActive || !aState.mCrushed)
			continue;
		aState.mCrushedTicks++;
		const float aProgress = std::min(1.0f, (float)aState.mCrushedTicks / (float)CRUSH_DONE_TICKS);
		if (aState.mPendingIceRemoved <= 0.0f || aProgress < CRUSH_PROGRESS_COUNTS) {
			if (aProgress >= 1.0f)
				SetColumnActive(aColumn, aState.mSpan, false);
		}
		else {
			mIceRemoved = mIceRemoved + aState.mPendingIceRemoved;
			aState.mPendingIceRemoved = 0.0f;
			LimitSpawnDeadline();
		}
	}
	if (theMayLose && mLoseColumn >= 0 && IsSettled() && !mPaused)
		NoMoreMoves(true);
	int aMade = 0;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		const Column& aState = mColumns[aColumn];
		if (aState.mOwner == aColumn && aState.mActive && !aState.mCrushed)
			aMade++;
	}
	if (!theMaySpawn || mMaxActiveColumns <= aMade)
		mSpawnDeadlineTick++;
	if (theMaySpawn && (mSpawnDeadlineTick <= mTicks || aMade == 0))
		SpawnColumn();
}

// slot 234 (0x3a8), 0x6e5850 (bejeweled3): Board's step (0x767090) first, then the ice's: the mode's tick count goes up, the grace and the multiplier's
// pause are counted down, the stage's time is checked, the ice steps (a made column may only appear while the pause is off), and when the ice
// pushed back is past the bar's need the multiplier rises. port: the intensity curve and the PIEffects it drives are left out
void IceStormBoard::UpdateTick()
{
	Board::UpdateTick();
	mTicks++;
	if (mGameOverTicks != 0 || mTicks < 2)
		return;
	if (mGraceTicks > 0)
		mGraceTicks--;
	const bool aMayRun = mGraceTicks == 0;
	if (mChainCount > 0 && mGameTicks <= mChainWindowTicks + mChainLastTick && mPaused && (mTicks & 1) == 0)
		mChainWindowTicks++;
	if (mPaused) {
		if (mPauseEndTick <= mTicks) {
			mPaused = false;
			for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
				const Column& aState = mColumns[aColumn];
				if (aState.mActive && aState.mOwner == aColumn && aState.mReprieveDelay == 0 && ReprieveStrength(aColumn) != 0.0f) {
					mPaused = true;
					break;
				}
			}
			if (mPaused && mTicks > mPauseEndTick + MULTIPLIER_PAUSE_EXTRA_TICKS)
				mPaused = false;
		}
		for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
			Column& aState = mColumns[aColumn];
			if (!aState.mActive || aState.mOwner != aColumn)
				continue;
			if (aState.mReprieveDelay > 0)
				aState.mReprieveDelay--;
			else
				aState.mPushStrength = ReprieveStrength(aColumn);
		}
	}
	if (aMayRun && mIsSecretMode) {
		// 0x6e5790: the stage's time is up: the next row. A quest (the secret flag clear) takes its other branch, which draws the app's Rand() twice for the
		// glow (+0x369c and +0x36a0) and does not step the stage or the multiplier: port: left out, see docs/NOTES.md, "Inferno as a quest"
		if (mTicks - mStageStartTick >= mStageDurationTicks) {
			mStage++;
			PlanLayout();
		}
		mPreviousIceRemoved = mIceRemoved;
		mProgress = Ftol((double)mIceRemoved * PROGRESS_SCALE);
		mProgressTarget = Ftol((double)mMultiplierIceThreshold * PROGRESS_SCALE);
	}
	StepColumns(aMayRun, aMayRun, aMayRun && !mPaused);
	if (aMayRun && mProgressTarget > 0 && (double)mProgress / (double)mProgressTarget > 1.0)
		RaiseMultiplier();
}

// 0x6e5850 (bejeweled3), its last part: the bar is full. The ice holds still for 200 updates, the multiplier goes up by one, the bar starts
// again with the next threshold, each made column is given its own wait before the reprieve (0x50 and 5 more for each column on) and, in a random
// order, a ramp (from 0.2 to 1) that starts later for each (0x1e, a random up to 0x1d and 0x14 more for each, added up)
void IceStormBoard::RaiseMultiplier()
{
	mPauseEndTick = mTicks + MULTIPLIER_PAUSE_TICKS;
	mPaused = true;
	mLevelNumber++;
	mMultiplierIceThreshold = MultiplierThreshold();
	mIceRemoved = 0.0f;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		Column& aState = mColumns[aColumn];
		aState.mReprieveDelay = aState.mActive && aState.mOwner == aColumn ? REPRIEVE_DELAY_BASE_TICKS + REPRIEVE_DELAY_STEP_TICKS * aColumn : 0;
	}
	std::vector<int> anOrder;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++)
		anOrder.push_back(aColumn);
	ShuffleWithAppRand(anOrder);
	int aStartDelay = 0;
	int anExtra = 0;
	for (size_t i = 0; i < anOrder.size(); i++) {
		Column& aState = mColumns[anOrder[i]];
		if (!aState.mActive || aState.mCrushed)
			continue;
		aState.mRamp.SetCurve(RAMP_CURVE);
		aState.mRampDelay = aStartDelay;
		aStartDelay += 0x1e + (Rand() % 0x1e + anExtra);
		anExtra += 0x14;
	}
}

// slot 22 (0x58), 0x6e63c0 (bejeweled3): QuestBoard's update (0x6dd8a0, which runs Board's), then the death: once the game is lost its curve runs and at its end the game ends
// (QuestBoard's 0x6d8e30, Board's NoMoreMoves)
void IceStormBoard::Update()
{
	QuestBoard::Update();
	if (mDeathStarted && mGameOverTicks == 0) {
		mDeathCurve.IncInVal();
		if (mDeathCurve.GetOutVal() >= 1.0)
			QuestBoard::NoMoreMoves(true);
	}
}

// slot 161 (0x284), 0x6e0450 (bejeweled3): the game is lost (no move is left, or a column cracked through): the death begins, which is a curve of
// 100 updates; the end comes with it. port: the "death" animation and the flame gems' flags it clears are left out
void IceStormBoard::NoMoreMoves(bool theAnnounce)
{
	(void)theAnnounce;
	if (mGameOverTicks != 0 || mDeathStarted)
		return;
	mDeathStarted = true;
	mDeathCurve.SetCurve(DEATH_CURVE);
}

// slot 211 (0x34c), 0x6e2250 (bejeweled3): a run of 3 or more gems whose gems all stand in one column (not -1) has its pieces' ids kept: when
// they are cleared the column's ice is crushed. In the secret mode, with the column's owner active, the first one queues the tutorial box (bit 0xd) over its first gem
// and asks for it at once (0x765d50)
void IceStormBoard::OnRunFound(const std::vector<Piece*>& thePieces)
{
	if (thePieces.size() < 3)
		return;
	const int aColumn = thePieces[0]->mCol;
	if (aColumn == -1)
		return;
	for (const Piece* aPiece : thePieces) {
		if (aPiece->mCol != aColumn)
			return;
	}
	for (const Piece* aPiece : thePieces)
		mVerticalRunPieceIds.push_back(aPiece->mId);
	if (aColumn >= 0 && aColumn < COLUMN_COUNT && mColumns[mColumns[aColumn].mOwner].mActive && mIsSecretMode && CanShowHint(HINT_VERTICAL_MATCH)) {
		QueueHint(thePieces[0], HINT_VERTICAL_MATCH);
		CheckForTutorialDialogs();
	}
}

// slot 143 (0x23c), 0x6dd580 (bejeweled3): a gem is cleared. One of a vertical run crushes its column's ice (0x6db1a0 with the gem's row). The
// column's own ice, if it is made, is pushed down by MatchPushStr, by SpecialGemPushMod times that when a special gem cleared it (the exe's
// test is the bit 2 of what cleared it), and a hypercube's group at work crushes it; then the goal's
void IceStormBoard::OnPieceCleared(Piece* thePiece)
{
	const int aColumn = thePiece->mCol;
	const int aRow = thePiece->mRow;
	if (aColumn < 0 || aColumn >= COLUMN_COUNT || aRow < 0 || aRow >= ROWS) {
		QuestBoard::OnPieceCleared(thePiece);
		return;
	}
	const int anOwner = mColumns[aColumn].mOwner;
	const bool anOwnerActive = mColumns[anOwner].mActive;
	std::vector<int>::iterator aFound = std::find(mVerticalRunPieceIds.begin(), mVerticalRunPieceIds.end(), thePiece->mId);
	if (aFound != mVerticalRunPieceIds.end()) {
		mVerticalRunPieceIds.erase(aFound);
		CrushColumn(anOwner, aRow);
	}
	if (anOwnerActive) {
		mColumns[aColumn].mPushStrength += mMatchPushStrength;
		if ((thePiece->mDestroyerFlags & Piece::FLAG_HYPERCUBE) != 0)
			mColumns[aColumn].mPushStrength += (mSpecialGemPushMod - 1.0f) * mMatchPushStrength;
		if (ColumnHasLiveGroup(aColumn))
			CrushColumn(anOwner, aRow);
	}
	QuestBoard::OnPieceCleared(thePiece);   // the end of 0x6dd580: the goal's slot 40 (a quest has one: Clear Gems counts the gem)
}

// slot 166 (0x2a8), 0x6caf30 (bejeweled3): a flame gem is made: its column's push goes up by (SpecialGemPushMod - 1) times MatchPushStr, then the
// board's own (0x745ca0)
void IceStormBoard::MakeFlameGem(Piece* thePiece)
{
	if (thePiece->mCol >= 0 && thePiece->mCol < COLUMN_COUNT)
		mColumns[thePiece->mCol].mPushStrength += (mSpecialGemPushMod - 1.0f) * mMatchPushStrength;
	Board::MakeFlameGem(thePiece);
}

// 0x6db1a0 (bejeweled3): a column's ice is crushed (true when it was): a made column that is not crushed and has no ice waiting to be counted is
// marked crushed, the ice it will take off the bar is its height (1 at least), its points are scored (0x6d6d60) and the columns crushed (counter
// 0x23) goes up by one. port: the sound is left out
bool IceStormBoard::CrushColumn(int theOwner, int theRow)
{
	Column& aState = mColumns[theOwner];
	if (!aState.mActive || aState.mCrushed || aState.mPendingIceRemoved != 0.0f)
		return false;
	aState.mCrushed = true;
	aState.mPendingIceRemoved = aState.mPosition >= 1.0f ? aState.mPosition : 1.0f;
	aState.mCrushedTicks = 0;
	ScoreCrushedColumn();
	(void)theRow;
	AddCounter(COUNTER_COLUMNS_CRUSHED, 1, -1);
	return true;
}

// 0x6d6d60 (bejeweled3): a crushed column's points. ColDestroyBonus times the multiplier is added to the bonus total (+0x366c, which the exe only
// keeps and shows in a popup). With ColCountBonus given, a column crushed within the chain's window of the last one counts as the next of a chain
// (else the first); the best chain is kept; the list's entry for the chain's length (the last for a longer one) is added to the score as it
// is, and the window for the next is the ColComboCoolDownVsCount curve's value at the chain's length (up to its range's end), in ticks.
// port: the chain's popups, flames and sounds are left out
void IceStormBoard::ScoreCrushedColumn()
{
	mColumnBonusTotal += (int)mColumnDestroyBonus * mLevelNumber;
	if (mColumnCountBonus.empty())
		return;
	if (mChainCount < 1 || mChainWindowTicks + mChainLastTick < mGameTicks)
		mChainCount = 0;
	mChainCount++;
	mBestChain = std::max(mBestChain, mChainCount);
	const int anIndex = std::min(mChainCount - 1, (int)mColumnCountBonus.size() - 1);
	const int aPoints = mColumnCountBonus[anIndex];
	mChainLastTick = mGameTicks;
	mChainWindowTicks = Ftol(mColumnComboCoolDownVsCount.GetOutVal(std::min((double)mChainCount, mColumnComboCoolDownVsCount.mInMax)));
	mScorePoints += aPoints;
	mColumnPoints += aPoints;
}

// slot 257 (0x404), 0x6d3180 (bejeweled3): the end dialog of the mode (vftable 0x8662c4, 0x390 bytes)
ClassicEndLevelDialog* IceStormBoard::MakeEndLevelDialog()
{
	return new IceStormEndLevelDialog(this);
}

// slot 43 (0xac), 0x6da820 (bejeweled3): the ice. port: the exe draws animated ice (PIEffects 0x330 and 0x334 of each column, with its crack and shake)
// from the grid's bottom; the port draws a pale blue block from the bottom to the ice's height that goes towards red as the crack runs and fades as a
// crushed column's ice breaks. The bar, the multiplier's text and the popups are left out (NOTES.md)
void IceStormBoard::DrawModeOverlay(Graphics* g)
{
	const int aBottom = GetGridY() + ROWS * CELL_SIZE;
	for (int aColumn = 0; aColumn < COLUMN_COUNT; aColumn++) {
		const Column& aState = mColumns[aColumn];
		if (aState.mOwner != aColumn || !aState.mActive || aState.mPosition <= 0.0f)
			continue;
		const int aHeight = Ftol((double)std::min(aState.mPosition, POSITION_TOP) * (double)CELL_SIZE);
		const double aCrack = CrackFraction(aState);
		const int aRed = Ftol(150.0 + 105.0 * aCrack);
		const int aGreen = Ftol(220.0 - 140.0 * aCrack);
		const int aBlue = Ftol(255.0 - 175.0 * aCrack);
		double anAlpha = 120.0;
		if (aState.mCrushed)
			anAlpha *= 1.0 - std::min(1.0, (double)aState.mCrushedTicks / (double)CRUSH_DONE_TICKS);
		g->SetColor(Color(aRed, aGreen, aBlue, Ftol(anAlpha)));
		g->FillRect(Scaled(GetGridX() + aColumn * CELL_SIZE + 4), Scaled(aBottom - aHeight), Scaled(aState.mSpan * CELL_SIZE - 8), Scaled(aHeight));
	}
	g->SetColor(Color(255, 255, 255, 255));
}

}  // namespace Sexy
