// port(mod): mod "type-word", typing a word (TypedWord.h). Not part of the original game.

#include "TypedWord.h"
#include "Board.h"
#include "BookWormApp.h"
#include "Piece.h"
#include "Res.h"
#include "Solver.h"
#include <algorithm>
#include <cctype>
#include <functional>

using namespace Sexy;

// Paths searched for the letters typed at most (a short text on a board of repeated letters has many)
static const size_t TYPED_MAX_PATHS = 5000;

TypedWord::TypedWord(Board* theBoard)
{
	mBoard = theBoard;
}

void TypedWord::Reset()
{
	mTyped.clear();
	mPath.clear();
}

// Keys reach the board in play only; Ctrl+letter is the board's (Ctrl+Z: undo)
bool TypedWord::TakesKeys() const
{
	return (mBoard->mState == BOARD_PLAYING) && (mBoard->mPauseCount == 0) && (!mBoard->mSummaryShown) &&
		(mBoard->mVisible) && (!mBoard->mDisabled) && (!mBoard->mCtrlDown);
}

// The selection is still the typed letters' (not a click's, a hint's, or gone with a word played)
bool TypedWord::SelectionIsTyped() const
{
	if ((mTyped.empty()) || (mBoard->mSelection.size() != mPath.size()))
		return false;
	return std::equal(mPath.begin(), mPath.end(), mBoard->mSelection.begin());
}

void TypedWord::Select(const SexyString& theText, const SolverWord& thePath)
{
	mTyped = theText;
	mPath = thePath.mPath;
	mBoard->SelectTiles(mPath);
}

// As a right click on the selection
void TypedWord::Clear()
{
	if (SelectionIsTyped())
	{
		std::for_each(mBoard->mSelection.begin(), mBoard->mSelection.end(), Piece::Deselect);
		mBoard->ClearWord();
	}
	Reset();
}

// The letters thePiece spells at thePos of theText: its letter, "QU" for the Qu tile, or "Q" when that is the
// last letter typed so far; 0 when it does not fit
static int TileFits(const Piece* thePiece, const SexyString& theText, size_t thePos)
{
	if (theText[thePos] != thePiece->mLetter)
		return 0;
	if (thePiece->mLetter != 'Q')
		return 1;
	if (thePos + 1 == theText.size())
		return 1;
	return (theText[thePos + 1] == 'U') ? 2 : 0;
}

// Every path over theTiles (adjacent tiles, each at most once) that spells theText
static void FindTextPaths(const std::vector<Piece*>& theTiles, const SexyString& theText,
	std::vector<std::vector<Piece*> >* thePaths)
{
	size_t aCount = theTiles.size();
	std::vector<std::vector<size_t> > aNext(aCount);
	for (size_t i = 0; i < aCount; i++)
	{
		for (size_t j = 0; j < aCount; j++)
		{
			if ((i != j) && (Board::IsAdjacent(theTiles[i], theTiles[j])))
				aNext[i].push_back(j);
		}
	}

	std::vector<size_t> aPath;
	std::vector<bool> aUsed(aCount, false);
	std::function<void(size_t)> aVisit = [&](size_t thePos)
	{
		if (thePos == theText.size())
		{
			std::vector<Piece*> aPieces;
			for (size_t i = 0; i < aPath.size(); i++)
				aPieces.push_back(theTiles[aPath[i]]);
			thePaths->push_back(aPieces);
			return;
		}

		size_t aChoices = aPath.empty() ? aCount : aNext[aPath.back()].size();
		for (size_t c = 0; (c < aChoices) && (thePaths->size() < TYPED_MAX_PATHS); c++)
		{
			size_t aTile = aPath.empty() ? c : aNext[aPath.back()][c];
			if (aUsed[aTile])
				continue;
			int aTaken = TileFits(theTiles[aTile], theText, thePos);
			if (aTaken == 0)
				continue;
			aUsed[aTile] = true;
			aPath.push_back(aTile);
			aVisit(thePos + aTaken);
			aPath.pop_back();
			aUsed[aTile] = false;
		}
	};
	if (!theText.empty())
		aVisit(0);
}

// True: a is the better path for the typed letters. A path that ends the game comes last; then the most fire
// tiles taken, the fewest dropped, none landing on the bottom row, the most points.
bool TypedWord::Better(const SolverWord& theFirst, const SolverWord& theSecond)
{
	bool aFatal = theFirst.mSafety == SOLVER_FATAL;
	bool bFatal = theSecond.mSafety == SOLVER_FATAL;
	if (aFatal != bFatal)
		return bFatal;
	if (theFirst.mFireTiles != theSecond.mFireTiles)
		return theFirst.mFireTiles > theSecond.mFireTiles;
	if (theFirst.mFireDrops != theSecond.mFireDrops)
		return theFirst.mFireDrops < theSecond.mFireDrops;
	bool aDanger = theFirst.mSafety == SOLVER_DANGER;
	bool bDanger = theSecond.mSafety == SOLVER_DANGER;
	if (aDanger != bDanger)
		return bDanger;
	return theFirst.mScore > theSecond.mScore;
}

bool TypedWord::FindPath(const SexyString& theText, SolverWord* theBest) const
{
	// the lettered tiles at their places in the grid (falling ones too), not those burning away
	std::vector<Piece*> aTiles;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard->mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z') && (!aPiece->mBurning))
				aTiles.push_back(aPiece);
		}
	}

	std::vector<std::vector<Piece*> > aPaths;
	FindTextPaths(aTiles, theText, &aPaths);
	bool isFound = false;
	for (size_t i = 0; i < aPaths.size(); i++)
	{
		SolverWord aWord;
		aWord.mPath = aPaths[i];
		RateSolverWord(&aWord, aTiles, mBoard->mApp);
		if ((!isFound) || (Better(aWord, *theBest)))
			*theBest = aWord;
		isFound = true;
	}
	return isFound;
}

bool TypedWord::KeyChar(SexyChar theChar)
{
	if (!TakesKeys())
		return false;
	SexyChar aChar = (SexyChar)toupper((unsigned char)theChar);
	if ((aChar < 'A') || (aChar > 'Z'))
		return false;

	SexyString aText = (SelectionIsTyped() ? mTyped : SexyString()) + aChar;
	SolverWord aBest;
	if (!FindPath(aText, &aBest))
	{
		mBoard->mApp->PlaySample(SOUND_BUZZER);
		return true;
	}
	Select(aText, aBest);
	return true;
}

bool TypedWord::KeyDown(KeyCode theKey)
{
	if (!TakesKeys())
		return false;

	// Enter: the selection submitted, as the Submit button does (any selection, a clicked one too)
	if (theKey == KEYCODE_RETURN)
	{
		if (mBoard->mSelection.empty())
			return true;
		mBoard->ButtonDepress(BOARD_BUTTON_SUBMIT);
		if (mBoard->mSelection.empty())
			Reset();
		return true;
	}

	if (!SelectionIsTyped())
		return false;
	if (theKey == KEYCODE_ESCAPE)
	{
		Clear();
		return true;
	}
	if (theKey == KEYCODE_BACK)
	{
		SexyString aText = mTyped.substr(0, mTyped.size() - 1);
		SolverWord aBest;
		if ((aText.empty()) || (!FindPath(aText, &aBest)))
			Clear();
		else
			Select(aText, aBest);
		return true;
	}
	return false;
}
