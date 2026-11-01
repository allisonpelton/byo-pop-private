#ifndef __BONNIES_HIGHSCORES_H__
#define __BONNIES_HIGHSCORES_H__

// The high scores (0x40cd90-0x40d160, 0xf4 bytes, the object 0x476ad0): two tables of five, the timed game's and the
// other's, in Data/HighScores.dat.

#include "Str.h"

namespace bonnies
{

// An entry (0x18 bytes, ctor 0x40ccd0, dtor 0x40cd20)
struct HighScore
{
	int mUsed;                // +0x00
	Str mName;                // +0x04
	Str mWord;                // +0x0c: the longest word
	int mScore;               // +0x14
};

class HighScores
{
public:
	// 0x40ce80
	HighScores();

	// 0x40cd90
	void save();
	// 0x40ce10: the tables of the texts and scores in the exe
	void reset();
	// 0x40ceb0
	void load();
	// 0x40cf50: theScore into table theTable; its rank, -1 when it is not in the five
	int add(int theTable, Str theName, Str theWord, int theScore);

	int mLoaded;              // +0x00
	HighScore mScores[10];    // +0x04
};

extern HighScores gHighScores;  // 0x476ad0
extern int gHighScoreRank;      // 0x473e38: the last game's rank
extern int gHighScoreTable;     // 0x473e3c: and its table

} // namespace bonnies

#endif
