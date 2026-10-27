#ifndef __HIGHSCORES_H__
#define __HIGHSCORES_H__

// A mode's five high scores (vtable 0x4fb7f0, only its destructor; 0xac bytes; 0x422410-0x422d90), the board's
// +0x1d8. Each mode keeps its own file beside the game (STOMPED.HS, ENDLESS.HS, FOSSIL.HS, TIMETRIAL.HS); without one
// the five are the game's characters with set scores. A game is known by its id (the board's start ticks), so a
// game that scores again replaces its own row. The time trial keeps its times negative, in hundredths.

namespace Sexy
{

class Graphics;

// one row, 0x1c bytes, as the file keeps it
struct HighScoreEntry
{
	int					mId;				// +0 the game's id (-1 for the starting rows)
	char				mName[0x14];		// +4
	int					mScore;				// +0x18
};

class HighScores
{
public:
	int					mMode;				// +4 0 stomped, 1 endless, 2 fossil, 3 time trial
	bool				mDirty;				// +8 the destructor saves
	char				mFileName[0x17];	// +9
	HighScoreEntry		mEntries[5];		// +0x20

	HighScores(int theMode);				// 0x422410
	virtual ~HighScores();					// 0x422820

	// 0x4226d0: theScore beats its game's own row, or (without one) any row
	bool				IsHighScore(int theScore, int theId);
	void				Save();									// 0x422730
	void				Remove(int theId);						// 0x4227b0: the game's row, the rest move up
	void				Add(int theScore, const char* theName, int theId);	// 0x422840
	// 0x422900: a dark box, then the names and scores in the graphics' font with a shadow
	void				Draw(Graphics* g, int x, int y, int theWidth, int theHeight);
};

}

#endif
