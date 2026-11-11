#ifndef __ATOMICA_HIGHSCORES_H__
#define __ATOMICA_HIGHSCORES_H__

// HighScores.cpp: the high score tables (Atomica.exe 0x41e240-0x41e5c0): the best five scores of a kind of game with the
// players' names, and the best score of the day. The original keeps two of them in the app (+0x3eb0 for the strategy
// mode, +0x3f8c for the action mode) and writes them to HS.CFG.

#include <cstdio>

namespace Sexy
{

class Buffer;

class ScoreTable
{
public:
	enum { NUM_SCORES = 5, NAME_SIZE = 30 };

	int		mScores[NUM_SCORES];			// +8
	char	mNames[NUM_SCORES][NAME_SIZE];	// +0x1c
	int		mBestToday;						// +0xb4 the best score of the day, -1 for none
	char	mBestName[NAME_SIZE];			// +0xb8
	int		mDate;							// +0xd8 the day of the best score (a code of year, month and day)

	ScoreTable() { Reset(); }
	void		Reset();					// 0x41e320: the tables a new installation starts with (port: the names are the port's own)
	void		Read(Buffer* theBuffer);	// 0x41e260
	void		Write(Buffer* theBuffer) const;	// 0x41e2c0
	static int	DateCode();					// (month + year * 12) * 31 + day
	bool		Qualifies(int theScore) const;	// 0x41e440: a score that is in the table or the best of the day
	void		Add(int theScore, const char* theName);	// 0x41e530
};

}

#endif
