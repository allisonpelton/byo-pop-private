#include "Settings.h"
#include "App.h"
#include "Button.h"
#include "Host.h"
#include "Sound.h"
#include "TextItem.h"

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

namespace
{
// A box of the options screen (null when the screen has none of the name)
engine::CheckBox* getCheckBox(engine::ScreenLayout* theScreen, const char* theName)
{
	return static_cast<engine::CheckBox*>(theScreen->getNode(theName));
}

// The names of the screen's nodes
const char* const SOUND_ERROR_TEXT = "soundErrorText";
const char* const MUSIC_CHECK = "musicCheck";
const char* const SOUND_CHECK = "soundCheck";
const char* const FULL_SCREEN_CHECK = "fullScreenCheck";
const char* const TOOL_TIPS_CHECK = "toolTipsCheck";
const char* const MOUSE_BUTTONS[4] = {"Mouse1", "Mouse2", "Mouse3", "Mouse4"};
const char* const VERSION_TEXT = "versionNumber";
}

// 0x433540
void Settings::updateScreen(engine::ScreenLayout* theScreen, const std::string& theVersion)
{
	if (engine::App::getSoundSystem() == nullptr)
	{
		theScreen->getNode(SOUND_ERROR_TEXT)->setVisible(true);
		theScreen->getNode(MUSIC_CHECK)->setEnabled(false);
		theScreen->getNode(SOUND_CHECK)->setEnabled(false);
		mMusicEnabled = false;
		mSoundEnabled = false;
	}
	else
		theScreen->getNode(SOUND_ERROR_TEXT)->setVisible(false);

	engine::CheckBox* aBox = getCheckBox(theScreen, SOUND_CHECK);
	if (aBox != nullptr)
		aBox->setChecked(mSoundEnabled);
	aBox = getCheckBox(theScreen, MUSIC_CHECK);
	if (aBox != nullptr)
		aBox->setChecked(mMusicEnabled);
	aBox = getCheckBox(theScreen, FULL_SCREEN_CHECK);
	if (aBox != nullptr)
	{
		mFullScreen = engine::getHost()->isFullScreen();
		aBox->setChecked(mFullScreen);
	}
	aBox = getCheckBox(theScreen, TOOL_TIPS_CHECK);
	if (aBox != nullptr)
		aBox->setChecked(mTipsEnabled);
	for (int i = 0; i < 4; i++)
	{
		aBox = getCheckBox(theScreen, MOUSE_BUTTONS[i]);
		if (aBox != nullptr)
			aBox->setChecked(mMouseSensitivity == i);
	}
	engine::TextItem* aVersion = static_cast<engine::TextItem*>(theScreen->getNode(VERSION_TEXT));
	if (aVersion != nullptr)
		aVersion->setText(theVersion);
}
