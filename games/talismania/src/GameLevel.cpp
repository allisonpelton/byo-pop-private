#include "GameLevel.h"

#include "AtlantisApp.h"
#include "Gameboard.h"

GameLevel::GameLevel(AtlantisApp* theApp, const GameLevelClass& theClass, engine::Container* theParent)
	: mApp(theApp), mClass(theClass)
{
	mLayout = new engine::GenericScreen();
	mLayout->load(mClass.levelLayoutFile, mApp->mTheme.get());
	theParent->addChild(mLayout.get());
	mLayout->show();

	std::map<std::string, BoardClass>::const_iterator aBoard = mApp->mData.boards.find(mClass.boardId);
	if (aBoard == mApp->mData.boards.end())
	{
		engine::getEngine()->log("NO BOARD NAMED %s\n", mClass.boardId.c_str());
		return;
	}
	engine::Container* aCells = dynamic_cast<engine::Container*>(mLayout->findComponent("boardcells"));
	if (aCells == nullptr)
	{
		engine::getEngine()->log("The level layout %s has no group boardcells\n", mClass.levelLayoutFile.c_str());
		return;
	}
	mBoard.reset(new Gameboard(mApp, aBoard->second, aCells));
}

GameLevel::~GameLevel() = default;
