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

// wordlist.txt keeps words of 3..12 letters (GAME_NOTES section 1), so no longer path can spell one
static const int SOLVER_MAX_WORD_LENGTH = 12;

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
			if ((int)aWord.size() < SOLVER_MAX_WORD_LENGTH)
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

// Letters, tile types, the level (the points depend on it) and the bonus word
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
	return aSig + StrFormat("|%d|", mBoard->mLevel) + mBoard->mBonusWord;
}

// Safer first, then the given order
static bool SaferThan(const SolverWord& a, const SolverWord& b)
{
	return a.mSafety < b.mSafety;
}

// The hint for the fire: takes the lowest fire tile, then the most fire tiles, then leaves the remaining
// fire highest, then the points
static bool BetterForFire(const SolverWord& a, const SolverWord& b)
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

static bool BetterByScore(const SolverWord& a, const SolverWord& b)
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

void Solver::Solve()
{
	mHints.clear();
	mHintIdx = -1;
	mBoardSig = BoardSignature();

	BookWormApp* anApp = mBoard->mApp;
	bool isClassic = (anApp->mGameMode == 0);

	std::vector<Piece*> aTiles;
	std::vector<Piece*> aFires;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard->mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mLetter < 'A') || (aPiece->mLetter > 'Z'))
				continue;
			aTiles.push_back(aPiece);
			if (aPiece->mType == TILE_FIRE)
				aFires.push_back(aPiece);
		}
	}

	// Every path, with its points and what it does to the fire
	std::vector<SolverWord> aWords;
	WordScore aScore;
	aScore.mApp = anApp;
	PieceList aList;
	FindBoardWords(aTiles, anApp->mDictionary, [&](const std::vector<Piece*>& thePath, const SexyString& theWord)
	{
		SolverWord aWord;
		aWord.mWord = theWord;
		aWord.mPath = thePath;
		aList.assign(thePath.begin(), thePath.end());
		aScore.Build(&aList);
		aWord.mScore = aScore.mScore;
		aWord.mFireTiles = 0;
		aWord.mFireRow = -1;
		for (size_t i = 0; i < thePath.size(); i++)
		{
			if (thePath[i]->mType == TILE_FIRE)
			{
				aWord.mFireTiles++;
				aWord.mFireRow = std::max(aWord.mFireRow, thePath[i]->mRow);
			}
		}
		aWord.mFireLeftRow = -1;
		aWord.mSafety = SOLVER_SAFE;
		for (size_t f = 0; f < aFires.size(); f++)
		{
			Piece* aFire = aFires[f];
			if (std::find(thePath.begin(), thePath.end(), aFire) != thePath.end())
				continue;
			if ((isClassic) && (aFire->mRow >= BOARD_ROWS - 1))
				aWord.mSafety = SOLVER_FATAL;
			int aFall = 0;
			for (size_t i = 0; i < thePath.size(); i++)
			{
				if ((thePath[i]->mCol == aFire->mCol) && (thePath[i]->mRow > aFire->mRow))
					aFall++;
			}
			aWord.mFireLeftRow = std::max(aWord.mFireLeftRow, aFire->mRow + aFall);
		}
		if ((aWord.mSafety == SOLVER_SAFE) && (aWord.mFireLeftRow >= BOARD_ROWS - 1))
			aWord.mSafety = SOLVER_DANGER;
		if ((aWord.mSafety == SOLVER_SAFE) && (ModWordFire(theWord) > 0))
			aWord.mSafety = SOLVER_SETS_FIRE;
		aWord.mKind = SOLVER_HINT_BEST;
		aWords.push_back(aWord);
	});
	if (aWords.empty())
		return;

	// One path per word for the lists by points and length: its best by points
	std::map<SexyString, size_t> aBestPath;
	for (size_t i = 0; i < aWords.size(); i++)
	{
		std::map<SexyString, size_t>::iterator anItr = aBestPath.find(aWords[i].mWord);
		if (anItr == aBestPath.end())
			aBestPath[aWords[i].mWord] = i;
		else if (BetterByScore(aWords[i], aWords[anItr->second]))
			anItr->second = i;
	}
	std::vector<SolverWord> aDistinct;
	for (std::map<SexyString, size_t>::iterator anItr = aBestPath.begin(); anItr != aBestPath.end(); ++anItr)
		aDistinct.push_back(aWords[anItr->second]);

	std::set<SexyString> aListed;
	if ((FireFirst()) && (!aFires.empty()))
	{
		const SolverWord* aBest = NULL;
		for (size_t i = 0; i < aWords.size(); i++)
		{
			if ((aWords[i].mFireTiles > 0) && ((aBest == NULL) || (BetterForFire(aWords[i], *aBest))))
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

	std::stable_sort(aDistinct.begin(), aDistinct.end(), BetterByScore);
	int aBestLeft = BestCount();
	for (size_t i = 0; (i < aDistinct.size()) && (aBestLeft > 0); i++)
	{
		if (!aListed.insert(aDistinct[i].mWord).second)
			continue;
		mHints.push_back(aDistinct[i]);
		mHints.back().mKind = SOLVER_HINT_BEST;
		aBestLeft--;
	}

	std::stable_sort(aDistinct.begin(), aDistinct.end(), BetterByLength);
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
	SexyString aText = StrFormat("%s  %d/%d", aKind, mHintIdx + 1, (int)mHints.size());
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
