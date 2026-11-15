#include "OptionScreen.h"
#include "Button.h"

using namespace pizzafrenzy;

// 0x43c2b0
void OptionScreen::setupOptions(engine::GenericApplication* theApp, Settings* theSettings)
{
	setup(theApp, theSettings);
	engine::CheckBox* aStory = static_cast<engine::CheckBox*>(getNode("storyOptionCheck"));
	if (aStory != nullptr)
		aStory->setChecked(theSettings->isReplayStory());
}

// 0x43c370
void OptionScreen::action(std::string theAction)
{
	if (theAction.compare("toggleStory") == 0)
	{
		engine::CheckBox* aStory = static_cast<engine::CheckBox*>(getNode("storyOptionCheck"));
		if (aStory != nullptr)
		{
			static_cast<Settings*>(mSettings.get())->setReplayStory(aStory->isChecked());
			mSettings->save();
		}
	}
	else
		GenericOptionScreen::action(theAction);
}
