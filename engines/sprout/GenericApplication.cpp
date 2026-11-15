#include "GenericApplication.h"
#include "Host.h"
#include "Sound.h"

using namespace engine;

// 0x46ae90 (pizzafrenzy)
void GenericApplication::setSoundEnabled(bool theEnabled)
{
	SoundSystem* aSounds = App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->setCategoryVolume(0, theEnabled ? 1.0f : 0.0f);
}

// 0x46ae50 (pizzafrenzy)
void GenericApplication::setMusicEnabled(bool theEnabled)
{
	SoundSystem* aSounds = App::getSoundSystem();
	if (aSounds != nullptr)
		aSounds->setCategoryVolume(1, theEnabled ? 1.0f : 0.0f);
}

// 0x46ae00 (pizzafrenzy)
void GenericApplication::initSound()
{
	SoundSystem* aSounds = getHost()->createSoundSystem();
	App::instance()->setSoundSystem(aSounds);
	mHasSound = aSounds != nullptr;
	mHasMusic = aSounds != nullptr;
}
