#ifndef __TALISMANIA_GAMEMODE_H__
#define __TALISMANIA_GAMEMODE_H__

// The game mode (GameMode, 0x11c bytes, made by 0x4415e0 from a GameModeClass: "story" or "hero"): the object the main
// menu's Story and Hero buttons start, and the top of the game's flow. It runs a state machine of its own, its states
// members of the object (Init +0x9c, CheckNewGame +0xa4, ChapterSelect +0xd0, Play +0xec, Kill +0xf8, the current one
// at +0x104). The Init state's step 0x4430d0 moves to CheckNewGame; that state asks "new game or continue" over a saved
// game and otherwise waits 0.01 seconds (0x443b00) and goes on to Play, or to ChapterSelect when the mode has more than
// one difficulty; Play makes the game manager (AtlantisGameMgr) and hands over to it.
// Translated so far: the flow of a first game (no saved game, one difficulty): begin, the wait, the Play state's
// pop of the main menu. Not yet: the saved-game question (asknewgame.xml), ChapterSelect, Kill, the persistence, and the
// game manager the Play state makes.

#include "AtlantisData.h"
#include "State.h"

class AtlantisApp;

class GameMode
{
public:
	GameMode(AtlantisApp* theApp, const GameModeClass& theClass);

	// 0x4430d0 (Init's step, called by the menu's start): the CheckNewGame state entered
	void begin();
	// The play state's update (0x4068d0 calls the current state's)
	void update(engine::UpdateContext* theContext) { mStates.update(theContext); }

	const GameModeClass& getClass() const { return mClass; }
	AtlantisApp* getApp() const { return mApp; }

	// The states of the flow (GameModeState's subclasses, defined with the mode)
	class State;
	class CheckNewGameState;
	class PlayState;
	void goToPlay();

private:
	AtlantisApp* mApp;
	GameModeClass mClass;
	engine::StateMgr<engine::State> mStates;  // the current state at +0x104
};

#endif
