#ifndef __WORDHARMONY_GAME_SAVEDGAME_H__
#define __WORDHARMONY_GAME_SAVEDGAME_H__

// A game left to be continued (0x8c bytes, constructor 0x41efe0, destructor 0x41f4e0): the settings keep one per mode
// (Settings::mSavedGames) and store it in the registry as XML text (PuzzleSavedGame, ActionSavedGame,
// SolitaireSavedGame). gameOver fills it with the ball system's saveState and the controller's counts.

#include <string>
#include <vector>

class Ball;
class Chain;
class Settings;
struct Bubble;
struct FoundWord;

class SavedGame
{
public:
	// 0x41efe0: the mode only (the rest is filled before the game is saved)
	explicit SavedGame(int theMode);
	~SavedGame();  // 0x41f4e0

	// 0x41e760
	void setHeader(int theLevel, int theScore, float theRespawnTimer);
	// 0x41f0d0: a ball in play (its position, letter and flags)
	void addBall(Ball* theBall);
	// 0x41f1c0: a ball read back ('?' a wildcard)
	void addBubble(char theLetter, const std::string& theSpecial, float x, float y);
	// 0x41f2b0: a ball not in play
	void addEmpty();
	// 0x41f310: a chain read back, its indices still to come
	Chain* addChain(const std::string& theWord);
	// 0x41f7e0: a copy of a chain in play
	void addChain(Chain* theChain);
	// 0x41f3d0: a found word read back
	void addWord(const std::string& theDisplayText, const std::string& theWord, bool theMatched, int theScore,
		int theMultiplier, const std::vector<std::string>& theChains);
	// 0x41f6c0: a copy of a found word of the HUD
	void addWord(FoundWord* theWord);
	// 0x41f7d0: a letter or piece of the queue
	void addLetters(const std::string& theLetters);

	// 0x41e820: the registry's text. A chain's balls are written inside it and taken out of mBubbles (the original
	// leaves the game as it is only because the settings read it back at once).
	std::string toXml();

	int mLevel;                         // +0x00
	int mMode;                          // +0x04
	int mScore;                         // +0x08
	float mRespawnTimer;                // +0x0c: the ball system's +0x100
	int mWildcards;                     // +0x10: "wc"
	int mHintsUsed;                     // +0x14: "hug"
	int mLevelHintsUsed;                // +0x18: "hul"
	int mHints;                         // +0x1c: "ha"
	int mLevelPoints;                   // +0x20: "lp"
	int mLettersUsed;                   // +0x24: "lba"
	int mTemplate;                      // +0x28: "ilg"
	int mTime;                          // +0x2c
	std::string mBonusWord;             // +0x30: "bonus", the ball system's +0x148
	std::vector<Bubble*> mBubbles;      // +0x4c: one per ball of the array (null: not in play)
	std::vector<Chain*> mChains;        // +0x5c
	std::vector<FoundWord*> mWords;     // +0x6c: the HUD's list
	std::vector<std::string> mQueue;    // +0x7c: the letters and pieces still to come
};

// 0x40a250's part: a saved game's text parsed (with the reader 0x40c8f0: 0x40cf90 its elements, 0x40cdc0 the end of a
// rope) into the settings' saved game of its mode
void readSavedGame(Settings* theSettings, const std::string& theText);

#endif
