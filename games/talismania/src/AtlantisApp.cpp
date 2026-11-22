#include "AtlantisApp.h"

#include "AtlantisData.h"
#include "Engine.h"

bool gUsingArchive = false;

// 0x401580
AtlantisApp::AtlantisApp()
{
}

// 0x401a20
AtlantisApp::~AtlantisApp()
{
}

AtlantisApp* AtlantisApp::boot()
{
	engine::Engine* anEngine = engine::getEngine();
	anEngine->setUseLooseFiles(true);
	if (anEngine->diskFileExists("Talismania.saf"))
	{
		anEngine->log("FOUND Talismania.saf, USING ARCHIVE\n");
		anEngine->setUseLooseFiles(false);
		anEngine->addArchive("Talismania.saf");
		gUsingArchive = true;
	}
	// The data set list, then the base data set ("baseset": config/scenes.xml and config/gameoptions.xml)
	AtlantisApp* anApp = new AtlantisApp();
	anApp->mData.loadFile("config/datasets.xml");
	anApp->mData.loadDataSet("baseset");
	if (anApp->mData.appOptions.find("std") == anApp->mData.appOptions.end())
		anEngine->log("Cannot find application options \"std\"\n");
	else
		anApp->mOptions = anApp->mData.appOptions["std"];
	anApp->addRef();
	return anApp;
}

void AtlantisApp::configure()
{
}

void AtlantisApp::start()
{
	engine::Engine* anEngine = engine::getEngine();
	anEngine->log("==========================\nINITIALIZING GAME \"%s\": Version %s.%d\n==========================\n",
				  mOptions.gameTitle.c_str(), mOptions.gameVersion.c_str(), mOptions.gameBuild);
	std::map<std::string, SceneClass>::iterator aSceneClass = mData.scenes.find(mOptions.sceneClass);
	if (aSceneClass == mData.scenes.end())
	{
		anEngine->log("Cannot find scene class \"%s\"\n", mOptions.sceneClass.c_str());
		return;
	}
	mScene = new AtlantisScene(aSceneClass->second);
	mScene->enable();
}
