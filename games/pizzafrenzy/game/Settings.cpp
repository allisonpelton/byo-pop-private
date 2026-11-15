#include "Settings.h"

using namespace pizzafrenzy;

// 0x418c70
void Settings::load()
{
	GenericGameOptions::load();
	mReplayStory = mKey->readInt("ReplayStory", 1) != 0;
}

// 0x418d20
void Settings::save()
{
	GenericGameOptions::save();
	mKey->writeInt("ReplayStory", mReplayStory);
}
