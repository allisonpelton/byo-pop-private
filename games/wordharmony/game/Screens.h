#ifndef __WORDHARMONY_SCREENS_H__
#define __WORDHARMONY_SCREENS_H__

// The game's screens (0x409c20-0x40db70): layouts of the archive with a little code each.

#include "ScreenLayout.h"
#include <string>

namespace engine
{
class TextItem;
}

// The level's end (constructor 0x409c20, vtable 0x480e88)
class CongratsScreen : public engine::ScreenLayout
{
public:
	CongratsScreen();  // 0x409c20

	// 0xf0: 0x409710, the score and the time played
	virtual void showResults();
};

// The game's end (constructor 0x40b0d0, vtable 0x481110)
class GameOverScreen : public engine::ScreenLayout
{
public:
	GameOverScreen();  // 0x40b0d0

	// 0xf0: 0x40aec0, the score, the hints used and the five longest words; the extra scores outside the puzzles
	virtual void showResults();
};

class HighScores;

// The high score tables (0xe8 bytes, constructor 0x40c190, vtable 0x481420): puzzle, solitaire and freematch
// ("tsunami"), ten lines each
class HighScoreScreen : public engine::ScreenLayout
{
public:
	HighScoreScreen();  // 0x40c190

	// 0xf0: 0x40b660, the tables shown from their first line
	virtual void setTables(HighScores* thePuzzle, HighScores* theSolitaire, HighScores* theTsunami);
	// 0xf4: 0x40b6c0, the lines from mFirst on (puzzle, tsunami, then solitaire); the nodes <table>-scoreRank<n>,
	// -scoreName<n> and -score<n> count from 0
	virtual void showScores();
	// 0xf8: 0x40b6a0
	virtual void reset();

	HighScores* mPuzzle;     // +0xd4
	HighScores* mSolitaire;  // +0xd8
	HighScores* mTsunami;    // +0xdc
	int mFirst;              // +0xe0
};

// The level choice (0xe0 bytes, constructor 0x40c480, vtable 0x481578): a button per theStep levels, the buttons
// level<n>, level<n>Img and level<n>Txt numbered by their first level
class LevelsSelectionScreen : public engine::ScreenLayout
{
public:
	LevelsSelectionScreen(int theCount, int theStep);  // 0x40c480

	// 0xf0: 0x40c2b0, the buttons past theHighest + 1 disabled and dimmed
	virtual void showLevels(int theHighest);

	int mHighest;  // +0xd4: theHighest + 1
	int mCount;    // +0xd8: the buttons
	int mStep;     // +0xdc: the levels per button
};

// The main menu (0xd4 bytes, constructor 0x40c770, vtable 0x4816a0): a mode button tells the controller when the
// mouse is over it, for the mode's description
class MainMenuScreen : public engine::ScreenLayout
{
public:
	MainMenuScreen();  // 0x40c770

	void buttonEnter(std::string theName) override;  // 0x40c5a0
	void buttonPress(std::string theName) override;  // 0x40c700
	void buttonExit(std::string theName) override;   // 0x40c650
};

// The screen saver (0xe4 bytes, constructor 0x40db70, vtable 0x4818e0; its key handler at +0xd4, vtable 0x4818d4),
// shown when the game runs as a screen saver: a key or a move of the mouse ends the program
class ScreenSaver : public engine::ScreenLayout, public engine::KeyHandler
{
public:
	ScreenSaver();  // 0x40db70

	void mouseMove(engine::Point& p) override;  // 0x40db50
	bool keyDown(int theKey) override;          // 0x40db60
	bool keyUp(int theKey) override;            // 0x40db60
};

// The name entry of a new high score (0x108 bytes, constructor 0x40b510, vtable 0x481238; its key handler at +0xd4,
// vtable 0x481230). Enter tells the controller "highScoreEntryDone".
class HighScoreEntryLayout : public engine::ScreenLayout, public engine::KeyHandler
{
public:
	HighScoreEntryLayout();             // 0x40b510
	~HighScoreEntryLayout() override;   // 0x40b410

	// 0x40b1f0: the name to edit and the score shown, the keys taken
	void startEntry(std::string theName, int theScore);

	bool keyDown(int theKey) override;  // 0x40b2e0: a printable key typed (16 letters at most), backspace, enter
	bool keyUp(int theKey) override;    // 0x40b1d0: whether the key is printable

	std::string mName;             // +0xdc
	int mScore;                    // +0xf8
	engine::TextItem* mNameText;   // +0x100
};

#endif
