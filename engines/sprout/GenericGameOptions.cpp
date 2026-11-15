#include "GenericGameOptions.h"

using namespace engine;

// 0x479960 (pizzafrenzy): the original sets no value here (load does); the port starts them off
GenericGameOptions::GenericGameOptions(const std::string& thePublisher, const std::string& theGame)
	: mFullScreen(false), mMusicEnabled(false), mSoundEnabled(false)
{
	std::string aPath = thePublisher;
	aPath += "\\";
	aPath += theGame;
	mKey = new PersistKey(aPath);
}

// 0x479ae0 (pizzafrenzy)
void GenericGameOptions::load()
{
	mFullScreen = mKey->readInt("FullScreen", 1) != 0;
	mMusicEnabled = mKey->readInt("MusicEnabled", 1) != 0;
	mSoundEnabled = mKey->readInt("SoundEnabled", 1) != 0;
}

// 0x479e50 (pizzafrenzy)
void GenericGameOptions::save()
{
	mKey->writeInt("FullScreen", mFullScreen);
	mKey->writeInt("MusicEnabled", mMusicEnabled);
	mKey->writeInt("SoundEnabled", mSoundEnabled);
}
