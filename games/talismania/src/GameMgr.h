#ifndef __TALISMANIA_GAMEMGR_H__
#define __TALISMANIA_GAMEMGR_H__

// The game manager (AtlantisGameMgr, made by the game mode's Play state, started by 0x41ca20) and the first chapter and
// level it brings up. Translated so far, in a reduced form: the mode's data set loaded, the first chapter of the class's
// progression found (0x41e120), the chapter's two data sets loaded (its LoadingChapterPreData and LoadingChapterData states,
// 0x414da0 and 0x415050, which the original runs a file a frame behind a loading screen; here at once) and its first level's
// board laid out and drawn. Not yet: the manager's states (game init, chapter intro, map, play), the chapter's states
// (the intro journal, the building scenes), the level's states, the hud, the map and the journal.

#include "AtlantisData.h"
#include "Container.h"

#include <memory>

class AtlantisApp;
class GameLevel;

class AtlantisGameMgr
{
public:
	AtlantisGameMgr(AtlantisApp* theApp, const GameModeClass& theMode);
	~AtlantisGameMgr();

	// 0x41ca20 (reduced)
	void start();
	void update(double theElapsed);

private:
	GameMgrClass* findClass();
	ChapterClass* findChapter(const std::string& theName);
	bool loadChapterData(const ChapterClass& theChapter);

	AtlantisApp* mApp;
	GameModeClass mMode;
	GameMgrClass mClass;
	engine::Ref<engine::Container> mLevelContainer;  // the "Level Container" the manager makes in the scene's main layer
	std::unique_ptr<GameLevel> mLevel;
};

#endif
