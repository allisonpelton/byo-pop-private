#ifndef __WORDHARMONY_WORDBALLSYSTEM_H__
#define __WORDHARMONY_WORDBALLSYSTEM_H__

// The playfield: the letter balls and their physics, the obstacles, the chains of a found word, the rising water of
// freematch, the mouse's rope and the HUD (0x220 bytes, constructor 0x416fd0, named "WordBallSystem"; the
// controller's +0x700). The original's parts: the ball array at +0, a Container at +0x20 (its base, vtable 0x4821c8,
// adds only the pure slot 0xd8), a KeyHandler at +0xc8 and a ScreenListener at +0xd0 for the level editor's HUD,
// which this build never loads, so the port leaves that listener out (games/wordharmony/docs/WORKING_NOTES.md, "The
// game code as read"). Offsets below are the object's; the Container part's methods see them 0x20 lower.

#include "Node.h"
#include "Physics.h"
#include <map>
#include <string>
#include <vector>

namespace engine
{
class FadeContainer;
class Image;
class ImageNode;
class ParticlePool;
}

class Hud;
class Level;
class Obstacle;

// The game's modes (+0xd4)
enum
{
	MODE_PUZZLE = 0,     // the levels' word lists (HUD.xml)
	MODE_SOLITAIRE = 1,  // HUD-solitaire.xml
	MODE_FREEMATCH = 2   // the rising water (HUD-freematch.xml)
};

// A row or a column of balls (0x54 bytes, 0x415c20)
struct BallGroup
{
	int mIndices[20];     // +0x00
	unsigned int mCount;  // +0x50
};

class SavedGame;

class WordBallSystem : public BallArray, public engine::Container, public engine::KeyHandler
{
public:
	WordBallSystem(int theMode);  // 0x416fd0
	~WordBallSystem() override;   // 0x415920

	// The ball array's slot 1: 0x414b90
	void step() override;

	// The Container part
	void draw(engine::Graphics* g) override;          // 0x6c: 0x412310
	void update(engine::UpdateInfo& theInfo) override;  // 0x70: 0x41b760
	void mouseMove(engine::Point& p) override;        // 0x9c: 0x411260
	// 0xd8: 0x417b20, the level's balls, letters and obstacles
	virtual void startLevel();

	// The key handler
	bool keyDown(int theKey) override;  // 0x416400
	bool keyUp(int theKey) override { return false; }  // 0x411ab0

	// 0x4135c0: the 105 balls, all gone
	void makeBalls();
	// 0x4113f0: a ball's images from its flags
	void setupBall(Ball* theBall);
	// 0x411a60: whether a ball is picked or a blossom still plays
	bool isBusy();
	// 0x411ac0: the mouse's rope on a ball
	void grab(Ball* theBall);
	// 0x411ba0
	void letGo();
	// 0x411c00
	void setMode(int theMode);
	// 0x411c50 (the port runs the hint finder without a thread)
	void stopHintThread();
	// 0x411c90: freematch's water level (0 to 1) as the music's clip, 0-7
	static int waterStage(float theValue);
	// 0x411dc0: the level is over
	void levelDone();
	// 0x4142c0: the balls, the obstacles and the word under way into a saved game
	void saveState(SavedGame* theGame);
	// 0x4199d0: the same taken from a saved game, after startLevel
	void loadState(SavedGame* theGame);
	// 0x412080: the nearest obstacle within 23 pixels, or null
	Obstacle* findObstacle(const engine::Point& thePoint);
	// 0x412920: the chain's balls freed from it, and the chain deleted
	void removeChain(Chain* theChain);
	// 0x412b30: a ball picked into the word (theB: its +0xb0)
	void pickBall(unsigned int theIndex, float theB);
	// 0x412bf0: a ball gone
	void removeBall(unsigned int theIndex);
	// 0x412ca0: every free ball picked
	void pickAll();
	// 0x4137b0
	void addHud();
	// 0x413860: a ball at (x, y), or -1 when all 105 are in play
	int spawn(float x, float y, char theLetter, unsigned int theFlags, bool theQuiet);
	// 0x4140d0: a ball at the top
	int spawnLetter(char theLetter, bool theQuiet);
	// 0x413da0: word theIndex's definition in the HUD
	void showDefinition(int theIndex);
	// 0x413f00
	void makeHuds();
	// 0x414d60: the next hint as a floating text
	void showHint();
	// 0x414fb0 (the queue's pop_back): the last letter or piece of the queue taken off
	void popQueue();
	// 0x415300
	void reset();
	// 0x4153a0: every ball gone, the playfield emptied
	void clear();
	// 0x415c20: the balls into rows and columns
	void groupBalls();
	// 0x416300: a chain starting with ball theIndex
	Chain* addChain(int theIndex);
	// 0x416c60: a word's balls at the top, chained
	void spawnWord(const std::string& theWord);
	// 0x4174f0: the same at (x, y)
	void spawnWordAt(const std::string& theWord, float x, float y);
	// 0x418f30: theLetters into the queue, the pieces first (their letters taken out of theLetters)
	void queueLetters(std::string theLetters, const std::vector<std::string>& thePieces);
	// 0x4191e0: the words the balls in play can make (the original's hint thread)
	void findHints();
	// 0x419540
	void startHints();
	// 0x4195d0: the queue's next letter or piece, refilled when empty; taken off unless theKeep
	void nextPiece(std::string& theResult, bool theKeep);
	// 0x419f40: freematch's next ball or word
	void spawnNext();
	// 0x41a050
	void addWildcard();
	// 0x41a120: theCount balls from theIndices[theStart] make theWord: picked and scored
	void scoreWord(const int* theIndices, int theStart, const std::string& theLetters, const std::string& theWord,
		int theCount);
	// 0x41a8b0: a row's or a column's words of theMinLength letters or more, scored; how many
	int scoreGroup(BallGroup* theGroup, unsigned int theMinLength);
	// 0x41b2d0: a row's or a column's words of the puzzle, scored; how many
	int scorePuzzleGroup(BallGroup* theGroup);
	// 0x41b5c0
	int scorePuzzle();
	// 0x41b650
	int scoreGroups(unsigned int theMinLength);
	// 0x41b6f0: the submit button
	void submit();

	int mMode;                                  // +0xd4
	float mLastStep;                            // +0xd8: the last physics step's time (the original leaves it
	                                            // unset until the first; the port starts it at 0)
	Bounds* mBounds;                            // +0xdc
	Rope* mRope;                                // +0xe0: the mouse's rope on the grabbed ball
	Follow* mFollow;                            // +0xe4: the editor's drag
	std::vector<Chain*> mChains;                // +0xe8
	std::vector<Obstacle*>* mObstacles;         // +0xf8: the level's
	float mStepTime;                            // +0xfc: time not yet stepped
	float mSpawnTime;                           // +0x100
	bool mStarted;                              // +0x104: set when a level starts
	int mLongestPiece;                          // +0x108
	int mNextSlot;                              // +0x10c
	std::vector<std::string> mQueue;            // +0x110: letters and pieces still to come
	int mBallsToCome;                           // +0x120
	float mWaterLevel;                          // +0x124
	float mFill;                                // +0x128: the balls in play over the level's maxBalls, at most 1
	int mShortestWord;                          // +0x12c
	std::map<std::string, int> mWordIndex;      // +0x130: the level's words
	double mLastMatch;                          // +0x140: the time of the last word (the original leaves it unset
	                                            // until then; the port starts it at 0)
	std::string mWord;                          // +0x148
	std::vector<BallGroup*> mRows;              // +0x164
	std::vector<BallGroup*> mColumns;           // +0x174
	std::vector<BallGroup*> mFreeGroups;        // +0x184
	bool m194;                                  // +0x194: set while words are scored (a submit, the puzzles);
	                                            // otherwise a word found only lights its balls
	Hud* mHud;                                  // +0x198: the mode's
	Hud* mSolitaireHud;                         // +0x19c
	Hud* mPuzzleHud;                            // +0x1a0
	Hud* mFreematchHud;                         // +0x1a4
	bool mNoPicking;                            // +0x1a8
	engine::ImageNode* mTarget;                 // +0x1ac: target_hit.png at the grabbed ball
	engine::Image* mChainLink;                  // +0x1b0
	engine::FadeContainer* mLayer;              // +0x1b4: the balls and the obstacles
	engine::Point mMouse;                       // +0x1b8
	engine::ParticlePool* mBlossoms;            // +0x1c0
	engine::ParticlePool* mSparkles;            // +0x1c4
	engine::Image* mWater[4];                   // +0x1c8
	float mWaveTime;                            // +0x1d8
	std::vector<std::string> mHints;            // +0x1e0
	bool m1F0;                                  // +0x1f0: set by a submit, so the hints are looked for again
	                                            // once the balls are done
	unsigned int mHintIndex;                    // +0x1f4
	bool mEditor;                               // +0x1f8: never set in this build
	int m1FC;                                   // +0x1fc
	Level* mLevelBackup;                        // +0x200: the editor's
	std::vector<std::string> m204;              // +0x204

protected:
	void onMouseDown(engine::Point& p) override;  // 0x80: 0x412120
	void onMouseUp(engine::Point& p) override;    // 0x88: 0x411d80
	void onRightUp(engine::Point& p) override;    // 0x98: 0x41c2d0
};

#endif
