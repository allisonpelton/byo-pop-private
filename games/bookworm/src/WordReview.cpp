#include "WordReview.h"
#include "Board.h"
#include "BookWormApp.h"
#include "ModFeatures.h"
#include "Piece.h"
#include "Solver.h"
#include "WordScore.h"

using namespace Sexy;

// The save trailer's tag and version
static const int REVIEW_TAG = 0x57455652;   // "REVW"
static const int REVIEW_VERSION = 1;

ReviewTotals::ReviewTotals()
{
	mScore = 0;
	mPar = 0;
	mWords = 0;
	mMissed = 0;
	mWorstScore = 0;
	mWorstBestScore = 0;
}

void ReviewTotals::Add(const SexyString& theWord, int theScore, const SexyString& theBest, int theBestScore)
{
	mScore += theScore;
	mPar += theBestScore;
	mWords++;
	int aMissed = theBestScore - theScore;
	mMissed += aMissed;
	if ((aMissed > 0) && (aMissed > mWorstBestScore - mWorstScore))
	{
		mWorstWord = theWord;
		mWorstScore = theScore;
		mWorstBest = theBest;
		mWorstBestScore = theBestScore;
	}
}

int ReviewTotals::ParPercent() const
{
	if ((mWords == 0) || (mPar <= 0))
		return -1;
	return (int)(((long long)mScore * 100 + mPar / 2) / mPar);
}

WordReview::WordReview(Board* theBoard, bool showMissed, bool showPar)
{
	mBoard = theBoard;
	mShowMissed = showMissed;
	mShowPar = showPar;
	mLevel = -1;
}

WordReview* WordReview::Create(Board* theBoard)
{
	bool showMissed = ModFeatureEnabled("missedWords");
	bool showPar = ModFeatureEnabled("par");
	return ((showMissed) || (showPar)) ? new WordReview(theBoard, showMissed, showPar) : NULL;
}

SexyString WordReview::WordPlayed(const WordScore& theWord)
{
	// the board's best word: the most points of every path over its lettered tiles (the solver's search)
	std::vector<Piece*> aTiles;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mBoard->mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z'))
				aTiles.push_back(aPiece);
		}
	}
	std::vector<SolverWord> aWords;
	FindSolverWords(aTiles, mBoard->mApp, &aWords);
	SexyString aBest = theWord.mWord;
	int aBestScore = theWord.mScore;
	for (size_t i = 0; i < aWords.size(); i++)
	{
		if (aWords[i].mScore > aBestScore)
		{
			aBest = aWords[i].mWord;
			aBestScore = aWords[i].mScore;
		}
	}

	if (mLevel != mBoard->mLevel)
	{
		mLevel = mBoard->mLevel;
		mLevelTotals = ReviewTotals();
	}
	mLevelTotals.Add(theWord.mWord, theWord.mScore, aBest, aBestScore);
	mGameTotals.Add(theWord.mWord, theWord.mScore, aBest, aBestScore);

	if (aBestScore <= theWord.mScore)
		return "The best word on the board!";
	SexyString aNote;
	if (mShowMissed)
		aNote = StrFormat("Best: %s (%s points)", aBest.c_str(), CommaSeperate(aBestScore).c_str());
	if ((mShowPar) && (aBestScore > 0))
		aNote += StrFormat("%s%d%% of par", aNote.empty() ? "" : "\n",
			(int)(((long long)theWord.mScore * 100 + aBestScore / 2) / aBestScore));
	return aNote;
}

const ReviewTotals& WordReview::LevelTotals() const
{
	static const ReviewTotals EMPTY;
	return (mLevel == mBoard->mLevel) ? mLevelTotals : EMPTY;
}

SexyString WordReview::PageLine(bool theLevel) const
{
	const ReviewTotals& aTotals = theLevel ? LevelTotals() : mGameTotals;
	if (aTotals.mWords == 0)
		return mShowPar ? "Par: no words" : "Missed: no words";
	SexyString aMissed = (aTotals.mMissed > 0) ? CommaSeperate(aTotals.mMissed) : SexyString("none");
	if (!mShowPar)
		return StrFormat("Missed: %s%s", aMissed.c_str(), (aTotals.mMissed > 0) ? " points" : "!");
	if (!mShowMissed)
		return StrFormat("Par: %d%%", aTotals.ParPercent());
	return StrFormat("Par %d%%, missed %s", aTotals.ParPercent(), aMissed.c_str());
}

static void WriteInt(FILE* theFile, int theValue)
{
	fwrite(&theValue, 4, 1, theFile);
}

static void WriteString(FILE* theFile, const SexyString& theString)
{
	WriteInt(theFile, (int)theString.size());
	if (!theString.empty())
		fwrite(theString.data(), 1, theString.size(), theFile);
}

static bool ReadInt(FILE* theFile, int* theValue)
{
	return fread(theValue, 4, 1, theFile) == 1;
}

static bool ReadString(FILE* theFile, SexyString* theString)
{
	int aLength = 0;
	if ((!ReadInt(theFile, &aLength)) || (aLength < 0) || (aLength > 64))
		return false;
	theString->assign((size_t)aLength, ' ');
	return (aLength == 0) || (fread(&(*theString)[0], 1, (size_t)aLength, theFile) == (size_t)aLength);
}

static void WriteTotals(FILE* theFile, const ReviewTotals& theTotals)
{
	WriteInt(theFile, theTotals.mScore);
	WriteInt(theFile, theTotals.mPar);
	WriteInt(theFile, theTotals.mWords);
	WriteInt(theFile, theTotals.mMissed);
	WriteString(theFile, theTotals.mWorstWord);
	WriteInt(theFile, theTotals.mWorstScore);
	WriteString(theFile, theTotals.mWorstBest);
	WriteInt(theFile, theTotals.mWorstBestScore);
}

static bool ReadTotals(FILE* theFile, ReviewTotals* theTotals)
{
	return (ReadInt(theFile, &theTotals->mScore)) && (ReadInt(theFile, &theTotals->mPar)) &&
		(ReadInt(theFile, &theTotals->mWords)) && (ReadInt(theFile, &theTotals->mMissed)) &&
		(ReadString(theFile, &theTotals->mWorstWord)) && (ReadInt(theFile, &theTotals->mWorstScore)) &&
		(ReadString(theFile, &theTotals->mWorstBest)) && (ReadInt(theFile, &theTotals->mWorstBestScore));
}

void WordReview::Write(FILE* theFile) const
{
	WriteInt(theFile, REVIEW_TAG);
	WriteInt(theFile, REVIEW_VERSION);
	WriteInt(theFile, mLevel);
	WriteTotals(theFile, mLevelTotals);
	WriteTotals(theFile, mGameTotals);
}

// false: no review there (a game saved before), or a damaged one; the totals then start empty
bool WordReview::Read(FILE* theFile)
{
	int aTag = 0;
	int aVersion = 0;
	int aLevel = -1;
	ReviewTotals aLevelTotals;
	ReviewTotals aGameTotals;
	long aStart = ftell(theFile);
	if ((!ReadInt(theFile, &aTag)) || (aTag != REVIEW_TAG) || (!ReadInt(theFile, &aVersion)) ||
		(aVersion != REVIEW_VERSION) || (!ReadInt(theFile, &aLevel)) || (!ReadTotals(theFile, &aLevelTotals)) ||
		(!ReadTotals(theFile, &aGameTotals)))
	{
		fseek(theFile, aStart, SEEK_SET);
		mLevel = -1;
		mLevelTotals = ReviewTotals();
		mGameTotals = ReviewTotals();
		return false;
	}
	mLevel = aLevel;
	mLevelTotals = aLevelTotals;
	mGameTotals = aGameTotals;
	return true;
}
