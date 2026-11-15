#include "GenericOptionScreen.h"
#include "Button.h"
#include "Host.h"
#include "TextItem.h"

using namespace engine;

namespace
{
// A check box of the screen by its name, null when the layout has none
CheckBox* findCheckBox(GenericOptionScreen* theScreen, const char* theName)
{
	return static_cast<CheckBox*>(theScreen->getNode(theName));
}

// 0x4669d0 (pizzafrenzy): a check box of a kind the application has none of is off, disabled and dimmed; otherwise it
// is enabled, at full opacity and shows the setting
void showOption(CheckBox* theCheckBox, bool theAvailable, bool theSetting)
{
	if (theCheckBox == nullptr)
		return;
	if (!theAvailable)
	{
		theCheckBox->setEnabled(false);
		theCheckBox->setChecked(false);
		theCheckBox->setOpacity(0.8f);
	}
	else
	{
		theCheckBox->setEnabled(true);
		theCheckBox->setOpacity(1.0f);
		theCheckBox->setChecked(theSetting);
	}
}
}

// 0x4669d0 (pizzafrenzy)
void GenericOptionScreen::setup(GenericApplication* theApp, GenericGameOptions* theSettings)
{
	showOption(findCheckBox(this, "soundOptionCheck"), theApp->hasSound(), theSettings->isSoundEnabled());
	showOption(findCheckBox(this, "musicOptionCheck"), theApp->hasMusic(), theSettings->isMusicEnabled());
	CheckBox* aFullScreen = findCheckBox(this, "fullScreenOptionCheck");
	if (aFullScreen != nullptr)
	{
		aFullScreen->setChecked(getHost()->isFullScreen());
		theSettings->setFullScreen(getHost()->isFullScreen());
	}
	mActionSignal.connect(this, &GenericOptionScreen::onAction);
	mSettings = theSettings;
	mApp = theApp;
}

// 0x4669d0 (pizzafrenzy), its last part: the version text (port: the game gives the text, which the original makes of
// the exe's version resource)
void GenericOptionScreen::setVersionText(const std::string& theText)
{
	TextItem* aVersion = static_cast<TextItem*>(getNode("VersionText"));
	if (aVersion != nullptr)
		aVersion->setText(theText);
}

// 0x466690 (pizzafrenzy)
void GenericOptionScreen::action(std::string theAction)
{
	if (theAction.compare("toggleSound") == 0)
	{
		CheckBox* aCheckBox = findCheckBox(this, "soundOptionCheck");
		if (aCheckBox != nullptr)
		{
			mSettings->setSoundEnabled(aCheckBox->isChecked());
			mApp->setSoundEnabled(mSettings->isSoundEnabled());
			mSettings->save();
		}
	}
	else if (theAction.compare("toggleMusic") == 0)
	{
		CheckBox* aCheckBox = findCheckBox(this, "musicOptionCheck");
		if (aCheckBox != nullptr)
		{
			mSettings->setMusicEnabled(aCheckBox->isChecked());
			mApp->setMusicEnabled(mSettings->isMusicEnabled());
			mSettings->save();
		}
	}
	else if (theAction.compare("toggleFullScreen") == 0)
	{
		CheckBox* aCheckBox = findCheckBox(this, "fullScreenOptionCheck");
		if (aCheckBox != nullptr)
		{
			mSettings->setFullScreen(aCheckBox->isChecked());
			getHost()->setFullScreen(mSettings->isFullScreen());
			mSettings->save();
		}
	}
}
