#include "StatKeeper.h"
#include "WordScore.h"

#include <cstdint>

using namespace Sexy;

// StatKeeper.cpp (in the original part of Board.cpp): the statistics of a game or of a level.
// Spec: re/bookworm/analysis/board_logic.md 2.4 (behaviour) and 6.3 (record).

// Record helpers: little-endian raw fields as the original's fwrite/fread. A field the file does not hold
// (a truncated file) keeps its value, as with the original's fread.
static void WriteInt(FILE* theFile, int theValue)
{
	fwrite(&theValue, 1, 4, theFile);
}

static void ReadInt(FILE* theFile, int* theValue)
{
	fread(theValue, 1, 4, theFile);
}

static void WriteString(FILE* theFile, const SexyString& theString)
{
	uint32_t aLength = (uint32_t)theString.size();
	fwrite(&aLength, 1, 4, theFile);
	fwrite(theString.data(), 1, aLength, theFile);
}

static void ReadString(FILE* theFile, SexyString* theString)
{
	uint32_t aLength = 0;
	fread(&aLength, 1, 4, theFile);
	theString->resize(aLength, '\0');
	if (aLength > 0)
		fread(&(*theString)[0], 1, aLength, theFile);
}

// 0x408690
StatKeeper::StatKeeper(BookWormApp* theApp)
{
	mApp = theApp;
	// port: the original never initialises these two (heap garbage, also not cleared by Reset); nothing
	// shows them (NAMES.md D39)
	mSapphireTilesUsed = 0;
	mDiamondTilesUsed = 0;
	Reset();
}

// 0x405140 (scalar deleting dtor 0x4059c0)
StatKeeper::~StatKeeper()
{
}

// 0x406950: everything but the sapphire and diamond counters
void StatKeeper::Reset()
{
	mScore = 0;
	mWords = 0;
	mLetters = 0;
	mTiles = 0;
	mFireTilesUsed = 0;
	mGreenTilesUsed = 0;
	mGoldTilesUsed = 0;
	mTilesBurned = 0;
	mScrambles = 0;
	mBonusWords = 0;
	mBestWord = "";
	mBestWordScore = 0;
	mLongestWord = "";
	mLongestWordScore = 0;
}

// 0x4068a0: the longest and the best word keep the first of equals (strict compares)
void StatKeeper::AddWord(const WordScore& theWord)
{
	mWords++;
	mScore += theWord.mScore;
	mLetters += (int)theWord.mWord.size();
	mTiles += theWord.mNumPieces;
	mGreenTilesUsed += theWord.mTypeCount[TILE_GREEN];
	mGoldTilesUsed += theWord.mTypeCount[TILE_GOLD];
	mFireTilesUsed += theWord.mTypeCount[TILE_FIRE];
	mSapphireTilesUsed += theWord.mTypeCount[TILE_SAPPHIRE];
	mDiamondTilesUsed += theWord.mTypeCount[TILE_DIAMOND];

	if (mLongestWord.size() < theWord.mWord.size())
	{
		mLongestWord = theWord.mWord;
		mLongestWordScore = theWord.mScore;
	}

	if (mBestWordScore < theWord.mScore)
	{
		mBestWord = theWord.mWord;
		mBestWordScore = theWord.mScore;
	}
}

// 0x403690
void StatKeeper::AddBonusWord(int thePoints)
{
	mScore += thePoints;
	mBonusWords++;
}

// 0x4036b0
void StatKeeper::IncBurned()
{
	mTilesBurned++;
}

// 0x4036c0
void StatKeeper::IncScrambles()
{
	mScrambles++;
}

// 0x404e10: 12 ints (+0x08..+0x34), best word, its score, longest word, its score
void StatKeeper::Save(FILE* theFile)
{
	WriteInt(theFile, mScore);
	WriteInt(theFile, mWords);
	WriteInt(theFile, mLetters);
	WriteInt(theFile, mTiles);
	WriteInt(theFile, mFireTilesUsed);
	WriteInt(theFile, mGreenTilesUsed);
	WriteInt(theFile, mGoldTilesUsed);
	WriteInt(theFile, mSapphireTilesUsed);
	WriteInt(theFile, mDiamondTilesUsed);
	WriteInt(theFile, mTilesBurned);
	WriteInt(theFile, mScrambles);
	WriteInt(theFile, mBonusWords);
	WriteString(theFile, mBestWord);
	WriteInt(theFile, mBestWordScore);
	WriteString(theFile, mLongestWord);
	WriteInt(theFile, mLongestWordScore);
}

// 0x405fa0
void StatKeeper::Load(FILE* theFile)
{
	ReadInt(theFile, &mScore);
	ReadInt(theFile, &mWords);
	ReadInt(theFile, &mLetters);
	ReadInt(theFile, &mTiles);
	ReadInt(theFile, &mFireTilesUsed);
	ReadInt(theFile, &mGreenTilesUsed);
	ReadInt(theFile, &mGoldTilesUsed);
	ReadInt(theFile, &mSapphireTilesUsed);
	ReadInt(theFile, &mDiamondTilesUsed);
	ReadInt(theFile, &mTilesBurned);
	ReadInt(theFile, &mScrambles);
	ReadInt(theFile, &mBonusWords);
	ReadString(theFile, &mBestWord);
	ReadInt(theFile, &mBestWordScore);
	ReadString(theFile, &mLongestWord);
	ReadInt(theFile, &mLongestWordScore);
}
