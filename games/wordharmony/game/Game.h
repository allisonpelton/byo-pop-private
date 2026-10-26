#ifndef __WORDHARMONY_GAME_H__
#define __WORDHARMONY_GAME_H__

// The game's controller (vtable 0x480a14; the global at 0x4995b8): the screens, the levels, the scores and the game's
// state machine (games/wordharmony/docs/WORKING_NOTES.md, "Next pieces"). Only what the ball system uses is declared
// so far; the rest comes with the controller's port (0x401000-0x409c00).

#include "App.h"
#include "Dictionary.h"
#include "ScreenLayout.h"
#include <string>
#include <vector>

namespace engine
{
class MusicSystem;
class TextEffects;
}

class Level;
class WordBallSystem;

class Game : public engine::Controller, public engine::ScreenListener
{
public:
	static Game* instance() { return sInstance; }  // 0x4995b8

	// 0x401260: points to the score, and the score to the HUD
	void addScore(int thePoints);
	// 0x401290: a found word's floating text is done (m3C less thePoints, at least 0)
	static void scoreShown(int thePoints);
	// 0x401580
	bool isPaused();
	// 0x402b10: plays a sound of the sound bank by its name
	void playSound(std::string theName, bool theFlag);
	// 0x402980: the letters in a random order
	static std::string shuffle(std::string theLetters);
	// 0x404220
	void resume();
	// 0x404e10 (not read yet): the game is over (the last puzzle level done, or freematch's water at the top)
	void gameOver(bool theA, bool theB);
	// 0x404f60 (not read yet): random letters, theMin to theMax of them
	std::string makeLetters(int theMin, int theMax);
	// 0x4050f0: theCount random letters
	std::string randomLetters(int theCount);
	// 0x4055f0: the next state of the state machine (+0x1d8)
	void setState(int theState);
	// 0x406520
	void pause(bool theShowMenu);
	// 0x4070d0 (not read yet): the H key
	void helpKey(int theA, int theMode);
	// 0x4088f0 (not read yet): a found word into the list of the longest (+0x5c)
	void noteWord(std::string theWord);
	// 0x408640: solitaire's words for a level of theCount balls
	void makeWords(int theCount, std::vector<std::string>& theWords);

	// The fields the ball system uses
	Dictionary mDictionary;             // +0x10
	int mScore;                         // +0x2c
	int m38;                            // +0x38: the points too (0x401260)
	int m3C;                            // +0x3c: lowered by the found words' floating texts (0x401290)
	int mLettersUsed;                   // +0x40: the found words' lengths, and solitaire's wildcards
	unsigned int mWildcards;            // +0x48: wildcards to add to the next level's letters
	int mHints;                         // +0x4c: hints left
	int mHintsUsed;                     // +0x50
	int mHintsUsedTotal;                // +0x54
	engine::TextEffects* mTextEffects;  // +0x70
	engine::ScreenLayout* mOptions;     // +0x104
	bool mGridSnap;                     // +0x108: the editor's grid
	int m18C;                           // +0x18c
	engine::MusicSystem* mMusic;        // +0x1a4
	std::vector<Level*> mLevels;        // +0x1a8
	int mState;                         // +0x1d8: the state machine (5: playing)
	int mLevel;                         // +0x308: the puzzle level played
	Level* mCurrentLevel;               // +0x30c
	WordBallSystem* mBallSystem;        // +0x700
	int mMode;                          // +0x708: the game's mode (the ball system's MODE_ values)

	static Game* sInstance;
};

#endif
