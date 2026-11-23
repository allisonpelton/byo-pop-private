#include "GameMgr.h"

#include "AtlantisApp.h"
#include "GameLevel.h"

namespace
{

// The data's names meet the progressions' in lower case (a chapter "A" is listed as "a"): where the original makes them
// meet is not found, so the names are compared without regard to case
bool sameName(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++)
		if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
			return false;
	return true;
}

} // namespace

AtlantisGameMgr::AtlantisGameMgr(AtlantisApp* theApp, const GameModeClass& theMode) : mApp(theApp), mMode(theMode)
{
}

AtlantisGameMgr::~AtlantisGameMgr() = default;

// The game class the mode's default difficulty names (0x40e330 and after)
GameMgrClass* AtlantisGameMgr::findClass()
{
	std::map<std::string, std::string>::const_iterator aDiff = mMode.diffModes.find(mMode.defaultDiffMode);
	if (aDiff == mMode.diffModes.end())
		return nullptr;
	std::map<std::string, GameMgrClass>::iterator aClass = mApp->mData.gameOptions.find(aDiff->second);
	return aClass == mApp->mData.gameOptions.end() ? nullptr : &aClass->second;
}

ChapterClass* AtlantisGameMgr::findChapter(const std::string& theName)
{
	for (std::map<std::string, ChapterClass>::iterator it = mApp->mData.chapters.begin(); it != mApp->mData.chapters.end(); ++it)
		if (sameName(it->first, theName))
			return &it->second;
	return nullptr;
}

// 0x414da0 and 0x415050: the chapter's two data sets
bool AtlantisGameMgr::loadChapterData(const ChapterClass& theChapter)
{
	if (!theChapter.chapterPreDataSet.empty() && !mApp->mData.loadDataSet(theChapter.chapterPreDataSet))
		return false;
	if (!theChapter.chapterDataSet.empty() && !mApp->mData.loadDataSet(theChapter.chapterDataSet))
		return false;
	return true;
}

void AtlantisGameMgr::start()
{
	GameMgrClass* aClass = findClass();
	if (aClass == nullptr)
	{
		engine::getEngine()->log("CAN'T FIND GAME CLASS FOR MODE %s\n", mMode.name.c_str());
		return;
	}
	mClass = *aClass;
	if (!mClass.dataSet.empty() && !mApp->mData.loadDataSet(mClass.dataSet))
		return;

	// 0x41ca20: a container for the level in the scene's main layer
	mLevelContainer = new engine::Container();
	mLevelContainer->setName("Level Container");
	engine::Container* aMain = dynamic_cast<engine::Container*>(mApp->mScene->getLayer("main"));
	if (aMain != nullptr)
		aMain->addChild(mLevelContainer.get());

	// 0x41e120: the first chapter of the progression
	if (mClass.chapterProgression.empty())
		return;
	ChapterClass* aChapter = findChapter(mClass.chapterProgression[0]);
	if (aChapter == nullptr)
	{
		engine::getEngine()->log("CAN'T LOAD FIRST CHAPTER!\n");
		return;
	}
	if (!loadChapterData(*aChapter) || aChapter->levelProgression.empty())
		return;

	// The first level of the chapter's progression (the data's __bonus entries are the bonus levels')
	std::map<std::string, GameLevelClass>::const_iterator aLevel = mApp->mData.gameLevels.find(aChapter->levelProgression[0]);
	if (aLevel == mApp->mData.gameLevels.end())
	{
		engine::getEngine()->log("NO LEVEL NAMED %s\n", aChapter->levelProgression[0].c_str());
		return;
	}
	mLevel.reset(new GameLevel(mApp, aLevel->second, mLevelContainer.get()));
	// The camera comes down from the sky to the land where the level is (where the original moves it is not yet read)
	mApp->mScene->setRegion("land");
}

void AtlantisGameMgr::update(double)
{
}
