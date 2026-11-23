// Sexy::DigBoard and Sexy::DigGoal, the port's DigBoard and DigGoal: see the header. Every function says its slot and address in the exe (bejeweled3.exe).
#include "DiamondMineBoard.h"

#include "Bej3App.h"
#include "Common.h"
#include "EndLevelDialog.h"
#include "Graphics.h"
#include "Res.h"
#include "X86Math.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// a cell of the grid is 128 (1200ths)
const int CELL_SIZE = 128;
// the cells of the four neighbours that a cleared gem hits, in the order of the exe's two tables at 0x8d39fc (x) and 0x8d3a00 (y), a pair every 8 bytes: right, left, below, above
const int NEIGHBOUR_OFFSETS[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
// the letters of a cell of the grid (0x6bbb10 compares the upper case letter)
const char LETTER_HYPERCUBE = 'H';
const char LETTER_SHUFFLE = 'S';
const char LETTER_RANDOM = 'R';
const char LETTER_MONEY = 'M';
// the piece flag of a shuffle gem (0x6bbb10 sets it)
const int FLAG_SHUFFLE = 0x1000;
// a flame, hypercube or star gem's bits, which a brick is not (0x6b59f0 tests 2, 5 and 0x1000)
const int FLAGS_SPECIAL = 2 | 5 | FLAG_SHUFFLE;
// 0x6b6a80: the bits of the clearer that make a hit powered
const int DESTROYER_POWER_BITS = 7;
// 0x6ad0d0: the bits of the clearer that keep a cleared gem from hitting its neighbours (a flame or star: its own blast hits them)
const int DESTROYER_BLAST_BITS = 5;
// 0x6ad0d0: the bit of a clearer that makes a hit powered
const int DESTROYER_HYPERCUBE_BIT = 2;

// the setting's text with the blanks and quotes at its ends cut
std::string Trimmed(const std::string& theText)
{
	size_t aFirst = theText.find_first_not_of(" \t\r\n\"");
	if (aFirst == std::string::npos)
		return std::string();
	size_t aLast = theText.find_last_not_of(" \t\r\n\"");
	return theText.substr(aFirst, aLast - aFirst + 1);
}

// the app's Rand() as a number from 0 up to 1 (the exe's Rand() % 1000000 / 1e6)
float RandomFraction()
{
	return (float)((double)(int)(Rand() % 1000000) / 1000000.0);
}

}  // namespace

// 0x6c0430: the scroll's curves (the settings' text of the exe's own, set when a scroll starts)
const char* const DigGoal::SCROLL_DEPTH_CURVE = "b+0,1,0.01,1.5,#### O####        Q~###";
const char* const DigGoal::SCROLL_STEPS_CURVE = "b+0,10,0.01,1.5,#7GX  |~###    =~P##  f#:g2";

// ---- SpreadTable ----

// 0x784f80 (bejeweled3): the table is the identity
SpreadTable::SpreadTable()
{
	for (int i = 0; i < TABLE_SIZE; i++)
		mTable[i] = (float)i / (float)TABLE_SIZE;
}

// 0x784fe0 (bejeweled3): the curve's value at i / 500 is the weight of the numbers round i; the table's entries k are put at the i where the weights'
// running total (scaled to 499) first reaches k
void SpreadTable::Build(CurvedVal& theCurve)
{
	float aWeights[TABLE_SIZE];
	double aTotal = 0.0;
	for (int i = 0; i < TABLE_SIZE; i++) {
		aWeights[i] = (float)theCurve.GetOutVal((double)i / (double)TABLE_SIZE);
		aTotal += (double)aWeights[i];
	}
	if (aTotal <= 0.0)
		return;
	double aPosition = 0.0;
	int aNext = 0;
	for (int i = 0; i < TABLE_SIZE; i++) {
		aPosition += (double)aWeights[i] / aTotal * (double)(TABLE_SIZE - 1);
		while ((double)aNext <= aPosition && aNext < TABLE_SIZE) {
			mTable[aNext] = (float)((double)i / (double)(TABLE_SIZE - 1));
			aNext++;
		}
	}
}

// 0x784e90 (bejeweled3): the entry nearest to the value times 499 (the value is held between 0 and 1)
double SpreadTable::Lookup(double theValue) const
{
	double aIndex = theValue * (double)(TABLE_SIZE - 1);
	if (aIndex < 0.0)
		aIndex = 0.0;
	if (aIndex > (double)(TABLE_SIZE - 1))
		aIndex = (double)(TABLE_SIZE - 1);
	return (double)mTable[Ftol(aIndex)];
}

// ---- DigGoal ----

void DigGoal::Clear()
{
	std::memset(mHitMask, 0, sizeof(mHitMask));
	mBricks.clear();
	mHypercubeBrickIds.clear();
	mPlacedArtifacts.clear();
	mCollectedArtifacts.clear();
	mHits.clear();
	mDepth = 0.0;
	mRowsGenerated = 0.0;
	mWholeBoardClear = false;
	mTicksToArtifact = 0;
	mScrollDepthCurve.SetConstant(0.0);
	mScrollStepsCurve.SetConstant(0.0);
	mScrolling = false;
	mScrollRowsDone = 0;
	mScrollRows = 0;
	mTallyTicks = 0;
	mTallies[0] = mTallies[1] = mTallies[2] = 0;
	mGoldCollected = 0;
	mDiamondsCollected = 0;
	mArtifactsCollected = 0;
	mBestTreasureValue = 0;
	mScrollCount = 0;
	mBricksDestroyed = 0;
}

// 0x6c0430 (bejeweled3), run by the constructor 0x6c4230 from 0x6ddc70: the quest's settings are read (the counts with atoi, the rest with wtof, the lists
// and curves parsed); the grid's text goes to 0x744e00, the artifacts are the settings Artifact1 up, and the curves of the spreads become tables
void DigGoal::Load()
{
	mTargetCount = Dig()->SettingInt("TargetCount");
	mDigCountPerScroll = Dig()->SettingInt("DigCountPerScroll");
	mGoldValue = Dig()->SettingInt("GoldValue");
	mDiamondValue = Dig()->SettingInt("DiamondValue");
	mArtifactBaseValue = Dig()->SettingInt("ArtifactBaseValue");
	mArtifactSkipTileCount = Dig()->SettingInt("ArtifactSkipTileCount");
	mPowerGemThresholdDepth0 = Dig()->SettingInt("PowerGemThresholdDepth0");
	mPowerGemThresholdDepth20 = Dig()->SettingInt("PowerGemThresholdDepth20");
	mPowerGemThresholdDepth40 = Dig()->SettingInt("PowerGemThresholdDepth40");
	mArtifactMinTiles = Dig()->SettingInt("ArtifactMinTiles");
	mArtifactMaxTiles = Dig()->SettingInt("ArtifactMaxTiles");
	mTreasureRange.clear();
	for (double aValue : Dig()->SettingList("TreasureRange"))
		mTreasureRange.push_back((int)aValue);
	auto aSetting = [this](const char* theName) {
		auto anIter = Dig()->mSettings.find(theName);
		return anIter == Dig()->mSettings.end() ? std::string() : anIter->second;
	};
	mRandomAfterGrid = Dig()->HasSetting("MaxBrickStrPerLevel");
	mMinBrickStr.SetCurve(aSetting("MinBrickStrPerLevel"));
	mMaxBrickStr.SetCurve(aSetting("MaxBrickStrPerLevel"));
	mEdgeBrickStr.SetCurve(aSetting("EdgeBrickStrPerLevel"));
	mMinMineStr.SetCurve(aSetting("MinMineStrPerLevel"));
	mMaxMineStr.SetCurve(aSetting("MaxMineStrPerLevel"));
	mMineProb.SetCurve(aSetting("MineProbPerLevel"));
	mDarkRockFrequency.SetCurve(aSetting("DarkRockFrequency"));
	mArtifactPossRange.SetCurve(aSetting("ArtifactPossRange"));
	CurvedVal aSpread;
	aSpread.SetCurve(aSetting("BrickStrSpread"));
	mBrickStrSpread.Build(aSpread);
	aSpread.SetCurve(aSetting("MineStrSpread"));
	mMineStrSpread.Build(aSpread);
	aSpread.SetCurve(aSetting("ArtifactSpread"));
	mArtifactSpread.Build(aSpread);
	// the artifacts: "id, minDepth, value, text" (the text may hold commas: it is the rest of the line)
	mArtifacts.clear();
	for (int aNumber = 1;; aNumber++) {
		char aName[24];
		snprintf(aName, sizeof(aName), "Artifact%d", aNumber);
		auto anIter = Dig()->mSettings.find(aName);
		if (anIter == Dig()->mSettings.end())
			break;
		const std::string& aText = anIter->second;
		std::string aFields[4];
		size_t aStart = 0;
		for (int aField = 0; aField < 4; aField++) {
			size_t anEnd = aField < 3 ? aText.find(',', aStart) : std::string::npos;
			if (anEnd == std::string::npos) {
				aFields[aField] = Trimmed(aText.substr(std::min(aStart, aText.size())));
				break;
			}
			aFields[aField] = Trimmed(aText.substr(aStart, anEnd - aStart));
			aStart = anEnd + 1;
		}
		Artifact anArtifact;
		anArtifact.mId = aFields[0];
		anArtifact.mMinDepth = atoi(aFields[1].c_str());
		anArtifact.mValue = atoi(aFields[2].c_str());
		anArtifact.mText = aFields[3];
		mArtifacts.push_back(anArtifact);
	}
	mGridCells.clear();
	auto aGridIter = Dig()->mSettings.find("Grids");
	if (aGridIter != Dig()->mSettings.end())
		ParseGrid(aGridIter->second);
	mTicksToArtifact = mArtifactMinTiles;   // port: the exe's value before the first artifact's draw is not read; the first artifact comes after the shortest run
}

// 0x744e00 (bejeweled3): the text is read a character at a time. A digit starts the next cell (cell n is row n / 8, column n % 8) with that digit as its strength; a
// letter is the letter of the cell before it
void DigGoal::ParseGrid(const std::string& theText)
{
	for (char aCharacter : theText) {
		if (isalpha((unsigned char)aCharacter)) {
			if (!mGridCells.empty())
				mGridCells.back().mLetter = (char)toupper((unsigned char)aCharacter);
		}
		else if (isdigit((unsigned char)aCharacter)) {
			GridCell aCell;
			aCell.mStrength = aCharacter - '0';
			mGridCells.push_back(aCell);
		}
	}
}

DigGoal::Brick* DigGoal::FindBrick(const Piece* thePiece)
{
	auto anIter = mBricks.find(thePiece->mId);
	return anIter == mBricks.end() ? nullptr : &anIter->second;
}

// 0x6b3700 (bejeweled3): the brick's record is made for the piece: its kind, its strength (twice) and a random number; the effects the exe makes for it (its
// dirt's and its treasure's looks, +0x10, +0x14 and +0x1c of the record) are not translated
void DigGoal::SetBrick(Piece* thePiece, int theKind, int theStrength)
{
	Brick& aBrick = mBricks[thePiece->mId];
	aBrick = Brick();
	aBrick.mKind = theKind;
	aBrick.mHitsLeft = theStrength;
	aBrick.mHitsInitial = theStrength;
	aBrick.mSeed = (int)Rand();
	thePiece->mFlags |= FLAG_BRICK;
}

// 0x6b3a50 (bejeweled3): the curves are read at the depth. By chance (MineProb) the block is money: its strength is MinMineStr and MaxMineStr with the spread between,
// at most 7. Otherwise it is dirt: MinBrickStr and MaxBrickStr with the spread between, times EdgeBrickStr in the first and last columns, from 1 to 5; a strength of 4 or more is
// the dark rock (5) by chance (DarkRockFrequency) and a 4 otherwise
void DigGoal::MakeRandomBrick(Piece* thePiece, int theColumn, double theDepth, bool& theMade)
{
	const double aMinBrick = mMinBrickStr.GetOutVal(theDepth);
	const double aMaxBrick = mMaxBrickStr.GetOutVal(theDepth);
	const double anEdge = mEdgeBrickStr.GetOutVal(theDepth);
	const double aMinMine = mMinMineStr.GetOutVal(theDepth);
	const double aMaxMine = mMaxMineStr.GetOutVal(theDepth);
	const double aMineProbability = mMineProb.GetOutVal(theDepth);
	const double aDarkRock = mDarkRockFrequency.GetOutVal(theDepth);
	if (RandomFraction() < (float)aMineProbability) {
		const double aSpread = mMineStrSpread.Lookup((double)RandomFraction());
		int aStrength = Ftol(aMinMine + aSpread * (aMaxMine - aMinMine));
		aStrength = std::min(aStrength, (int)STRENGTH_MONEY_MAX);
		aStrength = std::max(aStrength, 1);
		SetBrick(thePiece, KIND_MONEY, aStrength);
		theMade = true;
		return;
	}
	const double aSpread = mBrickStrSpread.Lookup((double)RandomFraction());
	double aValue = aMinBrick + aSpread * (aMaxBrick - aMinBrick);
	if (theColumn == 0 || theColumn == COLS - 1)
		aValue *= anEdge;
	int aStrength = Ftol(aValue);
	aStrength = std::min(aStrength, (int)STRENGTH_DIRT_MAX);
	aStrength = std::max(aStrength, 1);
	if (aStrength >= STRENGTH_NEEDS_POWER)
		aStrength = (double)RandomFraction() <= aDarkRock ? STRENGTH_DARK_ROCK : STRENGTH_NEEDS_POWER;
	SetBrick(thePiece, KIND_DIRT, aStrength);
	theMade = true;
}

// the absolute depth of the first row now: the rows that have scrolled, and what the scroll's own curve has taken so far
int DigGoal::CurrentDepth() const
{
	const int aMultiplier = ((mWholeBoardClear ? 1 : 0) + 1) * mDigCountPerScroll;
	return Ftol(const_cast<CurvedVal&>(mScrollDepthCurve).GetOutVal() * (double)aMultiplier + mDepth);
}

int DigGoal::DepthShown() const
{
	return CurrentDepth();
}

// 0x6bbb10 (bejeweled3), the artifacts a row may be given: those whose minimum depth is reached and that are neither in the dirt nor collected, in groups by
// their minimum depth, the deepest group first, until there are as many as ArtifactPossRange allows at the row count; then the order is reversed
std::vector<int> DigGoal::ArtifactPool(int theDepth)
{
	std::map<int, std::vector<int>> aGroups;
	for (int anIndex = 0; anIndex < (int)mArtifacts.size(); anIndex++) {
		if (mArtifacts[anIndex].mMinDepth > theDepth)
			continue;
		if (std::find(mPlacedArtifacts.begin(), mPlacedArtifacts.end(), anIndex) != mPlacedArtifacts.end())
			continue;
		if (std::find(mCollectedArtifacts.begin(), mCollectedArtifacts.end(), anIndex) != mCollectedArtifacts.end())
			continue;
		aGroups[mArtifacts[anIndex].mMinDepth].push_back(anIndex);
	}
	const int aWanted = Ftol(mArtifactPossRange.GetOutVal(mRowsGenerated));
	std::vector<int> aPool;
	for (auto aGroup = aGroups.rbegin(); aGroup != aGroups.rend() && (int)aPool.size() < aWanted; ++aGroup)
		aPool.insert(aPool.end(), aGroup->second.begin(), aGroup->second.end());
	std::reverse(aPool.begin(), aPool.end());
	return aPool;
}

// 0x6bbb10 (bejeweled3): for each row from theStartRow down and each column, the piece there (the fill made it) becomes a brick if the grid's cell (or, past the grid or
// where the cell says R, the random one) says so. A block made counts one off the run to the next artifact; when that is out (the count was below 1) and the
// block was a random one, it is a candidate, and then one of the candidates of the row is given an artifact picked by ArtifactSpread from the pool, and the run
// to the next artifact is a new random one between ArtifactMinTiles and ArtifactMaxTiles. port: the exe's +0x18 of a brick (the first made in its column by the call)
// is kept; the +0x248 offset of the scroll (rows * -0x80 + 0x674, which the draw reads) is not
void DigGoal::MakeRows(int theStartRow)
{
	Board& aBoard = *mBoard;
	bool aSeenInColumn[COLS];
	for (int i = 0; i < COLS; i++)
		aSeenInColumn[i] = theStartRow != 0;
	const int aTableRows = (int)mGridCells.size() / COLS;
	for (int aRow = theStartRow; aRow < ROWS; aRow++) {
		const int aDepthBase = CurrentDepth();
		int aTableRow = aDepthBase + aRow;
		std::vector<int> aPool = ArtifactPool(aDepthBase);
		std::vector<Piece*> aCandidates;
		for (int aColumn = 0; aColumn < COLS; aColumn++) {
			Piece* aPiece = aBoard.mGrid[aRow][aColumn];
			if (aPiece == nullptr)
				continue;
			bool aMade = false;
			bool aArtifactDue = false;
			bool aMayHoldArtifact = Dig()->mIsSecretMode;
			bool aBrickMade = false;
			if (!mRandomAfterGrid || aTableRow < aTableRows) {
				if (aTableRows > 0) {
					while (aTableRow >= aTableRows)
						aTableRow -= ROWS;
				}
				const GridCell aCell = aTableRows > 0 ? mGridCells[(size_t)(aColumn + aTableRow * COLS)] : GridCell();
				const char aLetter = (char)toupper((unsigned char)aCell.mLetter);
				if (aLetter == LETTER_HYPERCUBE) {
					if (aCell.mStrength == 0)
						aBoard.MakeHypercube(aPiece, true);
					else
						mHypercubeBrickIds.push_back(aPiece->mId);
					aMayHoldArtifact = false;
				}
				else if (aLetter == LETTER_SHUFFLE) {
					aPiece->mFlags |= FLAG_SHUFFLE;
					aMayHoldArtifact = false;
				}
				else if (aLetter == LETTER_RANDOM && mRandomAfterGrid) {
					MakeRandomBrick(aPiece, aColumn, mRowsGenerated, aMade);
					aBrickMade = aMade;
				}
				if (aCell.mStrength != 0) {
					SetBrick(aPiece, aLetter == LETTER_MONEY ? KIND_MONEY : (aLetter == 'A' ? KIND_ARTIFACT : KIND_DIRT), aCell.mStrength);
					aMayHoldArtifact = false;
					aMade = true;
					aBrickMade = true;
				}
			}
			else {
				MakeRandomBrick(aPiece, aColumn, mRowsGenerated, aMade);
				aBrickMade = aMade;
			}
			if (aBrickMade) {
				const int aRunLeft = mTicksToArtifact;
				mTicksToArtifact--;
				if (aRunLeft < 1)
					aArtifactDue = true;
				mBricks[aPiece->mId].mFirstInColumn = !aSeenInColumn[aColumn];
				aSeenInColumn[aColumn] = true;
				if (aArtifactDue && aMayHoldArtifact)
					aCandidates.push_back(aPiece);
			}
			if (aMade) {
				aPiece->mCanSwap = false;
				aPiece->mClearable = true;
				aPiece->mCanFall = false;
				aPiece->mColor = Piece::NO_COLOR;
				aPiece->mX = (float)(aPiece->mCol * CELL_SIZE);
				aPiece->mY = (float)(aPiece->mRow * CELL_SIZE);
				aPiece->mFallSpeed = 0.0f;
			}
			if (aArtifactDue && !aCandidates.empty() && !aPool.empty()) {
				Piece* aChosen = aCandidates[Rand() % aCandidates.size()];
				const double aSpread = mArtifactSpread.Lookup((double)RandomFraction());
				int aPoolIndex = Ftol(aSpread * (double)((int)aPool.size() - 1));
				aPoolIndex = std::max(0, std::min(aPoolIndex, (int)aPool.size() - 1));
				const int anArtifact = aPool[(size_t)aPoolIndex];
				SetBrick(aChosen, KIND_ARTIFACT, anArtifact);
				mPlacedArtifacts.push_back(anArtifact);
				aPool.erase(aPool.begin() + aPoolIndex);
				if (Dig()->mIsSecretMode) {
					const int aSpan = std::max(1, mArtifactMaxTiles - mArtifactMinTiles);
					mTicksToArtifact = (int)(Rand() % (unsigned)aSpan) + mArtifactMinTiles;
				}
			}
		}
		if (mRandomAfterGrid)
			mRowsGenerated += 1.0;
	}
	PlaceBricks();
}

// 0x6b9690 (bejeweled3): the bricks stand at their cells (the exe adds the scroll's pixel offset while its curve runs; the port's scroll is a step)
void DigGoal::PlaceBricks()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aColumn = 0; aColumn < COLS; aColumn++) {
			Piece* aPiece = Dig()->mGrid[aRow][aColumn];
			if (!IsBrick(aPiece))
				continue;
			aPiece->mX = (float)(aColumn * CELL_SIZE);
			aPiece->mY = (float)(aRow * CELL_SIZE);
			aPiece->mFallSpeed = 0.0f;
		}
	}
}

// 0x6aae50 (bejeweled3): with the secret flag set, true when no row of the first six (the first eight with theAllRows) holds a brick (or the flag +0x5f8 is set: nothing sets it
// in a game)
bool DigGoal::RowsAreFree(bool theAllRows) const
{
	const int aLastRow = theAllRows ? ROWS - 1 : 5;
	for (int aRow = 0; aRow <= aLastRow; aRow++) {
		for (int aColumn = 0; aColumn < COLS; aColumn++) {
			const Piece* aPiece = Dig()->mGrid[aRow][aColumn];
			if (aPiece != nullptr && (aPiece->mFlags & FLAG_BRICK) != 0)
				return false;
		}
	}
	return true;
}

// 0x6b59f0 (bejeweled3), the treasure of a block: a block of artifact is worth ArtifactBaseValue times the artifact's value; one of money is the roll from its kind's
// pair of TreasureRange (min + Rand % (max - min + 1)) times GoldValue (strengths 1 to 3) or DiamondValue (4 and more). 0x6a9780 adds it to the progress, which is the
// score (+0xe00), sets the tally's timer and the kind's tally. port: the exe's flying treasure (the list at +0x658) arrives after a while; here it is added at once
int DigGoal::CollectTreasure(Piece* thePiece, const Brick& theBrick)
{
	(void)thePiece;
	int aValue = 0;
	if (theBrick.mKind == KIND_ARTIFACT) {
		const int anIndex = theBrick.mHitsLeft;
		aValue = mArtifactBaseValue * (anIndex >= 0 && anIndex < (int)mArtifacts.size() ? mArtifacts[(size_t)anIndex].mValue : 0);
		mCollectedArtifacts.push_back(anIndex);
		mArtifactsCollected++;
		if (Dig()->mIsSecretMode)
			mTallies[KIND_ARTIFACT] += aValue;
	}
	else {
		const int aStrength = theBrick.mHitsInitial;
		const int aKinds = (int)mTreasureRange.size() / 2;
		int aRoll = 1;
		if (aKinds > 0) {
			const int aKind = std::min(std::max(aStrength - 1, 0), aKinds - 1);
			const int aMinimum = mTreasureRange[(size_t)aKind * 2];
			const int aMaximum = mTreasureRange[(size_t)aKind * 2 + 1];
			aRoll = (int)(Rand() % (unsigned)(aMaximum - aMinimum + 1)) + aMinimum;
		}
		aValue = aRoll * (aStrength <= GOLD_STRENGTH_MAX ? mGoldValue : mDiamondValue);
		if (aStrength <= GOLD_STRENGTH_MAX)
			mGoldCollected++;
		else
			mDiamondsCollected++;
		if (Dig()->mIsSecretMode)
			mTallies[KIND_MONEY] += aValue;
	}
	// 0x6a9780: in a secret mode the value is added to the progress and the tally is shown; in a quest each treasure counts 1 (the progress and the score)
	if (Dig()->mIsSecretMode) {
		Dig()->mLevelProgress += aValue;
		mTallyTicks = TALLY_SHOWN_TICKS;
	}
	else {
		Dig()->mLevelProgress++;
		Dig()->mScorePoints++;
	}
	mBestTreasureValue = std::max(mBestTreasureValue, aValue);
	return aValue;
}

// 0x6b59f0 (bejeweled3): a brick is hit; true when it is destroyed. A cell is hit once by a call that uses the mask (the cell is marked at the end whatever happens). A
// block of money or artifact is collected by the hit and goes. Dirt of strength 4 is only hit by a powered hit, which makes it 1; any other dirt with hits left (not
// the 4) takes one, two for the dark rock (5, which goes to 3), and is destroyed at 0. port: the exe's treasure takes hits as dirt does through the code that follows its
// collection; what that is for a block of money with more than one hit is not read (NOTES.md, Oddities): a treasure is collected by its first hit
bool DigGoal::DamageBrick(Piece* thePiece, bool thePowered, bool theUseMask)
{
	const int aRow = thePiece->mRow;
	const int aColumn = thePiece->mCol;
	if (theUseMask && mHitMask[aRow][aColumn])
		return false;
	bool aDestroyed = false;
	if (IsBrick(thePiece) && (thePiece->mFlags & FLAGS_SPECIAL) == 0) {
		thePiece->mRingMark = 0.0f;   // +0x264
		Brick* aBrick = FindBrick(thePiece);
		if (aBrick != nullptr) {
			if (aBrick->mKind == KIND_ARTIFACT || aBrick->mKind == KIND_MONEY) {
				CollectTreasure(thePiece, *aBrick);
				aDestroyed = true;
			}
			else {
				if (aBrick->mHitsLeft == STRENGTH_NEEDS_POWER && thePowered)
					aBrick->mHitsLeft = 1;
				if (aBrick->mHitsLeft > 0 && aBrick->mHitsLeft != STRENGTH_NEEDS_POWER) {
					if (aBrick->mHitsLeft > STRENGTH_NEEDS_POWER)
						aBrick->mHitsLeft--;
					aBrick->mHitsLeft--;
					if (aBrick->mHitsLeft == 0)
						aDestroyed = true;
				}
			}
		}
	}
	mHitMask[aRow][aColumn] = true;
	return aDestroyed;
}

// slot 143 (0x23c) of the board, 0x6ad0d0 (bejeweled3): a cleared gem (not a brick) says its cell was hit: unless it was cleared by a flame or star gem (a gem with a
// colour that a blast took out: the blast hits what is near itself), the cell goes in the list with whether the hit is powered (no colour, or cleared by a hypercube) and the
// gem's move
void DigGoal::OnPieceCleared(Piece* thePiece)
{
	if (IsBrick(thePiece))
		return;
	const bool aClearedByAnother = thePiece->mDestroyerId != -1 || thePiece->mDestroyerFlags != 0;   // +0x17d
	if (aClearedByAnother && thePiece->mColor != Piece::NO_COLOR && (thePiece->mDestroyerFlags & DESTROYER_BLAST_BITS) != 0)
		return;
	HitEntry anEntry;
	anEntry.mCol = thePiece->mCol;
	anEntry.mRow = thePiece->mRow;
	anEntry.mPowered = thePiece->mColor == Piece::NO_COLOR || (thePiece->mDestroyerFlags & DESTROYER_HYPERCUBE_BIT) != 0;
	anEntry.mMoveId = thePiece->mColumnTag;
	mHits.push_back(anEntry);
}

// slot 176 (0x2c0) with slot 142 (0x73d2b0) of the board: 0x6b6a80 (an explosion's hit) and 0x6bb6a0 (the removal). A brick that is not removed by the goal itself is hit as a blast
// does it (powered when its clearer had a special's bit); if it stands it is kept and what the clear set on it is undone. The dirt that hides a hypercube, once broken,
// becomes the hypercube (0x6bb6a0: MakeHypercube, its colour the most common in the grid, the brick flag off, and its group made) and is kept. Any other piece taken
// out of the grid that was a brick loses its record. True keeps the piece
bool DigGoal::OnPieceRemoving(Piece* thePiece)
{
	if (!IsBrick(thePiece))
		return false;
	if (!Dig()->mDigDestroying && !Dig()->mDigScrolling) {
		const bool aPowered = (thePiece->mDestroyerFlags & DESTROYER_POWER_BITS) != 0;
		if (!DamageBrick(thePiece, aPowered, false)) {
			thePiece->mMatchTimer = 0.0f;
			thePiece->mIsGroupOrigin = false;
			thePiece->mClearHeld = false;
			thePiece->mBusyCount = 0;
			thePiece->mDestroyerId = -1;
			thePiece->mDestroyerFlags = 0;
			return true;
		}
	}
	auto aHidden = std::find(mHypercubeBrickIds.begin(), mHypercubeBrickIds.end(), thePiece->mId);
	if (aHidden != mHypercubeBrickIds.end() && !Dig()->mDigScrolling) {
		mHypercubeBrickIds.erase(aHidden);
		mBricks.erase(thePiece->mId);
		mBricksDestroyed++;
		Dig()->MakeHypercube(thePiece, true);
		int aCounts[Piece::COLOR_COUNT] = {};
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aColumn = 0; aColumn < COLS; aColumn++) {
				const Piece* aPiece = Dig()->mGrid[aRow][aColumn];
				if (aPiece != nullptr && aPiece->mColor >= 0 && aPiece->mColor < Piece::COLOR_COUNT)
					aCounts[aPiece->mColor]++;
			}
		}
		int aMostCommon = 0;
		for (int aColor = 1; aColor < Piece::COLOR_COUNT; aColor++) {
			if (aCounts[aColor] > aCounts[aMostCommon])
				aMostCommon = aColor;
		}
		thePiece->mFlags &= ~FLAG_BRICK;
		thePiece->mCanSwap = true;
		thePiece->mCanFall = true;
		thePiece->mMatchTimer = 0.0f;
		thePiece->mIsGroupOrigin = false;
		Dig()->MakeHypercubeGroup(thePiece, aMostCommon);
		return true;
	}
	if (mBricks.erase(thePiece->mId) != 0)
		mBricksDestroyed++;
	auto aHiddenOther = std::find(mHypercubeBrickIds.begin(), mHypercubeBrickIds.end(), thePiece->mId);
	if (aHiddenOther != mHypercubeBrickIds.end())
		mHypercubeBrickIds.erase(aHiddenOther);
	return false;
}

// 0x6c2ad0 (bejeweled3), the goal's update: the masks of the hits are cleared; each cleared gem's four neighbours are hit (their move id raised to the gem's):
// a powered hit is made first with the mask, and if that does not break the brick (or the hit is not powered) the hit goes again with no mask; then the scroll goes on, or starts
// when the first six rows hold no brick (the scroll's curve is not running, the board is at rest with no swap pending, and the board's own conditions hold: 0x75fca0)
void DigGoal::Update()
{
	Board& aBoard = *mBoard;
	std::memset(mHitMask, 0, sizeof(mHitMask));
	std::vector<HitEntry> aEntries;
	aEntries.swap(mHits);
	for (const HitEntry& anEntry : aEntries) {
		for (int aDirection = 0; aDirection < 4; aDirection++) {
			const int aColumn = anEntry.mCol + NEIGHBOUR_OFFSETS[aDirection][0];
			const int aRow = anEntry.mRow + NEIGHBOUR_OFFSETS[aDirection][1];
			if (aColumn < 0 || aColumn >= COLS || aRow < 0 || aRow >= ROWS)
				continue;
			Piece* aNeighbour = aBoard.mGrid[aRow][aColumn];
			if (aNeighbour == nullptr)
				continue;
			aNeighbour->mColumnTag = std::max(aNeighbour->mColumnTag, anEntry.mMoveId);
			if (!IsBrick(aNeighbour))
				continue;
			bool aDone = false;
			if (anEntry.mPowered)
				aDone = Dig()->DigHit(aNeighbour, true, true);
			if (!aDone && aBoard.mGrid[aRow][aColumn] == aNeighbour)
				Dig()->DigHit(aNeighbour, anEntry.mPowered, false);
		}
	}
	if (mTallyTicks > 0)
		mTallyTicks--;
	if (mScrolling) {
		UpdateScroll();
		return;
	}
	if (aBoard.mGameOver || aBoard.mGameOverTicks != 0 || Dig()->mTimeUp || !RowsAreFree(false))
		return;
	if (!aBoard.mPendingSwaps.empty() || gApp->GetDialogCount() != 0 || !aBoard.IsSettled())
		return;
	StartScroll();
}

// 0x6c2ad0 (bejeweled3), where the scroll starts: with the grid empty of bricks (all eight rows) the scroll is the long one and its bonus the mega one; the time is added (0x736f20);
// the depth goes up by what the depth curve had reached (0 after a finished scroll), and the two curves are set. port: the sounds "predrill" and "drill", the board's
// slot 0x414 (the helping hand's power gems for the first depths: PowerGemThresholdDepth0, 20 and 40) and the effects are not translated
void DigGoal::StartScroll()
{
	mWholeBoardClear = RowsAreFree(true);
	Dig()->GiveTimeBonus(mWholeBoardClear);
	const int aMultiplier = ((mWholeBoardClear ? 1 : 0) + 1) * mDigCountPerScroll;
	mDepth += (double)(aMultiplier * Ftol(mScrollDepthCurve.GetOutVal()));
	mScrollDepthCurve.SetCurve(SCROLL_DEPTH_CURVE);
	mScrollStepsCurve.SetCurve(SCROLL_STEPS_CURVE);
	mScrollRows = aMultiplier;
	mScrollRowsDone = 0;
	mScrolling = true;
	mScrollCount++;
}

// 0x6be730 (bejeweled3): the steps curve goes on by a step; when its value (rounded) changes and was below the scroll's rows, a row is scrolled. When the curve is done the depth
// goes up by the scroll's rows times the depth curve's value (1) and the curves are put to 0
void DigGoal::UpdateScroll()
{
	const int aBefore = Ftol(mScrollStepsCurve.GetOutVal());
	mScrollStepsCurve.IncInVal();
	mScrollDepthCurve.IncInVal();
	const int anAfter = Ftol(mScrollStepsCurve.GetOutVal());
	const int aMultiplier = ((mWholeBoardClear ? 1 : 0) + 1) * mDigCountPerScroll;
	if (anAfter != aBefore && aBefore < aMultiplier) {
		ScrollOneRow();
		mScrollRowsDone++;
	}
	if (!mScrollStepsCurve.IsDoingCurve()) {
		mDepth += (double)(aMultiplier * Ftol(mScrollDepthCurve.GetOutVal()));
		mScrollDepthCurve.SetConstant(0.0);
		mScrollStepsCurve.SetConstant(0.0);
		mScrolling = false;
	}
}

// 0x6be3e0 (bejeweled3): the top row goes, every piece moves up a row, a row of new gems (each given a colour again, up to 25 times, while it forms a match) is made at
// the bottom and its bricks by 0x6bbb10 (7); with no move left, a hypercube is made of a random piece of the top row (up to 49 tries). port: the top row's pieces
// are deleted (the exe keeps them in the list at +0x444 to slide off), the pieces move up at once and the power-gem specs that the exe carries from the top row to the new row
// (slots 184 and 185, the helping hand) are not made
void DigGoal::ScrollOneRow()
{
	Board& aBoard = *mBoard;
	Dig()->mDigScrolling = true;
	for (int aColumn = 0; aColumn < COLS; aColumn++) {
		Piece* aPiece = aBoard.mGrid[0][aColumn];
		if (aPiece != nullptr)
			aBoard.RemovePiece(aPiece);
	}
	Dig()->mDigScrolling = false;
	for (int aRow = 1; aRow < ROWS; aRow++) {
		for (int aColumn = 0; aColumn < COLS; aColumn++) {
			Piece* aPiece = aBoard.mGrid[aRow][aColumn];
			aBoard.mGrid[aRow][aColumn] = nullptr;
			aBoard.mGrid[aRow - 1][aColumn] = aPiece;
			if (aPiece == nullptr)
				continue;
			aPiece->mRow = aRow - 1;
			aPiece->mY -= (float)CELL_SIZE;
		}
	}
	const int aLastRow = ROWS - 1;
	for (int aColumn = 0; aColumn < COLS; aColumn++) {
		Piece* aPiece = aBoard.MakePiece(aColumn, aLastRow);
		aPiece->mFlags = 0;
		aPiece->mCanFall = true;
		int aColor = Piece::NO_COLOR;
		for (int aTry = 0; aTry < NEW_ROW_COLOR_TRIES; aTry++) {
			aColor = aBoard.mColorList[aBoard.mRand.Next((unsigned long)aBoard.mColorList.size())];
			bool aMatches = false;
			if (aColumn >= 2 && aBoard.mGrid[aLastRow][aColumn - 1] != nullptr && aBoard.mGrid[aLastRow][aColumn - 2] != nullptr &&
				aBoard.mGrid[aLastRow][aColumn - 1]->mColor == aColor && aBoard.mGrid[aLastRow][aColumn - 2]->mColor == aColor)
				aMatches = true;
			if (aLastRow >= 2 && aBoard.mGrid[aLastRow - 1][aColumn] != nullptr && aBoard.mGrid[aLastRow - 2][aColumn] != nullptr &&
				aBoard.mGrid[aLastRow - 1][aColumn]->mColor == aColor && aBoard.mGrid[aLastRow - 2][aColumn]->mColor == aColor)
				aMatches = true;
			if (!aMatches)
				break;
		}
		aPiece->mColor = aColor;
	}
	MakeRows(aLastRow);
	EnsureMove();
}

// 0x6be3e0 (bejeweled3), its end: with no move on the board a random piece of the top row becomes a hypercube, up to 49 tries
void DigGoal::EnsureMove()
{
	Board& aBoard = *mBoard;
	for (int aTry = 0; aTry < NO_MOVE_HYPERCUBE_TRIES && !aBoard.HasMove(false); aTry++) {
		Piece* aPiece = aBoard.mGrid[0][Rand() % COLS];
		if (aPiece != nullptr && !IsBrick(aPiece) && aPiece->mFlags == 0)
			aBoard.MakeHypercube(aPiece, true);
	}
}

// the board (QuestGoal's +0x104) is the DigBoard that made the goal
DigBoard* DigGoal::Dig() const
{
	return static_cast<DigBoard*>(mBoard);
}

// ---- DigBoard ----

// 0x737420 (bejeweled3): TimeLimitBoard's constructor (0x6dd3b0: the bonus 10 and the penalty 3000), then the vftable and the strings (the mode's name "BuriedTreasure")
DigBoard::DigBoard()
{
}

// slot 85 (0x154), 0x737290 (bejeweled3): the quest's reset (0x6cda70), then the goal's state and the clock's are as at the start (+0x3274 is 0). port: the secret mode's
// +0x31c and +0x35c (the screen's offset: a scale of the art size) are not read
void DigBoard::ResetGame()
{
	QuestBoard::ResetGame();
	if (mGoal != nullptr)
		Goal()->Clear();
	mTimeLeftCap = 0;
	mTimeUp = false;
	mBonusShownTicks = 0;
	mLastBonusSeconds = 0;
	mDigDestroying = false;
	mDigScrolling = false;
}

// slot 135 (0x21c), 0x7375e0 (bejeweled3): TimeLimitBoard's (0x6e5030) reads Time, TimeBonus and TimePenalty, the quest's (0x6ddc70) makes the goal (0x6c4230 reads its
// settings), DigBoard's reads MegaTimeBonus, then the board's own new game (0x74e410) fills the grid and the goal makes the bricks of the eight rows. port: the first
// grid has a move: the fill made one, but the bricks may take it away, which the scroll's fix (0x6be3e0) is run for here as well
void DigBoard::NewGame()
{
	ReadTimeSettings();
	mMegaTimeBonus = SettingInt("MegaTimeBonus");
	CreateGoal();
	Goal()->Load();
	Board::NewGame();
	// 0x6ddc70's last part: a quest that does not record high scores shows no end dialog (a quest board is dealt as a secret mode is, but with the flags of QuestBoard::NewGame)
	if (!mRecordsHighScores)
		mEndScreenShown = true;
	Goal()->MakeRows(0);
	Goal()->EnsureMove();
}

// slot 22 (0x58), 0x7374d0 (bejeweled3): the quest's update, then the goal's
void DigBoard::Update()
{
	QuestBoard::Update();
}

// slot 234 (0x3a8), 0x6e5660 and slot 216 (0x360), 0x73b890 (bejeweled3): the step (the board's), then the progress is the score (0x6a97d0, the goal's slot 0x30, also
// does that, and slot 161 ends with it), and when the clock has run out (the time left is 0), nothing is going on (the board is at rest) and the game is on, the time's end is
// asked for with the flag +0xe28 set: slot 161
void DigBoard::UpdateTick()
{
	TimeLimitBoard::UpdateTick();
	mScorePoints = mLevelProgress;
	if (mBonusShownTicks > 0)
		mBonusShownTicks--;
}

// slot 214 (0x358), 0x736d60 (bejeweled3): the gravity (0x73dd70), then the bricks never fall: their speeds are 0 (the bricks are put at their cells before, so a gem above one
// lands on it)
void DigBoard::UpdateFalling()
{
	Goal()->PlaceBricks();
	Board::UpdateFalling();
	Goal()->PlaceBricks();
}

// slot 143 (0x23c): a cleared gem hits its neighbours (the goal's 0x6ad0d0), with nothing to do while the grid scrolls
void DigBoard::OnPieceCleared(Piece* thePiece)
{
	if (mDigScrolling)
		return;
	Goal()->OnPieceCleared(thePiece);
}

// slots 176 and 142: see DigGoal::OnPieceRemoving
bool DigBoard::OnPieceRemoving(Piece* thePiece)
{
	return Goal()->OnPieceRemoving(thePiece);
}

// a brick is hit by the goal's own hits: when it breaks it is taken out of the grid by the board's slot 176 (the hook above does not hit it again)
bool DigBoard::DigHit(Piece* thePiece, bool thePowered, bool theUseMask)
{
	if (!Goal()->DamageBrick(thePiece, thePowered, theUseMask))
		return false;
	mDigDestroying = true;
	RemovePiece(thePiece);
	mDigDestroying = false;
	return true;
}

// slot 161 (0x284), 0x737500 (bejeweled3): the score is the progress (+0xd24 = +0xe00), then the quest's (0x6d8e30: the board's, here)
void DigBoard::NoMoreMoves(bool theAnnounce)
{
	mScorePoints = mLevelProgress;
	QuestBoard::NoMoreMoves(theAnnounce);
}

// port: test only: the bot waits while the first six rows hold no dirt or the grid scrolls, so the board is at rest when the goal's update looks (a human waits for the
// scroll as well: it starts by itself)
bool DigBoard::BotMayMove() const
{
	return !Goal()->mScrolling && !Goal()->RowsAreFree(false);
}

// slot 152: the points of a match are not progress (the progress is the treasure's value, +0xe00)
void DigBoard::AddPoints(int thePoints, int theMoveId, bool theCountsForProgress, bool theUsesMultiplier)
{
	(void)theCountsForProgress;
	QuestBoard::AddPoints(thePoints, theMoveId, false, theUsesMultiplier);
}

// slot 257 (0x404): the end dialog of the mode (DigGoalEndLevelDialog, vftable 0x865c54)
ClassicEndLevelDialog* DigBoard::MakeEndLevelDialog()
{
	return new DigEndLevelDialog(this);
}

// slot 187 (0x2ec), 0x736b50 (bejeweled3): the clock's hundredths left, and no more than what the last bonus left (+0x3274, once there is one)
int DigBoard::GetTimeLeft() const
{
	const int aLeft = GetClockTimeLeft();
	if (mTimeLeftCap != 0)
		return std::min(aLeft, mTimeLeftCap);
	return aLeft;
}

// 0x736f20 (bejeweled3): the bonus (the mega one when the grid is empty of dirt: MegaTimeBonus, else TimeBonus) is added to the seconds, and what is left is kept
// (+0x3274): with less than 9001 hundredths the limit is put to 90 seconds and the ticks to 0x23f0 less what is left; with more, the limit is the seconds rounded up and
// the ticks are 200. port: the text "+%d SECOND MEGA BONUS" or "+%d SECOND BONUS" is not drawn as a text that rises: the overlay shows it for a while
void DigBoard::GiveTimeBonus(bool theMega)
{
	const int aBonus = theMega ? mMegaTimeBonus : mTimeBonus;
	mTimeSeconds += aBonus;
	mLastBonusSeconds = aBonus;
	mBonusWasMega = theMega;
	mBonusShownTicks = BONUS_SHOWN_TICKS;
	const int aLeft = GetClockTimeLeft();
	mTimeLeftCap = aLeft;
	if (aLeft < SHORT_CLOCK_HUNDREDTHS) {
		mTimeSeconds = BONUS_LIMIT_SECONDS;
		mGameTicks = BONUS_BASE_TICKS - aLeft;
	}
	else {
		mTimeSeconds = (aLeft + 99) / 100;
		mGameTicks = BONUS_TICKS_LONG;
	}
}

// slot 43 (0xac), 0x737370 (bejeweled3): the exe draws the dig bar (DrawDigBarAnim) and the board's overlay. port: the dirt, the gold, the diamonds and the artifacts are drawn plainly
// over their cells (dirt in brown, darker the stronger, with a dot for each hit left; gold, diamonds and artifacts as a coloured block), then the clock, the depth and the
// time bonus' seconds as text beside the score. The exe's effects (the dirt's crumbling, the treasure's sparkle and flight, the dig bar and its badges) are not made
void DigBoard::DrawModeOverlay(Graphics* g)
{
	for (int aRow = 0; aRow < Board::ROWS; aRow++) {
		for (int aColumn = 0; aColumn < Board::COLS; aColumn++) {
			const Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece == nullptr || (aPiece->mFlags & DigGoal::FLAG_BRICK) == 0)
				continue;
			auto aBrickIter = Goal()->mBricks.find(aPiece->mId);
			if (aBrickIter == Goal()->mBricks.end())
				continue;
			const DigGoal::Brick& aBrick = aBrickIter->second;
			const int aX = GetGridX() + (int)aPiece->mX;
			const int aY = GetGridY() + (int)aPiece->mY;
			int aRed = 0;
			int aGreen = 0;
			int aBlue = 0;
			if (aBrick.mKind == DigGoal::KIND_DIRT) {
				static const int SHADES[DigGoal::STRENGTH_DIRT_MAX + 1][3] = {{0, 0, 0}, {176, 130, 84}, {150, 108, 68}, {124, 88, 54}, {98, 70, 44}, {52, 40, 36}};
				const int* aShade = SHADES[std::min(std::max(aBrick.mHitsLeft, 1), (int)DigGoal::STRENGTH_DIRT_MAX)];
				aRed = aShade[0];
				aGreen = aShade[1];
				aBlue = aShade[2];
			}
			else if (aBrick.mKind == DigGoal::KIND_MONEY) {
				if (aBrick.mHitsInitial <= DigGoal::GOLD_STRENGTH_MAX) {
					aRed = 240;
					aGreen = 196;
					aBlue = 40;
				}
				else {
					aRed = 90;
					aGreen = 220;
					aBlue = 240;
				}
			}
			else {
				aRed = 190;
				aGreen = 96;
				aBlue = 220;
			}
			g->SetColor(Color(aRed, aGreen, aBlue, 255));
			g->FillRect(Scaled(aX + 4), Scaled(aY + 4), Scaled(CELL_SIZE - 8), Scaled(CELL_SIZE - 8));
			if (aBrick.mKind == DigGoal::KIND_DIRT) {
				g->SetColor(Color(255, 255, 255, 200));
				for (int aDot = 0; aDot < aBrick.mHitsLeft; aDot++)
					g->FillRect(Scaled(aX + 14 + aDot * 20), Scaled(aY + 14), Scaled(12), Scaled(12));
			}
			else {
				g->SetColor(Color(255, 255, 255, 220));
				g->FillRect(Scaled(aX + 40), Scaled(aY + 40), Scaled(48), Scaled(48));
			}
		}
	}
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	const int aLeft = GetTimeLeft();
	char aText[48];
	snprintf(aText, sizeof(aText), "%d:%02d", aLeft / 6000, (aLeft / 100) % 60);
	g->WriteString(aText, Scaled(250), Scaled(262), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "%d m", Goal()->DepthShown());
	g->WriteString(aText, Scaled(250), Scaled(322), 0, 0, true, 0, -1, -1);
	if (mBonusShownTicks > 0) {
		snprintf(aText, sizeof(aText), "+%d", mLastBonusSeconds);
		g->WriteString(aText, Scaled(250), Scaled(382), 0, 0, true, 0, -1, -1);
	}
}

}  // namespace Sexy
