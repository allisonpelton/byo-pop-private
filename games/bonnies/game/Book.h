#ifndef __BONNIES_BOOK_H__
#define __BONNIES_BOOK_H__

// The levels, one chapter of a book each (0x40d3c0-0x40dd00, 0x444 bytes each; the array 0x466728 in the exe's
// data holds 50): the board's shape, its first tiles and what may fall where. loadBooks copies them from the exe,
// with the board stored by rows as the exe has it; init (from loadResources) turns it to columns.

#include "Board.h"
#include "Str.h"

namespace bonnies
{

// A cell of a level's board (12 bytes, at +0x78)
struct BookCell
{
	uint32_t mFlags;          // +0x00: 1 the board has the cell, 2 fixed, 4
	int mKind;                // +0x04: the tile's kind at the start
	char mTile;               // +0x08: its tile at the start
};

struct Book
{
	// 0x406450: the chapter's text
	Str text();
	// 0x40d3c0: the chapter's name, the book's and "_0" and the chapter's number
	Str name();
	// 0x40d4a0: the name of the book's summary
	Str summaryName();
	// 0x40d550: the book's title
	Str title();
	// 0x40d580: whether the chapter has a text of more than one letter
	int hasText();
	// 0x40d5b0: book theBook's chapter theChapter, theLast when it is the book's last
	void init(int theBook, int theChapter, int theLast);
	// 0x40d160: the letters that never fall
	Str banned();
	// 0x40d180: the letters that never fall into theColumn
	Str bannedIn(int theColumn);
	// 0x40d1b0: whether the board has a cell there
	int isCell(int theColumn, int theRow);
	// 0x40d1d0: whether theChar may fall into theColumn
	int allows(char theChar, int theColumn);

	int mBook;                // +0x00
	int mChapter;             // +0x04
	int mLast;                // +0x08
	int mText;                // +0x0c: the chapter's text id
	int mGoalBase;            // +0x10
	int mBonusBase;           // +0x14
	int mRows;                // +0x18: the last row with a cell, plus 1
	int mColumns;             // +0x1c: the last column with a cell, plus 1
	int mFillCount;           // +0x20: the cells, filled at the start
	int mMaxStars;            // +0x24
	int mNoVowels;            // +0x28
	int mNoConsonants;        // +0x2c
	int mNoWilds;             // +0x30: '?' and '!' never fall
	int mNoStars;             // +0x34: '*' never falls
	int mTwoFills;            // +0x38: a tile takes two words to mark (the flags 0x10, then 0x20)
	int mFastOdds;            // +0x3c: a long word adds more to the special tiles' odds
	int mStones;              // +0x40: '#' tiles
	int mPairs;               // +0x44: two-letter tiles may fall
	float mOffsetX;           // +0x48: the board's position
	float mOffsetY;           // +0x4c
	const char* mBanned;      // +0x50
	const char* mBannedIn[9]; // +0x54
	BookCell mCells[BOARD_CELLS];      // +0x78
};

extern Book gBooks[LEVEL_COUNT];  // 0x466728

// Copies the books from the exe (after runStaticInitializers)
void loadBooks();

} // namespace bonnies

#endif
