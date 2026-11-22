#ifndef __FEEDINGFRENZY2_GAMEDATA_H__
#define __FEEDINGFRENZY2_GAMEDATA_H__

// The game's data (GameData, 0xb8 bytes, vtable 0x55b014 (feedingfrenzy2); its constructor 0x49ffe0
// (feedingfrenzy2)), the application's +0x3c: the game's stages and the menu's, each from its manifest.

#include "Object.h"
#include "StageManifest.h"

class GameData : public engine::Object
{
public:
	GameData() {}

	StageManifest& getStages() { return mStages; }
	StageManifest& getMenuStages() { return mMenuStages; }

private:
	// +0x0c (0) and +0x28 (-1): set by the constructor, their readers not yet found
	StageManifest mStages;        // +0x44
	StageManifest mMenuStages;    // +0x78: the menu's backgrounds (the screensaver's in saver mode)
};

#endif
