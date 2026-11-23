#ifndef __FEEDINGFRENZY2_GAMEDATA_H__
#define __FEEDINGFRENZY2_GAMEDATA_H__

// The game's data (GameData, 0xb8 bytes, vtable 0x55b014 (feedingfrenzy2); its constructor 0x49ffe0
// (feedingfrenzy2)), the application's +0x3c: the game's stages and the menu's, each from its manifest, and the game
// being played: its mode, lives, score, continues and stage.

#include "Object.h"
#include "StageManifest.h"

// A new game's lives and continues (0x49c050 (feedingfrenzy2))
const int NEW_GAME_LIVES = 3;
const int NEW_GAME_CONTINUES = 2;
// The score between two extra lives grows by this after each, up to the most (0x49c090 (feedingfrenzy2))
const int EXTRA_LIFE_SCORE_STEP = 5000;
const int EXTRA_LIFE_SCORE_MOST = 25000;

class GameData : public engine::Object
{
public:
	GameData()
	    : mMode(0), mLives(0), mScore(0), mContinues(0), mNextLifeScore(0), mLifeScoreStep(0), mSpawnPhase(0),
	      mStage(-1), mTimeRollover(0), mFood(0)
	{
	}

	// 0x49c050 (feedingfrenzy2): a new game of the mode, from the first stage
	void newGame(int theMode);
	// 0x49c090 (feedingfrenzy2): the score of the next extra life, the first past the score, and the step after it
	void findNextLifeScore();
	// 0x4106f0 (feedingfrenzy2): the points added to the score (held at 0 when it falls below); past the next extra
	// life's score, outside the timed mode, a life more and the next one's score a step on, the step growing by 5000
	// up to 25000. True when a life was given
	bool addScore(int thePoints);
	// 0x49ab70 (feedingfrenzy2): the manifest's entry of the stage, null before the first
	const StageManifest::Stage* getStageEntry() const;

	int getMode() const { return mMode; }
	int getLives() const { return mLives; }
	int getScore() const { return mScore; }
	// The test command set's (docs/TEST_COMMANDS.md)
	void setScore(int theScore) { mScore = theScore; }
	void setLives(int theLives) { mLives = theLives; }
	int getContinues() const { return mContinues; }
	void setContinues(int theContinues) { mContinues = theContinues; }
	int getStage() const { return mStage; }
	void setStage(int theStage) { mStage = theStage; }
	int getTimeRollover() const { return mTimeRollover; }
	void setTimeRollover(int theTimeRollover) { mTimeRollover = theTimeRollover; }

	StageManifest& getStages() { return mStages; }
	StageManifest& getMenuStages() { return mMenuStages; }
	int getSpawnPhase() const { return mSpawnPhase; }
	void setSpawnPhase(int theSpawnPhase) { mSpawnPhase = theSpawnPhase; }
	int getFood() const { return mFood; }
	void setFood(int theFood) { mFood = theFood; }
	void addFood(int theFood) { mFood += theFood; }

private:
	int mMode;                    // +0x0c: GameMode
	int mLives;                   // +0x10
	int mScore;                   // +0x14
	int mContinues;               // +0x18
	int mNextLifeScore;           // +0x1c: the score that gives the next extra life
	int mLifeScoreStep;           // +0x20: the score between it and the one after
	int mSpawnPhase;              // +0x24: the phase every spawn uses; 0 when a stage starts (the constructor
	                              // leaves it, and nothing reads it before)
	int mStage;                   // +0x28: the stage's place in the manifest, -1 before the first
	int mTimeRollover;            // +0x2c: the timed mode's seconds carried into the stage
	// +0x30 (0), +0x34 (3), +0x38 and +0x3c: set by a new game, their readers not yet found
	int mFood;                    // +0x40: the food eaten in the stage, which grows the player through its phases
	StageManifest mStages;        // +0x44
	StageManifest mMenuStages;    // +0x78: the menu's backgrounds (the screensaver's in saver mode)
};

#endif
