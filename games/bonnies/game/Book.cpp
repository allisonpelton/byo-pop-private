// The levels (see Book.h).

#include "Book.h"
#include "Exe.h"
#include "Game.h"
#include "Globals.h"
#include "Language.h"
#include "Options.h"

#include <cstring>

namespace bonnies
{

Book gBooks[50];

void loadBooks()
{
	for (int i = 0; i < 50; i++)
	{
		uint32_t anAddress = 0x466728 + i * 0x444;
		Book& aBook = gBooks[i];
		aBook.mBook = exeI32(anAddress);
		aBook.mChapter = exeI32(anAddress + 4);
		aBook.mLast = exeI32(anAddress + 8);
		aBook.mText = exeI32(anAddress + 0xc);
		aBook.mGoalBase = exeI32(anAddress + 0x10);
		aBook.mBonusBase = exeI32(anAddress + 0x14);
		aBook.mRows = exeI32(anAddress + 0x18);
		aBook.mColumns = exeI32(anAddress + 0x1c);
		aBook.mFillCount = exeI32(anAddress + 0x20);
		aBook.mMaxStars = exeI32(anAddress + 0x24);
		aBook.mNoVowels = exeI32(anAddress + 0x28);
		aBook.mNoConsonants = exeI32(anAddress + 0x2c);
		aBook.mNoWilds = exeI32(anAddress + 0x30);
		aBook.mNoStars = exeI32(anAddress + 0x34);
		aBook.mField38 = exeI32(anAddress + 0x38);
		aBook.mField3C = exeI32(anAddress + 0x3c);
		aBook.mStones = exeI32(anAddress + 0x40);
		aBook.mPairs = exeI32(anAddress + 0x44);
		aBook.mOffsetX = exeFloat(anAddress + 0x48);
		aBook.mOffsetY = exeFloat(anAddress + 0x4c);
		aBook.mBanned = exeString(exeU32(anAddress + 0x50));
		for (int j = 0; j < 9; j++)
			aBook.mBannedIn[j] = exeString(exeU32(anAddress + 0x54 + j * 4));
		for (int j = 0; j < 81; j++)
		{
			uint32_t aCell = anAddress + 0x78 + j * 0xc;
			aBook.mCells[j].mFlags = exeU32(aCell);
			aBook.mCells[j].mKind = exeI32(aCell + 4);
			aBook.mCells[j].mTile = (char)*exePtr(aCell + 8);
		}
	}
}

// 0x406450
Str Book::text()
{
	return Str(getText(mText));
}

// 0x40d160
Str Book::banned()
{
	return Str(mBanned);
}

// 0x40d180
Str Book::bannedIn(int theColumn)
{
	return Str(mBannedIn[theColumn]);
}

// 0x40d1b0
int Book::isCell(int theColumn, int theRow)
{
	return mCells[theColumn * 9 + theRow].mFlags & 1;
}

// 0x40d1d0
int Book::allows(char theChar, int theColumn)
{
	Str aBanned = banned();
	int i = 0;
	if (0 < (int)aBanned.size())
	{
		do
		{
			if ((unsigned char)at(aBanned, i) == (int)theChar)
				return 0;
			i++;
		} while (i < (int)aBanned.size());
	}
	Str aBannedIn = bannedIn(theColumn);
	i = 0;
	if (0 < (int)aBannedIn.size())
	{
		do
		{
			if ((unsigned char)at(aBannedIn, i) == (int)theChar)
				return 0;
			i++;
		} while (i < (int)aBannedIn.size());
	}
	if (theChar == '*')
	{
		// No '*' falls into a column with a '!' or a cell of flag 4
		if (gGame != nullptr)
		{
			int aRow = 0;
			do
			{
				if ((mCells[theColumn * 9 + aRow].mFlags & 1) != 0 &&
					(gGame->mLetters[theColumn * 9 + aRow] == '!' ||
						(gGame->mTileFlags[theColumn * 9 + aRow] & 4) != 0))
					return 0;
				aRow++;
			} while (aRow < 9);
		}
		if (mNoStars != 0)
			return 0;
	}
	else
	{
		if ('`' < theChar && theChar < '{' && mPairs == 0)
			return 0;
		if (theChar == '?' || theChar == '!')
		{
			if (mNoWilds != 0)
				return 0;
		}
	}
	if ((isVowel(theChar) == 0 || mNoVowels == 0) && (isConsonant(theChar) == 0 || mNoConsonants == 0))
		return 1;
	return 0;
}

// 0x40d3c0
Str Book::name()
{
	Str aName = "";
	aName += Str(exeGlobalString(0x476fd0 + mBook * 8));
	aName += lit(0x457dec);
	aName += intToStr(mChapter + 1);
	return aName;
}

// 0x40d4a0
Str Book::summaryName()
{
	Str aName = "";
	aName += lit(0x457df0);
	aName += Str(exeGlobalString(0x476fd0 + mBook * 8));
	return aName;
}

// 0x40d550
Str Book::title()
{
	return Str(getText(exeI32(0x466630 + mBook * 4)));
}

// 0x40d580
int Book::hasText()
{
	if (mText != 0 && 1 < std::strlen(getText(mText)))
		return 1;
	return 0;
}

// The tutorial's words: the first letters of a text, upper-cased, into the cells listed
static void placeWord(Book* theBook, int theText, const int* theCells, int theCount)
{
	Str aWord = toUpperStr(Str(getText(theText)));
	for (int i = 0; i < theCount; i++)
		theBook->mCells[theCells[i]].mTile = (char)at(aWord, i);
}

// 0x40d5b0
void Book::init(int theBook, int theChapter, int theLast)
{
	mBook = theBook;
	mChapter = theChapter;
	mLast = theLast;
	mColumns = 0;
	mRows = 0;
	mFillCount = 0;
	// The exe has the board by rows
	int i = 0;
	do
	{
		int j = i;
		if (0 < i)
		{
			int k = 0;
			do
			{
				BookCell& aCell1 = mCells[k * 9 + i];
				BookCell& aCell2 = mCells[i * 9 + k];
				char aTile = aCell1.mTile;
				aCell1.mTile = aCell2.mTile;
				aCell2.mTile = aTile;
				uint32_t aFlags = aCell1.mFlags;
				aCell1.mFlags = aCell2.mFlags;
				aCell2.mFlags = aFlags;
				int aKind = aCell1.mKind;
				aCell1.mKind = aCell2.mKind;
				aCell2.mKind = aKind;
				k++;
				j--;
			} while (j != 0);
		}
		i++;
	} while (i < 9);
	int aFirstColumn = 99;
	int aStars = 0;
	int aRow = 0;
	do
	{
		int aColumn = 0;
		do
		{
			BookCell& aCell = mCells[aColumn * 9 + aRow];
			if ((aCell.mFlags & 1) != 0)
			{
				if (aColumn <= aFirstColumn)
					aFirstColumn = aColumn;
				if (mColumns <= aColumn + 1)
					mColumns = aColumn + 1;
				if (mRows <= aRow + 1)
					mRows = aRow + 1;
				mFillCount++;
				if (aCell.mTile == '*')
					aStars++;
			}
			aColumn++;
		} while (aColumn < 9);
		aRow++;
	} while (aRow < 9);
	mMaxStars = 6;
	double aMaxStars = mFillCount * (double)0.3f;
	if (!(6.0f <= aMaxStars))
		aMaxStars = 6.0f;
	mMaxStars = (int)aMaxStars;
	if (mMaxStars <= aStars + 4)
		mMaxStars = aStars + 4;
	if (theBook == 0)
	{
		if (theChapter == 0)
		{
			static const int aCells1[] = {28, 38, 47, 48};
			static const int aCells2[] = {39, 46, 57, 56};
			static const int aCells3[] = {3, 4, 5, 6};
			static const int aCells4[] = {31, 41, 50, 60};
			static const int aCells5[] = {12, 13, 14, 2};
			placeWord(this, 0x3e9, aCells1, 4);
			placeWord(this, 0x3ea, aCells2, 4);
			placeWord(this, 0x3ec, aCells3, 4);
			placeWord(this, 0x3ed, aCells4, 4);
			placeWord(this, 0x3ee, aCells5, 4);
		}
		else if (theChapter == 1)
		{
			static const int aCells1[] = {41, 42, 51, 61};
			static const int aCells2[] = {1, 2, 10, 19, 20};
			static const int aCells3[] = {6, 7, 16, 25, 33};
			static const int aCells4[] = {55, 56, 57, 65, 74};
			placeWord(this, 0x3eb, aCells1, 4);
			placeWord(this, 0x3ef, aCells2, 5);
			placeWord(this, 0x3f0, aCells3, 5);
			placeWord(this, 0x3f1, aCells4, 5);
		}
	}
	double aX = (((0.715f - aFirstColumn * (double)0.0575f * 0.5f) - mColumns * (double)0.0575f * 0.5f) + 0.44625f) -
		0.44625f + mOffsetX;
	mOffsetX = (float)aX;
	float aY = (float)((0.44166666f - (mRows + (double)0.5f) * 0.076666668f * 0.5f) + mOffsetY);
	mOffsetY = aY;
	if (8 < mColumns)
		mOffsetX = (float)(aX - 0.0099999998f);
	if (8 < mRows)
		mOffsetY = (float)(aY - 0.0225f);
	else if (7 < mRows)
		mOffsetY = (float)(aY - 0.0049999999f);
}

} // namespace bonnies
