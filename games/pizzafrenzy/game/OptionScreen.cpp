#include "OptionScreen.h"
#include "Button.h"

using namespace pizzafrenzy;

namespace
{
// The original's version text (application slot 0, 0x4bdb20): its exe's FileVersion string, read from the
// string table of the user's language with code page 04B0. The exe has only US English's (040904B0), which is what an
// English Windows reads; on another language the original shows "no version info". There is no SpecialBuild string.
const char* const GAME_VERSION = "5, 8, 30, 1";
}

// 0x43c2b0
void OptionScreen::setupOptions(engine::GenericApplication* theApp, Settings* theSettings)
{
	setup(theApp, theSettings);
	setVersionText(std::string("Version ") + GAME_VERSION);
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
