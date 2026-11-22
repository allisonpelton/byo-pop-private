#include "AtlantisApp.h"

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
	// The base data set ("baseset": config/scenes.xml and config/gameoptions.xml) is loaded here, and the application
	// is made from the options named "std" (not yet translated)
	AtlantisApp* anApp = new AtlantisApp();
	anApp->addRef();
	return anApp;
}

void AtlantisApp::configure()
{
}

void AtlantisApp::start()
{
}
