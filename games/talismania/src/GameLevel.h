#ifndef __TALISMANIA_GAMELEVEL_H__
#define __TALISMANIA_GAMELEVEL_H__

// A game level (GameLevel): the level's layout screen (the level class's levellayoutfile: the board's backing pictures and
// the groups the board's cells, paths and tops go in) and its board. Translated so far: the layout screen and the board.
// Not yet: the level's states (init, intro, play, cool down, done), its clock, jackpot, meters and spawning.

#include "AtlantisData.h"
#include "Container.h"
#include "GenericScreen.h"

#include <memory>

class AtlantisApp;
class Gameboard;

class GameLevel
{
public:
	GameLevel(AtlantisApp* theApp, const GameLevelClass& theClass, engine::Container* theParent);
	~GameLevel();

private:
	AtlantisApp* mApp;
	GameLevelClass mClass;
	engine::Ref<engine::GenericScreen> mLayout;
	std::unique_ptr<Gameboard> mBoard;
};

#endif
