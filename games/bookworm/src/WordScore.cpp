#include "WordScore.h"
#include "Board.h"
#include "BookWormApp.h"
#include "ModWords.h"

#include <cstdint>

using namespace Sexy;

// WordScore.cpp (in the original part of Board.cpp): the word a list of tiles spells and its score.
// Spec: re/bookworm/analysis/board_logic.md 2.3 (behaviour) and 6.3 (record).

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

static void WriteBool(FILE* theFile, bool theValue)
{
	unsigned char aByte = theValue ? 1 : 0;
	fwrite(&aByte, 1, 1, theFile);
}

static void ReadBool(FILE* theFile, bool* theValue)
{
	unsigned char aByte = *theValue ? 1 : 0;
	fread(&aByte, 1, 1, theFile);
	*theValue = aByte != 0;
}

// 0x405190
WordScore::WordScore()
{
	mApp = NULL;
	mValid = false;
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		mTypeCount[i] = 0;
	mNumPieces = 0;
	mScore = 0;
	mLevel = -1;
}

// 0x406560 (only BoardDraw::DrawWord copies one)
WordScore::WordScore(const WordScore& theOther)
{
	mApp = theOther.mApp;
	mWord = theOther.mWord;
	mValid = theOther.mValid;
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		mTypeCount[i] = theOther.mTypeCount[i];
	mNumPieces = theOther.mNumPieces;
	mScore = theOther.mScore;
	mLevel = theOther.mLevel;
}

// 0x4051d0 (scalar deleting dtor 0x4059f0)
WordScore::~WordScore()
{
}

// 0x407b90: field by field
WordScore& WordScore::operator=(const WordScore& theOther)
{
	mApp = theOther.mApp;
	mWord = theOther.mWord;
	mValid = theOther.mValid;
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		mTypeCount[i] = theOther.mTypeCount[i];
	mNumPieces = theOther.mNumPieces;
	mScore = theOther.mScore;
	mLevel = theOther.mLevel;
	return *this;
}

// 0x4069c0: mScore is kept
void WordScore::Clear()
{
	mWord = "";
	mValid = false;
	mNumPieces = 0;
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		mTypeCount[i] = 0;
}

// 0x406a10: the word of the tiles in order ("QU" for a Q tile) and its score. Does nothing but Clear
// without an app or a list.
void WordScore::Build(const PieceList* thePieces)
{
	Clear();
	if ((mApp == NULL) || (thePieces == NULL))
		return;

	mNumPieces = (int)thePieces->size();
	for (PieceList::const_iterator anItr = thePieces->begin(); anItr != thePieces->end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece == NULL)
			continue;

		int aType = aPiece->mType;
		if (aType < 5)
		{
			if (aType < 0)
				aType = 0;
		}
		else
			aType = 5;
		mTypeCount[aType]++;

		mWord.append(1, aPiece->mLetter);
		if ((aPiece->mLetter == 'Q') || (aPiece->mLetter == 'q'))
			mWord.append(1, 'U');
	}

	if (mApp->mScoringMethod == 0)
		mScore = CalcScoreByLength();
	else if (mApp->mScoringMethod == 1)
		mScore = CalcScore();
	else
		mScore = 0;
	mScore = ModWordScore(mWord, mScore);  // port(mod): a mod's points multiplier (rude-words)
}

// 0x404f60: the live formula. (letter values + level + 1) * 10 * (length + 2 per green, 4 per gold,
// 7 per sapphire, 10 per diamond tile, + tiles - 1 when every tile is a fire tile). Words under 3 letters
// score 0; the 'U' of "QU" counts as a letter.
int WordScore::CalcScore()
{
	int aLength = (int)mWord.size();
	if (aLength < 3)
		return 0;

	int aSum = 0;
	for (int i = 0; i < aLength; i++)
	{
		int anIndex = (char)mWord[i] - 'A';
		// port: the original indexes the table with any char (its neighbours in rdata for non-letters);
		// a word only holds 'A'..'Z'
		if ((anIndex >= 0) && (anIndex < 26))
			aSum += gLetterValues[anIndex];
	}

	int aMult = aLength + mTypeCount[TILE_GREEN] * 2 + mTypeCount[TILE_GOLD] * 4 + mTypeCount[TILE_SAPPHIRE] * 7 +
		mTypeCount[TILE_DIAMOND] * 10;
	if (mTypeCount[TILE_FIRE] == mNumPieces)
		aMult = aMult - 1 + mNumPieces;

	int aLevel = (mLevel >= 0) ? mLevel : mApp->mBoard->mLevel;
	return aMult * (aSum + aLevel + 1) * 10;
}

// 0x4040e0: the length formula, selected by mScoringMethod 0 (never: the app sets 1)
int WordScore::CalcScoreByLength()
{
	unsigned int aLength = (unsigned int)mWord.size();
	if (aLength < 3)
		return 0;

	unsigned int anIndex = (aLength > 7) ? 8 : aLength;
	int aBase = ((mLevel >= 0) ? mLevel : mApp->mBoard->mLevel) + gLengthBonus[anIndex];
	if (aLength > 8)
		aBase += (int)(aLength - 8) * 500;

	return (mTypeCount[TILE_GREEN] * 2 + 1 + mTypeCount[TILE_GOLD] * 4 + mTypeCount[TILE_SAPPHIRE] * 7 +
		mTypeCount[TILE_DIAMOND] * 10) * aBase * 10;
}

// 0x406130: in the dictionary (case-sensitive: the app's set holds upper-case words)
bool WordScore::CheckValid(const std::set<SexyString>* theDictionary)
{
	if ((mWord.size() > 2) && (theDictionary->size() != 0) &&
		(theDictionary->find(mWord) != theDictionary->end()))
	{
		mValid = true;
		return mValid;
	}

	mValid = false;
	return mValid;
}

// 0x406b00: the longest common prefix of mWord and theOther (bytes; OverlayWidget::Draw)
SexyString WordScore::CommonPrefix(const SexyString& theOther)
{
	SexyString aPrefix = "";
	for (int i = 0; i < (int)theOther.size(); i++)
	{
		if (i >= (int)mWord.size())
			break;
		if (theOther[i] != mWord[i])
			break;
		aPrefix.append(1, theOther[i]);
	}
	return aPrefix;
}

// 0x404fe0: word, valid, 6 type counts, tiles, score
void WordScore::Save(FILE* theFile)
{
	uint32_t aLength = (uint32_t)mWord.size();
	fwrite(&aLength, 1, 4, theFile);
	fwrite(mWord.data(), 1, aLength, theFile);
	WriteBool(theFile, mValid);
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		WriteInt(theFile, mTypeCount[i]);
	WriteInt(theFile, mNumPieces);
	WriteInt(theFile, mScore);
}

// 0x406180
void WordScore::Load(FILE* theFile)
{
	// port: 0 when the file ends here (the original then used the FILE pointer as the length)
	uint32_t aLength = 0;
	fread(&aLength, 1, 4, theFile);
	mWord.resize(aLength, '\0');
	if (aLength > 0)
		fread(&mWord[0], 1, aLength, theFile);
	ReadBool(theFile, &mValid);
	for (int i = 0; i < NUM_TILE_TYPES; i++)
		ReadInt(theFile, &mTypeCount[i]);
	ReadInt(theFile, &mNumPieces);
	ReadInt(theFile, &mScore);
}
