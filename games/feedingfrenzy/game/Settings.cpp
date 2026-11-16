#include "Settings.h"

using namespace feedingfrenzy;

namespace
{
// The registry key's publisher and game (0x433870)
const char* const PUBLISHER = "GameHouse";
const char* const GAME = "FeedingFrenzy";
}

// 0x433870: everything starts on (full screen, music, sound and tips) and the mouse sensitivity at 1
Settings::Settings() : engine::GenericGameOptions(PUBLISHER, GAME), mTipsEnabled(true), mMouseSensitivity(1)
{
	mFullScreen = true;
	mMusicEnabled = true;
	mSoundEnabled = true;
}

// 0x4339c0: each value that is not there is on (the mouse sensitivity 1, the score name empty)
void Settings::load()
{
	GenericGameOptions::load();
	mTipsEnabled = mKey->readInt("TipsEnabled", 1) != 0;
	mDefaultScoreName = mKey->readString("DefaultScoreName", "");
	mMouseSensitivity = mKey->readInt("MouseSensitivity", 1);
}

// 0x4332c0
void Settings::save()
{
	GenericGameOptions::save();
	mKey->writeInt("TipsEnabled", mTipsEnabled);
	mKey->writeInt("MouseSensitivity", mMouseSensitivity);
	mKey->writeString("DefaultScoreName", mDefaultScoreName);
}

// The factors of the four sensitivities (0x4331c0), and the one a value out of range gets
const float MOUSE_SPEED_FACTORS[4] = {0.8f, 1.1f, 1.3f, 1.55f};
const int MOUSE_SENSITIVITY_DEFAULT = 1;
const float MOUSE_SPEED_FACTOR_DEFAULT = 1.0f;

// 0x4331c0
float Settings::getMouseSpeedFactor()
{
	if (mMouseSensitivity >= 0 && mMouseSensitivity < 4)
		return MOUSE_SPEED_FACTORS[mMouseSensitivity];
	mMouseSensitivity = MOUSE_SENSITIVITY_DEFAULT;
	return MOUSE_SPEED_FACTOR_DEFAULT;
}
