#ifndef __PIZZAFRENZY_OPTIONSCREEN_H__
#define __PIZZAFRENZY_OPTIONSCREEN_H__

// The options screen (0x1e4 bytes, constructor 0x4010d0, vtable 0x4f92d0): the engine's, with a check box for whether
// the story shows again

#include "GenericOptionScreen.h"
#include "Settings.h"

namespace pizzafrenzy
{

class OptionScreen : public engine::GenericOptionScreen
{
public:
	// 0x184: 0x43c2b0: the engine's setup, then the story's check box shows the setting
	void setupOptions(engine::GenericApplication* theApp, Settings* theSettings);
	// 0x180: 0x43c370: the story's check box flips the setting, which is saved; the rest is the engine's
	void action(std::string theAction) override;
};

} // namespace pizzafrenzy

#endif
