#include "Solver.h"
#include "Board.h"
#include "BookWormApp.h"
#include "ModFeatures.h"
#include "ModWords.h"
#include "Res.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Common.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

using namespace Sexy;

// No path longer than the dictionary's longest word can spell one: 12 letters for wordlist.txt (GAME_NOTES
// section 1), more with the mod "long-words" and a word list that has longer words (ModWords.h)

// The mod's settings (games/bookworm/mods/solver/mod.json), read live
static bool FireFirst()
{
	return ModFeatureBool("solver", "fireFirst", true);
}

static int BestCount()
{
	return ModFeatureInt("solver", "best", 10, 1, 30);
}

static bool ShowTag()
{
	return ModFeatureBool("solver", "tag", true);
}

static bool SaveBonus()
{
	return ModFeatureBool("solver", "saveBonus", false);
}

static bool FewerFire()
{
	return ModFeatureBool("solver", "fewerFire", false);
}

bool Sexy::SolverHotkeys()
{
	return ModFeatureBool("solver", "hotkeys", true);
}

void Sexy::FindBoardWords(const std::vector<Piece*>& theTiles, const std::set<SexyString>& theDictionary,
	const BoardWordVisitor& theVisitor)
{
	size_t aCount = theTiles.size();
	if ((aCount == 0) || (theDictionary.empty()))
		return;

	std::vector<std::vector<size_t> > aNeighbours(aCount);
	for (size_t i = 0; i < aCount; i++)
	{
		for (size_t j = 0; j < aCount; j++)
		{
			if ((i != j) && (Board::IsAdjacent(theTiles[i], theTiles[j])))
				aNeighbours[i].push_back(j);
		}
	}

	std::vector<bool> anInPath(aCount, false);
	std::vector<Piece*> aPath;
	SexyString aWord;
	int aMaxLength = ModWordMaxLength();
	std::function<void(size_t)> aFrom = [&](size_t theIdx)
	{
		Piece* aPiece = theTiles[theIdx];
		size_t aLength = aWord.size();
		aWord += aPiece->mLetter;
		if (aPiece->mLetter == 'Q')
			aWord += 'U';

		// the first word not before the letters so far starts with them, or no word does
		std::set<SexyString>::const_iterator anItr = theDictionary.lower_bound(aWord);
		if ((anItr != theDictionary.end()) && (anItr->compare(0, aWord.size(), aWord) == 0))
		{
			anInPath[theIdx] = true;
			aPath.push_back(aPiece);
			if ((aWord.size() >= 3) && (*anItr == aWord))
				theVisitor(aPath, aWord);
			if ((int)aWord.size() < aMaxLength)
			{
				for (size_t i = 0; i < aNeighbours[theIdx].size(); i++)
				{
					if (!anInPath[aNeighbours[theIdx][i]])
						aFrom(aNeighbours[theIdx][i]);
				}
			}
			aPath.pop_back();
			anInPath[theIdx] = false;
		}
		aWord.resize(aLength);
	};
	for (size_t i = 0; i < aCount; i++)
		aFrom(i);
}

Solver::Solver(Board* theBoard)
{
	mBoard = theBoard;
	mHintIdx = -1;
	mPending = false;
}

// In play, not paused, no summary, and every tile resting (none burning, none away from its place: tiles
// fall into the gaps over a few updates). The -autoplay bot waits for the same.
bool Solver::BoardAtRest(Board* theBoard)
{
	if ((theBoard->mState != BOARD_PLAYING) || (theBoard->mPauseCount > 0) || (theBoard->mSummaryShown) ||
		(!theBoard->mVisible) || (theBoard->mDisabled))
		return false;

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			if ((aPiece->mBurning) || (fabs(aPiece->mX - theBoard->ColX(aCol)) > 3.5) ||
				(fabs(aPiece->mY - theBoard->RowY(aCol, aRow)) > 3.5))
				return false;
		}
	}
	return true;
}

// Letters, tile types, the level (the points depend on it), the settings and the bonus word; for "fewerFire" the
// fire meter and the skip
std::string Solver::BoardSignature() const
{
	std::string aSig;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard->mBoard[aRow][aCol];
			aSig += (aPiece != NULL) ? aPiece->mLetter : '.';
			aSig += (aPiece != NULL) ? (char)('0' + aPiece->mType) : '.';
		}
	}
	aSig += StrFormat("|%d|%d%d%d%d|", mBoard->mLevel, (int)FireFirst(), BestCount(), (int)SaveBonus(), (int)FewerFire());
	if (FewerFire())
		aSig += StrFormat("%.4f %d %d|", mBoard->mFireMeter, (int)mBoard->mSkipRandomFire, (int)mBoard->RefillIsLevelsFirst());
	return aSig + mBoard->mBonusWord;
}

// Safer first, then the given order
static bool SaferThan(const SolverWord& a, const SolverWord& b)
{
	return a.mSafety < b.mSafety;
}

// The hint for the fire: takes the lowest fire tile, then the most fire tiles, then leaves the remaining
// fire highest, then the points
bool Sexy::SolverBetterForFire(const SolverWord& a, const SolverWord& b)
{
	if (a.mSafety != b.mSafety)
		return SaferThan(a, b);
	if (a.mFireRow != b.mFireRow)
		return a.mFireRow > b.mFireRow;
	if (a.mFireTiles != b.mFireTiles)
		return a.mFireTiles > b.mFireTiles;
	if (a.mFireLeftRow != b.mFireLeftRow)
		return a.mFireLeftRow < b.mFireLeftRow;
	return a.mScore > b.mScore;
}

bool Sexy::SolverBetterByScore(const SolverWord& a, const SolverWord& b)
{
	if (a.mSafety != b.mSafety)
		return SaferThan(a, b);
	if (a.mScore != b.mScore)
		return a.mScore > b.mScore;
	if (a.mFireLeftRow != b.mFireLeftRow)
		return a.mFireLeftRow < b.mFireLeftRow;
	return a.mWord.size() > b.mWord.size();
}

static bool BetterByLength(const SolverWord& a, const SolverWord& b)
{
	if (a.mSafety != b.mSafety)
		return SaferThan(a, b);
	if (a.mWord.size() != b.mWord.size())
		return a.mWord.size() > b.mWord.size();
	return a.mScore > b.mScore;
}

// The points and the fire of thePath, as Submit will have them (theFires: the board's fire tiles)
static bool IsBonusTile(const Piece* thePiece)
{
	return (thePiece->mType == TILE_GREEN) || (thePiece->mType == TILE_GOLD) || (thePiece->mType == TILE_SAPPHIRE) ||
		(thePiece->mType == TILE_DIAMOND);
}

static void RatePath(SolverWord* theWord, const std::vector<Piece*>& theFires, bool isClassic, WordScore* theScore,
	PieceList* theList)
{
	const std::vector<Piece*>& thePath = theWord->mPath;
	theList->assign(thePath.begin(), thePath.end());
	theScore->Build(theList);
	theWord->mScore = theScore->mScore;
	theWord->mFireTiles = 0;
	theWord->mFireRow = -1;
	theWord->mBonusTiles = 0;
	theWord->mNewFire = 0.0;
	theWord->mNewFireSure = 0;
	theWord->mFireCost = 0.0;
	for (size_t i = 0; i < thePath.size(); i++)
	{
		if (IsBonusTile(thePath[i]))
			theWord->mBonusTiles++;
		if (thePath[i]->mType == TILE_FIRE)
		{
			theWord->mFireTiles++;
			theWord->mFireRow = std::max(theWord->mFireRow, thePath[i]->mRow);
		}
	}
	theWord->mFireLeftRow = -1;
	theWord->mFireDrops = 0;
	theWord->mSafety = SOLVER_SAFE;
	for (size_t f = 0; f < theFires.size(); f++)
	{
		Piece* aFire = theFires[f];
		if (std::find(thePath.begin(), thePath.end(), aFire) != thePath.end())
			continue;
		if ((isClassic) && (aFire->mRow >= BOARD_ROWS - 1))
			theWord->mSafety = SOLVER_FATAL;
		int aFall = 0;
		for (size_t i = 0; i < thePath.size(); i++)
		{
			if ((thePath[i]->mCol == aFire->mCol) && (thePath[i]->mRow > aFire->mRow))
				aFall++;
		}
		if (aFall > 0)
			theWord->mFireDrops++;
		theWord->mFireLeftRow = std::max(theWord->mFireLeftRow, aFire->mRow + aFall);
	}
	if ((theWord->mSafety == SOLVER_SAFE) && (theWord->mFireLeftRow >= BOARD_ROWS - 1))
		theWord->mSafety = SOLVER_DANGER;
	ModWordEffects anEffects;
	if ((theWord->mSafety == SOLVER_SAFE) && (ModWordGetEffects(theWord->mWord, &anEffects)) &&
		((anEffects.mEveryTile) || (anEffects.mFire > 0)))
		theWord->mSafety = SOLVER_SETS_FIRE;
	theWord->mKind = SOLVER_HINT_BEST;
}

static void GetFires(const std::vector<Piece*>& theTiles, std::vector<Piece*>* theFires)
{
	for (size_t i = 0; i < theTiles.size(); i++)
	{
		if (theTiles[i]->mType == TILE_FIRE)
			theFires->push_back(theTiles[i]);
	}
}

void Sexy::FindSolverWords(const std::vector<Piece*>& theTiles, BookWormApp* theApp, std::vector<SolverWord>* theWords,
	int theLevel)
{
	bool isClassic = (theApp->mGameMode == 0);
	std::vector<Piece*> aFires;
	GetFires(theTiles, &aFires);

	WordScore aScore;
	aScore.mApp = theApp;
	aScore.mLevel = theLevel;
	PieceList aList;
	FindBoardWords(theTiles, theApp->mDictionary, [&](const std::vector<Piece*>& thePath, const SexyString& theWord)
	{
		SolverWord aWord;
		aWord.mWord = theWord;
		aWord.mPath = thePath;
		RatePath(&aWord, aFires, isClassic, &aScore, &aList);
		theWords->push_back(aWord);
	});
}

void Sexy::RateSolverWord(SolverWord* theWord, const std::vector<Piece*>& theTiles, BookWormApp* theApp, int theLevel)
{
	std::vector<Piece*> aFires;
	GetFires(theTiles, &aFires);
	WordScore aScore;
	aScore.mApp = theApp;
	aScore.mLevel = theLevel;
	PieceList aList;
	RatePath(theWord, aFires, theApp->mGameMode == 0, &aScore, &aList);
	theWord->mWord = aScore.mWord;
}

SolverFireRater::SolverFireRater(const std::vector<Piece*>& theTiles, bool isClassic, int theLevel, bool isLevelsFirst,
	double theMeter, bool isSkipping, const SexyString& theBonusWord) :
	mGrid(BOARD_ROWS * BOARD_COLS, NULL), mClassic(isClassic), mLevel(theLevel), mLevelsFirst(isLevelsFirst),
	mMeter(theMeter), mSkipping(isSkipping), mBonusWord(theBonusWord)
{
	for (Piece* aPiece : theTiles)
	{
		mGrid[aPiece->mRow * BOARD_COLS + aPiece->mCol] = aPiece;
		if (aPiece->mType == TILE_FIRE)
			mFires.push_back(aPiece);
	}
}

SolverFireRater SolverFireRater::ForBoard(Board* theBoard, const std::vector<Piece*>& theTiles)
{
	return SolverFireRater(theTiles, theBoard->mApp->mGameMode == GAMEMODE_CLASSIC, theBoard->mLevel,
		theBoard->RefillIsLevelsFirst(), theBoard->mFireMeter, theBoard->mSkipRandomFire, theBoard->mBonusWord);
}

int SolverFireRater::CostOrder(const SolverWord& theWord)
{
	return (int)std::lround(theWord.mFireCost * 100.0);
}

// The new fire tiles to expect from a refill: theHit the chance of the chance fire tile, isMeterDue the meter's;
// theOpen columns lost exactly one tile, each takes one due fire tile
static double PlacedFire(double theHit, bool isMeterDue, int theOpen)
{
	if (theOpen == 0)
		return 0.0;
	if (theOpen == 1)
		return isMeterDue ? 1.0 : theHit;
	return (isMeterDue ? 1.0 : 0.0) + theHit;
}

void SolverFireRater::Rate(SolverWord* theWord) const
{
	theWord->mNewFire = 0.0;
	theWord->mNewFireSure = 0;
	theWord->mFireCost = 0.0;
	if (!mClassic)
		return;

	const std::vector<Piece*>& aPath = theWord->mPath;
	double aChance = gFireChanceByLevel[((unsigned int)mLevel > 11) ? 12 : mLevel] / 100.0;
	double aRefill = gFireMeterRefill[((unsigned int)mLevel > 10) ? 11 : mLevel];

	// Submit's fire meter
	int aLength = (int)theWord->mWord.size();
	unsigned int aLengthIndex = (unsigned int)(aLength - 3);
	double aByLength = gFireMeterByLength[(aLengthIndex > 4) ? 5 : aLengthIndex];
	double aMeter = mMeter;
	if ((theWord->mFireTiles == 0) || (aByLength > 0.0))
		aMeter += aByLength;
	bool isSkipping = mSkipping;
	if ((mBonusWord.size() != 0) && (_stricmp(theWord->mWord.c_str(), mBonusWord.c_str()) == 0))
	{
		isSkipping = true;
		aMeter += aLength;
	}
	aMeter -= 1.0;

	// The word's refill: the chance roll, then the meter (not on the level's first refill); open columns are the
	// ones the word takes exactly one tile from
	int aTaken[BOARD_COLS] = {};
	for (Piece* aPiece : aPath)
		aTaken[aPiece->mCol]++;
	bool isMeterDue = (!mLevelsFirst) && (aMeter <= 0.0);
	int anOpen = (int)std::count(aTaken, aTaken + BOARD_COLS, 1);
	double aNewFire = PlacedFire(isSkipping ? 0.0 : aChance, isMeterDue, anOpen);
	int aSure = ((isMeterDue) && (anOpen > 0)) ? 1 : 0;
	if (isMeterDue)
		aMeter += aRefill;

	// BurnTiles: a fire tile the word leaves out burns the tile under it when that one's countdown runs out
	// (neither in the word); once they have burned away, a refill with their columns open
	int aBurned[BOARD_COLS] = {};
	bool hasBurns = false;
	for (Piece* aFire : mFires)
	{
		if ((aFire->mRow >= BOARD_ROWS - 1) || (std::find(aPath.begin(), aPath.end(), aFire) != aPath.end()))
			continue;
		Piece* aBelow = mGrid[(aFire->mRow + 1) * BOARD_COLS + aFire->mCol];
		if ((aBelow != NULL) && (aBelow->mBurnCountdown == 1) && (std::find(aPath.begin(), aPath.end(), aBelow) == aPath.end()))
		{
			aBurned[aFire->mCol]++;
			hasBurns = true;
		}
	}
	if (hasBurns)
	{
		// a skip the word's roll left is used up by this one
		bool isBurnMeterDue = aMeter <= 0.0;
		int aBurnOpen = (int)std::count(aBurned, aBurned + BOARD_COLS, 1);
		aNewFire += PlacedFire(isSkipping ? aChance * aChance : aChance, isBurnMeterDue, aBurnOpen);
		if ((isBurnMeterDue) && (aBurnOpen > 0))
			aSure++;
		if (isBurnMeterDue)
			aMeter += aRefill;
	}

	theWord->mNewFire = aNewFire;
	theWord->mNewFireSure = aSure;
	// the meter's drain counts as fire to come: each refill of it goes with a fire tile, landed or dropped
	theWord->mFireCost = aNewFire + (mMeter - aMeter) / aRefill;
}

void Solver::Solve()
{
	mHints.clear();
	mHintIdx = -1;
	mBoardSig = BoardSignature();

	std::vector<Piece*> aTiles;
	bool hasFire = false;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard->mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mLetter < 'A') || (aPiece->mLetter > 'Z'))
				continue;
			aTiles.push_back(aPiece);
			if (aPiece->mType == TILE_FIRE)
				hasFire = true;
		}
	}

	// "saveBonus": after safety (and for the fire hint the lowest fire tile), a path without bonus tiles first
	bool isSaving = SaveBonus();
	auto aSavesFirst = [isSaving](const SolverWord& a, const SolverWord& b, bool* theResult)
	{
		if ((!isSaving) || (a.mSafety != b.mSafety) || ((a.mBonusTiles == 0) == (b.mBonusTiles == 0)))
			return false;
		*theResult = (a.mBonusTiles == 0);
		return true;
	};
	// "fewerFire": before that, the fewest new fire tiles (SolverFireRater's cost); not for the longest word
	bool isFewer = FewerFire();
	auto aFewerFirst = [isFewer](const SolverWord& a, const SolverWord& b, bool* theResult)
	{
		if ((!isFewer) || (a.mSafety != b.mSafety) || (SolverFireRater::CostOrder(a) == SolverFireRater::CostOrder(b)))
			return false;
		*theResult = SolverFireRater::CostOrder(a) < SolverFireRater::CostOrder(b);
		return true;
	};
	auto aBetterByScore = [&](const SolverWord& a, const SolverWord& b)
	{
		bool aResult;
		if ((aFewerFirst(a, b, &aResult)) || (aSavesFirst(a, b, &aResult)))
			return aResult;
		return SolverBetterByScore(a, b);
	};
	auto aBetterForFire = [&](const SolverWord& a, const SolverWord& b)
	{
		bool aResult;
		if ((a.mFireRow == b.mFireRow) && ((aFewerFirst(a, b, &aResult)) || (aSavesFirst(a, b, &aResult))))
			return aResult;
		return SolverBetterForFire(a, b);
	};
	auto aBetterByLength = [&](const SolverWord& a, const SolverWord& b)
	{
		bool aResult;
		return aSavesFirst(a, b, &aResult) ? aResult : BetterByLength(a, b);
	};

	// Every path, with its points and what it does to the fire
	std::vector<SolverWord> aWords;
	FindSolverWords(aTiles, mBoard->mApp, &aWords);
	if (aWords.empty())
		return;
	if (isFewer)
	{
		SolverFireRater aRater = SolverFireRater::ForBoard(mBoard, aTiles);
		for (SolverWord& aWord : aWords)
			aRater.Rate(&aWord);
	}

	// One path per word for the lists by points and length: its best by points
	std::map<SexyString, size_t> aBestPath;
	for (size_t i = 0; i < aWords.size(); i++)
	{
		std::map<SexyString, size_t>::iterator anItr = aBestPath.find(aWords[i].mWord);
		if (anItr == aBestPath.end())
			aBestPath[aWords[i].mWord] = i;
		else if (aBetterByScore(aWords[i], aWords[anItr->second]))
			anItr->second = i;
	}
	std::vector<SolverWord> aDistinct;
	for (std::map<SexyString, size_t>::iterator anItr = aBestPath.begin(); anItr != aBestPath.end(); ++anItr)
		aDistinct.push_back(aWords[anItr->second]);

	std::set<SexyString> aListed;
	if ((FireFirst()) && (hasFire))
	{
		const SolverWord* aBest = NULL;
		for (size_t i = 0; i < aWords.size(); i++)
		{
			if ((aWords[i].mFireTiles > 0) && ((aBest == NULL) || (aBetterForFire(aWords[i], *aBest))))
				aBest = &aWords[i];
		}
		if (aBest != NULL)
		{
			mHints.push_back(*aBest);
			mHints.back().mKind = SOLVER_HINT_FIRE;
			aListed.insert(aBest->mWord);
		}
	}

	if (mBoard->mBonusWord.size() != 0)
	{
		for (size_t i = 0; i < aDistinct.size(); i++)
		{
			if ((_stricmp(aDistinct[i].mWord.c_str(), mBoard->mBonusWord.c_str()) == 0) &&
				(aListed.insert(aDistinct[i].mWord).second))
			{
				mHints.push_back(aDistinct[i]);
				mHints.back().mKind = SOLVER_HINT_BONUS;
			}
		}
	}

	std::stable_sort(aDistinct.begin(), aDistinct.end(), aBetterByScore);
	int aBestLeft = BestCount();
	for (size_t i = 0; (i < aDistinct.size()) && (aBestLeft > 0); i++)
	{
		if (!aListed.insert(aDistinct[i].mWord).second)
			continue;
		mHints.push_back(aDistinct[i]);
		mHints.back().mKind = SOLVER_HINT_BEST;
		aBestLeft--;
	}

	// "saveBonus": the best word that spends the saved bonus tiles (any of its paths that uses them)
	if (isSaving)
	{
		const SolverWord* aCashIn = NULL;
		for (size_t i = 0; i < aWords.size(); i++)
		{
			if ((aWords[i].mBonusTiles > 0) && ((aCashIn == NULL) || (SolverBetterByScore(aWords[i], *aCashIn))))
				aCashIn = &aWords[i];
		}
		if (aCashIn != NULL)
		{
			mHints.push_back(*aCashIn);
			mHints.back().mKind = SOLVER_HINT_CASHIN;
			aListed.insert(aCashIn->mWord);
		}
	}

	std::stable_sort(aDistinct.begin(), aDistinct.end(), aBetterByLength);
	if (aListed.insert(aDistinct[0].mWord).second)
	{
		mHints.push_back(aDistinct[0]);
		mHints.back().mKind = SOLVER_HINT_LONGEST;
	}
}

bool Solver::HintShown() const
{
	if ((mHintIdx < 0) || (mHintIdx >= (int)mHints.size()))
		return false;
	const std::vector<Piece*>& aPath = mHints[mHintIdx].mPath;
	if (mBoard->mSelection.size() != aPath.size())
		return false;
	return std::equal(mBoard->mSelection.begin(), mBoard->mSelection.end(), aPath.begin());
}

const SolverWord* Solver::ShownHint() const
{
	return HintShown() ? &mHints[mHintIdx] : NULL;
}

void Solver::SelectHint(int theIdx)
{
	mHintIdx = theIdx;
	mBoard->SelectTiles(mHints[theIdx].mPath);
}

void Solver::Reset()
{
	mHints.clear();
	mHintIdx = -1;
	mBoardSig.clear();
	mPending = false;
}

bool Solver::IsCurrent() const
{
	return (!mBoardSig.empty()) && (BoardSignature() == mBoardSig);
}

void Solver::Refresh()
{
	if ((BoardAtRest(mBoard)) && (!IsCurrent()))
		Solve();
}

void Solver::SelectListed(int theIdx)
{
	if ((theIdx < 0) || (theIdx >= (int)mHints.size()) || (!BoardAtRest(mBoard)) || (!IsCurrent()))
		return;
	mPending = false;
	SelectHint(theIdx);
}

void Solver::PlayListed(int theIdx)
{
	if ((mBoard->mState != BOARD_PLAYING) || (mBoard->mPauseCount > 0) || (!BoardAtRest(mBoard)))
		return;
	Refresh();
	if ((theIdx < 0) || (theIdx >= (int)mHints.size()))
	{
		mBoard->mApp->PlaySample(SOUND_BUZZER);
		return;
	}
	mPending = false;
	SelectHint(theIdx);
	mBoard->ButtonDepress(BOARD_BUTTON_SUBMIT);
}

void Solver::RequestHint()
{
	mPending = true;
	Update();
}

void Solver::Update()
{
	if ((!mPending) || (!BoardAtRest(mBoard)))
		return;
	mPending = false;

	// The next hint while the selection shows one of this board's, else the first of a fresh list
	if ((BoardSignature() == mBoardSig) && (HintShown()))
	{
		SelectHint((mHintIdx + 1) % (int)mHints.size());
		return;
	}
	Solve();
	if (mHints.empty())
	{
		mBoard->mApp->PlaySample(SOUND_BUZZER);
		return;
	}
	SelectHint(0);
}

// A dark tag beside the hint's tiles: what the hint is and where it stands in the cycle, and a warning for a
// word that is not safe
void Solver::Draw(Graphics* g)
{
	if ((!ShowTag()) || (!HintShown()))
		return;

	const SolverWord& aHint = mHints[mHintIdx];
	const char* aKind = "BEST";
	if (aHint.mKind == SOLVER_HINT_FIRE)
		aKind = "PUTS OUT FIRE";
	else if (aHint.mKind == SOLVER_HINT_BONUS)
		aKind = "BONUS WORD";
	else if (aHint.mKind == SOLVER_HINT_LONGEST)
		aKind = "LONGEST";
	else if (aHint.mKind == SOLVER_HINT_CASHIN)
		aKind = "CASH IN";
	SexyString aText = StrFormat("%s  %d/%d", aKind, mHintIdx + 1, (int)mHints.size());
	if ((SaveBonus()) && (aHint.mBonusTiles > 0))
		aText += StrFormat("  %d BONUS TILE%s", aHint.mBonusTiles, (aHint.mBonusTiles == 1) ? "" : "S");
	if ((FewerFire()) && (mBoard->mApp->mGameMode == GAMEMODE_CLASSIC))
	{
		// the meter's fire tiles are certain, the rest a chance
		int aChance = (int)std::lround((aHint.mNewFire - aHint.mNewFireSure) * 100.0);
		if ((aHint.mNewFireSure == 0) && (aChance == 0))
			aText += "  NO NEW FIRE";
		else if (aHint.mNewFireSure == 0)
			aText += StrFormat("  NEW FIRE %d%%", aChance);
		else if (aChance == 0)
			aText += StrFormat("  NEW FIRE %d", aHint.mNewFireSure);
		else
			aText += StrFormat("  NEW FIRE %d + %d%%", aHint.mNewFireSure, aChance);
	}
	if (aHint.mSafety == SOLVER_FATAL)
		aText += "  ENDS THE GAME";
	else if (aHint.mSafety == SOLVER_DANGER)
		aText += "  FIRE TO THE BOTTOM";
	else if (aHint.mSafety == SOLVER_SETS_FIRE)
		aText += "  SETS FIRE";

	Font* aFont = mBoard->mApp->mFonts[FONT_VERDANA10BOLD];
	int aWidth = aFont->StringWidth(aText) + 12;
	int aHeight = aFont->GetAscent() + 8;
	// Above or below a tile of the word, the first tile first, where the tag covers none of the word's tiles
	// and arrows (else at the first place that fits); inside the board's tile area (x 230..638)
	std::vector<Rect> aPathRects;
	for (Piece* aPiece : aHint.mPath)
		aPathRects.push_back(Rect((int)aPiece->mX, (int)aPiece->mY, 57, 57).Inflate(2, 2));
	int aX = 0;
	int aY = 0;
	bool aPlaced = false;
	bool aClear = false;
	for (size_t i = 0; (i < aHint.mPath.size() * 2) && (!aClear); i++)
	{
		Piece* aPiece = aHint.mPath[i / 2];
		Rect aTag(std::max(230, std::min((int)aPiece->mX + 28 - aWidth / 2, 638 - aWidth)),
			(i % 2 == 0) ? (int)aPiece->mY - aHeight - 3 : (int)aPiece->mY + 57 + 3, aWidth, aHeight);
		if ((aTag.mY < 4) || (aTag.mY + aHeight > 476))
			continue;
		aClear = true;
		for (const Rect& aRect : aPathRects)
			aClear = aClear && !aRect.Intersects(aTag);
		if ((aClear) || (!aPlaced))
		{
			aX = aTag.mX;
			aY = aTag.mY;
			aPlaced = true;
		}
	}

	g->SetColor(Color(0, 0, 0, 190));
	g->FillRect(aX, aY, aWidth, aHeight);
	g->SetColor((aHint.mSafety == SOLVER_SAFE) ? Color(255, 230, 120) : Color(255, 120, 80));
	g->DrawRect(aX, aY, aWidth - 1, aHeight - 1);
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(aText, aX + 6, aY + 4 + aFont->GetAscent());
}
